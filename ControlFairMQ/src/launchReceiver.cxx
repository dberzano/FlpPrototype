/// \file    launchReceiver.cxx
/// \brief   Example of a FairMQDevice receiver
///
/// \author  Vasco Barroso, CERN

#include "ControlFairMQ/Receiver.h"
#include "ControlFairMQ/Version.h"
#include <boost/program_options.hpp>
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include <FairMQTransportFactoryZMQ.h>
#include <iostream>

using AliceO2::InfoLogger::InfoLogger;
namespace program_options = boost::program_options;
namespace ControlFairMQ = AliceO2::ControlFairMQ;

int main(int argc, char* argv[])
{
  std::string configFile;
  std::string deviceId;
  program_options::options_description commandLineOptions("Allowed Options");
  program_options::variables_map optionsValues;
  FairMQProgOptions config;
  InfoLogger logger;

  // define options from command line
  commandLineOptions.add_options()
    ("help,h", "Print help message")
    ("version,v", "Show program name/version banner and exit.")
    ("revision", "Print the revision number")
	("id", program_options::value<std::string>(&deviceId), "Device ID")
    ("config-file,c", program_options::value<std::string>(&configFile), "ALFA Configuration file path");

  try {
    // parse command line options
    program_options::store(program_options::parse_command_line(argc, argv, commandLineOptions), optionsValues);
    program_options::notify(optionsValues);

    // check for help
    if (optionsValues.count("help")) {
      logger << commandLineOptions << InfoLogger::endm;
      return EXIT_SUCCESS;
    }

    // check for version
    if (optionsValues.count("version")) {
      logger << "launchReceiver version " << ControlFairMQ::Core::Version::getString() << InfoLogger::endm;
      return EXIT_SUCCESS;
    }

    // check for revision
    if (optionsValues.count("revision")) {
      logger << "revision : " << ControlFairMQ::Core::Version::getRevision() << InfoLogger::endm;
      return EXIT_SUCCESS;
    }

    // check ALFA config file
    if (!optionsValues.count("config-file")) {
      throw program_options::error("missing mandatory option --config-file");
    }

    // check device ID
    if (!optionsValues.count("id")) {
      throw program_options::error("missing mandatory option --id");
    }

  }
  catch(program_options::error& e)
  {
    // error with a configuration option
    logger << "ERROR: " << e.what() << InfoLogger::endm;
    logger << commandLineOptions << InfoLogger::endm;
    return EXIT_FAILURE;
  }

  ControlFairMQ::Core::Receiver receiver;

  try
  {
    // Get the config for the receiver
	  config.UserParser<FairMQParser::JSON>(configFile, deviceId);

    // Set the channels based on the config
    receiver.fChannels = config.GetFairMQMap();

    // Get the proper transport factory
#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif
    receiver.SetTransport(transportFactory);

    receiver.SetProperty(ControlFairMQ::Core::Receiver::Id, deviceId);

    receiver.ChangeState("INIT_DEVICE");
    receiver.WaitForEndOfState("INIT_DEVICE");

    receiver.ChangeState("INIT_TASK");
    receiver.WaitForEndOfState("INIT_TASK");

    receiver.ChangeState("RUN");
    receiver.WaitForEndOfState("RUN");

    receiver.ChangeState("RESET_TASK");
    receiver.WaitForEndOfState("RESET_TASK");

    receiver.ChangeState("RESET_DEVICE");
    receiver.WaitForEndOfState("RESET_DEVICE");

    receiver.ChangeState("END");

  }
  catch (std::exception& e)
  {
    logger << e.what() << InfoLogger::endm;
    logger << "Command line options are the following: " << InfoLogger::endm;
    config.PrintHelp();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
