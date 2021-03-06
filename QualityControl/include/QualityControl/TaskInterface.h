///
/// \file   TaskInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QCTASK_H_
#define QUALITYCONTROL_LIBS_CORE_QCTASK_H_

#include "DataFormat/DataBlockContainer.h"
#include "QualityControl/ObjectsManager.h"
#include <memory>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

/// \brief Dummy class that should be removed when there is the official one.
/// \author   Barthelemy von Haller
class Activity
{
  public:
    /// Default constructor
    Activity() : mId(0), mType(0)
    {}

    Activity(int id, int type) : mId(id), mType(type)
    {}

    /// Destructor
    virtual ~Activity()
    {}

    int mId;
    int mType;
};

/// \brief  Skeleton of a QC task.
///
/// Purely abstract class defining the skeleton and the common interface of a QC task.
/// It is therefore the parent class of any QC task.
/// It is responsible for the instantiation, modification and destruction of the TObjects that are published.
///
/// It is part of the template method design pattern.
///
/// \author Barthelemy von Haller
class TaskInterface
{
  public:
    /// \brief Constructor
    /// Can't be used when dynamically loading the class with ROOT.
    /// @param name
    /// @param objectsManager
    TaskInterface(ObjectsManager *objectsManager);

    /// \brief Default constructor
    /// Remember to set an objectsManager.
    TaskInterface();

    virtual ~TaskInterface();

    // Definition of the methods for the template method pattern
    virtual void initialize() = 0;
    virtual void startOfActivity(Activity &activity) = 0;
    virtual void startOfCycle() = 0;
    //    virtual void monitorDataBlock(vector<pair<DataHeader*, char*>>& datablock) = 0;
    virtual void monitorDataBlock(std::vector<std::shared_ptr<DataBlockContainer>> &block) = 0;
    virtual void endOfCycle() = 0;
    virtual void endOfActivity(Activity &activity) = 0;
    virtual void reset() = 0;

    // Setters and getters
    void setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager);
    void setName(const std::string &name);
    const std::string &getName() const;

  protected:
    std::shared_ptr<ObjectsManager> getObjectsManager();

  private:
    std::shared_ptr<ObjectsManager> mObjectsManager; // TODO should we rather have a global/singleton for the objectsManager ?
    std::string mName;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_QCTASK_H_
