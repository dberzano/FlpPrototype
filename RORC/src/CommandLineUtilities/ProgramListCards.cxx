/// \file ProgramListCards.cxx
/// \brief Utility that lists the ReadoutCard devices on the system
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <sstream>
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "RocPciDevice.h"
#include "ReadoutCard/ChannelFactory.h"
#include <boost/format.hpp>

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using std::cout;
using std::endl;

namespace {
class ProgramListCards: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"List Cards", "Lists installed cards and some basic information about them", "roc-list-cards"};
    }

    virtual void addOptions(boost::program_options::options_description&)
    {
    }

    virtual void run(const boost::program_options::variables_map&)
    {
      auto cardsFound = AliceO2::roc::RocPciDevice::findSystemDevices();

      std::ostringstream table;

      auto formatHeader = "  %-3s %-6s %-10s %-11s %-11s %-8s %-15s\n";
      auto formatRow = "  %-3s %-6s %-10s 0x%-9s 0x%-9s %-8s %-15s\n";
      auto header = boost::str(boost::format(formatHeader)
          % "#" % "Type" % "PCI Addr" % "Vendor ID" % "Device ID" % "Serial" % "FW Version");
      auto lineFat = std::string(header.length(), '=') + '\n';
      auto lineThin = std::string(header.length(), '-') + '\n';

      table << lineFat << header << lineThin;

      int i = 0;
      for (const auto& card : cardsFound) {
        // Try to figure out the firmware version
        std::string firmware = "n/a";
        try {
          Parameters params = Parameters::makeParameters(card.pciAddress, 0);
          // Temporary (hopefully) workaround, because DmaChannel requires a buffer when initializing
          params.setBufferParameters(buffer_parameters::File{"/dev/shm/rorc_channel_utility_dummy_buffer", 4*1024});
          firmware = ChannelFactory().getDmaChannel(params)->getFirmwareInfo().value_or("n/a");
        }
        catch (const Exception& e) {
          if (isVerbose()) {
            cout << "Could not get firmware version string:\n" << boost::diagnostic_information(e) << '\n';
          }
        }

        auto format = boost::format(formatRow) % i % CardType::toString(card.cardType) % card.pciAddress.toString()
            % card.pciId.vendor % card.pciId.device;
        if (auto serial = card.serialNumber) {
          format % serial.get();
        } else {
          format % "n/a";
        }
        format % firmware;
        table << format;
        i++;
      }

      table << lineFat;
      cout << table.str();
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramListCards().execute(argc, argv);
}
