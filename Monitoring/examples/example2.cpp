#include <iostream>
#include "Monitoring/Collector.h"

namespace Monitoring = AliceO2::Monitoring;

int main() {
       	// parse configuration file
       	ConfigFile mConfigFile;
       	mConfigFile.load("file:/home/awegrzyn/hackathon/Monitoring/examples/example.ini");
       	 
       	// create monitoring object and confuguration as parameter to constructor
       	std::unique_ptr<Monitoring::Core::Collector> collector(new Monitoring::Core::Collector(mConfigFile));
       	 
	// set user specific entity value, eg.:
	collector->setEntity("testEntity");

	// now send an application specific metric
	// 10 is the value
	// myCrazyMetric is the name of the metri
	collector->send(10, "myCrazyMetric");
}