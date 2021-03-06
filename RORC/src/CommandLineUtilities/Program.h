/// \file Program.h
/// \brief Definition of the Program class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include "Common/Program.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "ReadoutCard/Exception.h"

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {

/// Helper class for making a ReadoutCard utility program. It adds logging facilities to the Common::Program
/// - SIGINT signals
class Program : public AliceO2::Common::Program
{
  public:
    virtual ~Program() = default;

  protected:
    /// Get Program's InfoLogger instance
    InfoLogger::InfoLogger& getLogger()
    {
      return mLogger;
    }

    InfoLogger::InfoLogger::Severity getLogLevel() const
    {
      return mLogLevel;
    }

    void setLogLevel(InfoLogger::InfoLogger::Severity logLevel = InfoLogger::InfoLogger::Severity::Info)
    {
      mLogLevel = logLevel;
    }

  private:
    InfoLogger::InfoLogger mLogger;
    InfoLogger::InfoLogger::Severity mLogLevel = InfoLogger::InfoLogger::Severity::Info;
};

} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
