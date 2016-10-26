/// \file RorcDevice.cxx
/// \brief Implementation of the RorcDevice class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RorcDevice.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include <functional>
#include <iostream>
#include "Crorc/Crorc.h"
#include "Cru/CruRegisterIndex.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDevice.h"
#include "RORC/Exception.h"
#include "Util.h"

namespace AliceO2 {
namespace Rorc {

uint32_t crorcGetSerial(PciDevice* pciDevice);
uint32_t cruGetSerialOld(PciDevice* pciDevice);
uint32_t cruGetSerialNew(PciDevice* pciDevice);

struct DeviceType
{
    CardType::type cardType;
    const PciId pciId;
    std::function<uint32_t (PciDevice* pciDevice)> getSerial;
};

const std::vector<DeviceType> deviceTypesOld = {
    { CardType::Crorc, {"0033", "10dc"}, crorcGetSerial }, // C-RORC
    { CardType::Cru, {"e001", "1172"}, cruGetSerialOld }, // Altera dev board CRU
};

const std::vector<DeviceType> deviceTypesNew = {
    { CardType::Crorc, {"0033", "10dc"}, crorcGetSerial }, // C-RORC
    { CardType::Cru, {"e001", "1172"}, cruGetSerialNew }, // Altera dev board CRU
};

RorcDevice::RorcDevice(int serialNumber)
    : mPciId({"unknown", "unknown"}), mSerialNumber(-1), mCardType(CardType::Unknown)
{
  try {
    for (const auto& type : deviceTypesNew) {
      Util::resetSmartPtr(mPdaDevice, type.pciId);
      auto pciDevices = mPdaDevice->getPciDevices();

      for (const auto& pciDevice : pciDevices) {
        int serial = type.getSerial(pciDevice);
        if (serial == serialNumber) {
          mPciDevice = pciDevice;
          mCardType = type.cardType;
          mSerialNumber = serial;
          mPciId = type.pciId;
          return;
        }
      }
    }
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Could not find card"));
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serialNumber);
    addPossibleCauses(e, {"Invalid serial number search target"});
    throw;
  }
}

RorcDevice::RorcDevice(int serialNumber, bool useNewCruSerialLocation)
  : mPciId({"unknown", "unknown"}), mSerialNumber(-1), mCardType(CardType::Unknown)
{
  try {
    for (const auto& type : useNewCruSerialLocation ? deviceTypesNew : deviceTypesOld) {
      Util::resetSmartPtr(mPdaDevice, type.pciId);
      auto pciDevices = mPdaDevice->getPciDevices();

      for (const auto& pciDevice : pciDevices) {
        int serial = type.getSerial(pciDevice);
        if (serial == serialNumber) {
          mPciDevice = pciDevice;
          mCardType = type.cardType;
          mSerialNumber = serial;
          mPciId = type.pciId;
          return;
        }
      }
    }
    BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Could not find card"));
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serialNumber);
    addPossibleCauses(e, { "Invalid serial number search target" });
    throw;
  }
}

RorcDevice::~RorcDevice()
{
}

std::vector<RorcDevice::CardDescriptor> RorcDevice::findSystemDevices()
{
  std::vector<RorcDevice::CardDescriptor> cards;

  for (const auto& type : deviceTypesNew) {
    Pda::PdaDevice pdaDevice(type.pciId);
    auto pciDevices = pdaDevice.getPciDevices();

    for (const auto& pciDevice : pciDevices) {
      RorcDevice::CardDescriptor cd;
      cd.cardType = type.cardType;
      cd.serialNumber = type.getSerial(pciDevice);
      cd.pciId = type.pciId;

      std::cout << "type:" << CardType::toString(cd.cardType)
          << " serial:" << cd.serialNumber
          << " vendor:" << cd.pciId.vendor
          << " device:" << cd.pciId.device << '\n';

      cards.push_back(cd);
    }
  }
  return cards;
}

std::vector<RorcDevice::CardDescriptor> RorcDevice::findSystemDevices(int serialNumber)
{
  std::vector<RorcDevice::CardDescriptor> cards;
  try {
    for (const auto& type : deviceTypesNew) {
      Pda::PdaDevice pdaDevice(type.pciId);
      auto pciDevices = pdaDevice.getPciDevices();

      for (const auto& pciDevice : pciDevices) {
        int serial = type.getSerial(pciDevice);
        if (serial == serialNumber) {
          RorcDevice::CardDescriptor cd;
          cd.cardType = type.cardType;
          cd.serialNumber = serial;
          cd.pciId = type.pciId;
          cards.push_back(cd);
        }
      }
    }
  }
  catch (boost::exception& e) {
    e << errinfo_rorc_serial_number(serialNumber);
    addPossibleCauses(e, {"Invalid serial number search target"});
    throw;
  }
  return cards;
}

void RorcDevice::printDeviceInfo(std::ostream& ostream)
{
  uint16_t domainId;
  PciDevice_getDomainID(mPciDevice, &domainId);

  uint8_t busId;
  PciDevice_getBusID(mPciDevice, &busId);

  uint8_t functionId;
  PciDevice_getFunctionID(mPciDevice, &functionId);

  const PciBarTypes* pciBarTypesPtr;
  PciDevice_getBarTypes(mPciDevice, &pciBarTypesPtr);

  auto barType = *pciBarTypesPtr;
  auto barTypeString =
      barType == PCIBARTYPES_NOT_MAPPED ? "NOT_MAPPED" :
      barType == PCIBARTYPES_IO ? "IO" :
      barType == PCIBARTYPES_BAR32 ? "BAR32" :
      barType == PCIBARTYPES_BAR64 ? "BAR64" :
      "n/a";
}

/// Uses old location of serial number
uint32_t cruGetSerialOld(PciDevice* pciDevice)
{
  int channel = 0; // Must use BAR 0 to access serial number
  Pda::PdaBar pdaBar(pciDevice, channel);
  uint32_t serial = pdaBar.getUserspaceAddressU32()[CruRegisterIndex::SERIAL_NUMBER];
  return serial;
}

/// Uses new location of serial number
uint32_t cruGetSerialNew(PciDevice* pciDevice)
{
//  BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("New serial number location not yet supported"));
  int channel = 2; // Must use BAR 2 to access serial number
  Pda::PdaBar pdaBar(pciDevice, channel);
  uint32_t serial = pdaBar.getUserspaceAddressU32()[0x0 / 4];
  return serial;
}

// The RORC headers have a lot of macros that cause problems with the rest of this file, so we include it down here.
#include "c/rorc/rorc.h"

// TODO Clean up, C++ificate
uint32_t crorcGetSerial(PciDevice* pciDevice)
{
  int channel = 0; // Must use BAR 0 to access flash
  Pda::PdaBar pdaBar(pciDevice, channel);
  uint32_t serial = Crorc::getSerial(pdaBar.getUserspaceAddress());
  return serial;
}

} // namespace Rorc
} // namespace AliceO2
