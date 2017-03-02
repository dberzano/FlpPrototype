/// \file Program.cxx
/// \brief Implementation of the Program class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Program.h"
#include <iomanip>
#include <iostream>
#include <boost/version.hpp>
#include "ExceptionInternal.h"
#include "RORC/Version.h"
#include "Utilities/System.h"

namespace AliceO2 {
namespace Rorc {
namespace CommandLineUtilities {

using std::cout;
using std::endl;
namespace po = boost::program_options;

namespace {
  const std::string HELP_SWITCH = "help";
  const std::string VERBOSE_SWITCH = "verbose";
  const std::string VERSION_SWITCH = "version";
}

std::atomic<bool> Program::sFlagSigInt(false);

void Program::sigIntHandler(int)
{
  sFlagSigInt = true;
}

Program::Program()
    : mVerbose(false)
{
}

Program::~Program()
{
}

void Program::printHelp (const po::options_description& optionsDescription)
{
  auto util = getDescription();
  cout << "#### RORC Utility: " << util.name << "\n"
  << util.description << '\n'
  << '\n'
  << optionsDescription
  << '\n'
  << "Example:\n"
  << "  " << util.usage << '\n';
}

int Program::execute(int argc, char** argv)
{
  Utilities::setSigIntHandler(sigIntHandler);

  auto optionsDescription = Options::createOptionsDescription();
  auto prnHelp = [&](){ printHelp(optionsDescription); };

  // We add a verbose switch
  optionsDescription.add_options()
      (VERBOSE_SWITCH.c_str(), "Verbose output")
      (VERSION_SWITCH.c_str(), "Display RORC library version");

  // Subclass will add own options
  addOptions(optionsDescription);

  try {
    // Parse options and get the resulting map of variables

    po::variables_map variablesMap;
    try {
      po::store(po::parse_command_line(argc, argv, optionsDescription), variablesMap);

      if (variablesMap.count(HELP_SWITCH)) {
        prnHelp();
        return 0;
      }

      po::notify(variablesMap);
    }
    catch (const po::unknown_option& e) {
      BOOST_THROW_EXCEPTION(ProgramOptionException()
          << ErrorInfo::Message("Unknown option '" + e.get_option_name() + "'"));
    }

    if (variablesMap.count(VERSION_SWITCH)) {
      cout << "RORC lib     " << Core::Version::getString() << '\n' << "VCS version  " << Core::Version::getRevision()
          << '\n';
      return 0;
    }

    mVerbose = bool(variablesMap.count(VERBOSE_SWITCH));

    // Start the actual program
    run(variablesMap);
  }
  catch (const ProgramOptionException& e) {
    auto message = boost::get_error_info<AliceO2::Rorc::ErrorInfo::Message>(e);
    std::cout << "Program options invalid: " << *message << "\n\n";
    prnHelp();
  }
  catch (const po::error& e) {
    std::cout << "Program options error: " << e.what() << "\n\n";
    prnHelp();
  }
  catch (const std::exception& e) {
#if (BOOST_VERSION >= 105400)
    std::cout << "Error: " << e.what() << '\n' << boost::diagnostic_information(e, isVerbose()) << '\n';
#else
#pragma message "BOOST_VERSION < 105400"
    std::cout << "Error: " << e.what() << '\n' << boost::diagnostic_information(e) << '\n';
#endif
  }

  return 0;
}

} // namespace CommandLineUtilities
} // namespace Rorc
} // namespace AliceO2
