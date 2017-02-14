///
/// \file 4-RateDerivedMetric.cxx
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#include <iostream>
#include "Monitoring/Collector.h"

namespace Monitoring = AliceO2::Monitoring;

int main() {
  // create monitoring object, pass configuration path as parameter
  std::unique_ptr<Monitoring::Collector> collector(
    new Monitoring::Collector("file:///home/awegrzyn/hackathon/Monitoring/examples/SampleConfig.ini")
  );  

  // derived metric :  rate
  collector->addDerivedMetric("myMetric", Monitoring::DerivedMetricMode::RATE);

  // now send at least two metrics to see the result
  collector->send(10, "myMetric");
  collector->send(20, "myMetric");
  collector->send(30, "myMetric");
  collector->send(50, "myMetric");
}
