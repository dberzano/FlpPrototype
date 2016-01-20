/// \file    DataCollectorApMon.cxx
/// \brief   Implementation of the DataCollectorApMon class.
///
/// \author  Vasco Barroso, CERN

#include "Monitoring/DataCollectorApMon.h"
#include "Monitoring/FileNotFoundException.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <stdio.h>
#include <string>

namespace AliceO2 {
namespace Monitoring {
namespace Core {

using AliceO2::InfoLogger::InfoLogger;

DataCollectorApMon::DataCollectorApMon(const std::string configurationFile)
  : mConfigurationFile(configurationFile)
{
  try {
    // create ApMon object
    mApMon = new ApMon(const_cast<char *>(configurationFile.c_str()));

    // configure process monitoring
    configureProcessMonitoring();

  } catch (std::runtime_error& e) {
    throw FileNotFoundException(configurationFile);
  }
}

DataCollectorApMon::~DataCollectorApMon()
{
  delete mApMon;
}

ApMon* DataCollectorApMon::getApMon() const
{
  return mApMon;
}

const std::string& DataCollectorApMon::getConfigurationFile() const
{
  return mConfigurationFile;
}

void DataCollectorApMon::sendValue(std::string cluster, std::string node, std::string metric, int value)
{
  getApMon()->sendParameter(const_cast<char *>(cluster.c_str()), const_cast<char *>(node.c_str()),
      const_cast<char *>(metric.c_str()), value);
}

void DataCollectorApMon::sendValue(std::string cluster, std::string node, std::string metric, double value)
{
  getApMon()->sendParameter(const_cast<char *>(cluster.c_str()), const_cast<char *>(node.c_str()),
      const_cast<char *>(metric.c_str()), value);
}

void DataCollectorApMon::sendValue(std::string cluster, std::string node, std::string metric, std::string value)
{
  getApMon()->sendParameter(const_cast<char *>(cluster.c_str()), const_cast<char *>(node.c_str()),
      const_cast<char *>(metric.c_str()), const_cast<char *>(value.c_str()));
}

void DataCollectorApMon::sendTimedValue(std::string cluster, std::string node, std::string metric, int value, int timestamp)
{
  getApMon()->sendTimedParameter(const_cast<char *>(cluster.c_str()), const_cast<char *>(node.c_str()),
      const_cast<char *>(metric.c_str()), XDR_INT32, (char *) &value, timestamp);
}

void DataCollectorApMon::sendTimedValue(std::string cluster, std::string node, std::string metric, double value,
    int timestamp)
{
  getApMon()->sendTimedParameter(const_cast<char *>(cluster.c_str()), const_cast<char *>(node.c_str()),
      const_cast<char *>(metric.c_str()), XDR_REAL64, (char *) &value, timestamp);
}

void DataCollectorApMon::sendTimedValue(std::string cluster, std::string node, std::string metric, std::string value,
    int timestamp)
{
  getApMon()->sendTimedParameter(const_cast<char *>(cluster.c_str()), const_cast<char *>(node.c_str()),
      const_cast<char *>(metric.c_str()), XDR_STRING, const_cast<char *>(value.c_str()), timestamp);
}

void DataCollectorApMon::configureProcessMonitoring()
{
  // get current working dir
  char *currentWorkingDir;
  currentWorkingDir = get_current_dir_name();

  getLogger() << "Setting default Cluster to '" << getDefaultCluster() << "'" << InfoLogger::endm;
  getLogger() << "Setting process unique ID to '" << getProcessUniqueId() << "'" << InfoLogger::endm;
  getLogger() << "Setting working dir to '" << std::string(currentWorkingDir) << "'" << InfoLogger::endm;

  // add process monitoring
  mApMon->addJobToMonitor(getpid(), currentWorkingDir, const_cast<char *>(getDefaultCluster().c_str()), const_cast<char *>(getProcessUniqueId().c_str()));

  free(currentWorkingDir);
}

std::string DataCollectorApMon::getDefaultCluster() const
{
  return std::string("defaultCluster");
}

} // namespace Core
} // namespace Monitoring
} // namespace AliceO2
