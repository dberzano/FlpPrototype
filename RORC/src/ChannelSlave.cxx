/// \file ChannelSlave.cxx
/// \brief Implementation of the ChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelSlave.h"
#include "Utilities/SmartPointer.h"

namespace AliceO2 {
namespace Rorc {

ChannelSlave::ChannelSlave(const Parameters& parameters)
 : mChannelNumber(parameters.getChannelNumberRequired())
{
  auto id = parameters.getCardIdRequired();
  if (auto serial = boost::get<int>(&id)) {
    Utilities::resetSmartPtr(mRorcDevice, *serial);
    mSerialNumber = *serial;
  } else if (auto address = boost::get<PciAddress>(&id)) {
    Utilities::resetSmartPtr(mRorcDevice, *address);
    mSerialNumber = mRorcDevice->getSerialNumber();
  }

  Utilities::resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), mChannelNumber);
}

ChannelSlave::~ChannelSlave()
{
}

uint32_t ChannelSlave::readRegister(int index)
{
  // TODO Access restriction
  return mPdaBar->getRegister<uint32_t>(index * sizeof(uint32_t));
}

void ChannelSlave::writeRegister(int index, uint32_t value)
{
  // TODO Access restriction
  mPdaBar->setRegister<uint32_t>(index * sizeof(uint32_t), value);
}

} // namespace Rorc
} // namespace AliceO2
