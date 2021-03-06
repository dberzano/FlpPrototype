///
/// \file   TaskFactory.cxx
/// \author Barthelemy von Haller
///

// std
#include <sstream>
// ROOT
#include <TSystem.h>
#include <TClass.h>
#include <TROOT.h>
#include <QualityControl/TaskDevice.h>
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/TaskFactory.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;
using namespace AliceO2::Common;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskFactory::TaskFactory()
{
}

TaskFactory::~TaskFactory()
{
}

TaskInterface *TaskFactory::create(TaskConfig &taskConfig, shared_ptr<ObjectsManager> objectsManager)
{
  TaskInterface *result = nullptr;
  QcInfoLogger &logger = QcInfoLogger::GetInstance();

  // Load the library
  string library = "lib" + taskConfig.moduleName + ".so";
  logger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
  if (gSystem->Load(library.c_str())) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
  }

  // Get the class and instantiate
  logger << "Loading class " << taskConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
  TClass *cl = TClass::GetClass(taskConfig.className.c_str());
  string tempString("Failed to instantiate Quality Control Module");
  if (!cl) {
    tempString += " because no dictionary for class named \"";
    tempString += taskConfig.className;
    tempString += "\" could be retrieved";
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  logger << "Instantiating class " << taskConfig.className << " (" << cl << ")"
         << AliceO2::InfoLogger::InfoLogger::endm;
  result = static_cast<TaskInterface *>(cl->New());
  if (!result) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  result->setName(taskConfig.taskName);
  result->setObjectsManager(objectsManager);
  logger << "QualityControl Module " << taskConfig.moduleName << " loaded " << AliceO2::InfoLogger::InfoLogger::endm;

  return result;
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
