///
/// \file   ExampleTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_EXAMPLETASK_H
#define QC_MODULE_EXAMPLE_EXAMPLETASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControlModules {
namespace Example {

/// \brief Example Quality Control Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
class ExampleTask /*final*/: public TaskInterface // todo add back the "final" when doxygen is fixed
{
  public:
    /// \brief Constructor
    ExampleTask();
    /// Destructor
    ~ExampleTask() override;

    // Definition of the methods for the template method pattern
    void initialize() override;
    void startOfActivity(Activity &activity) override;
    void startOfCycle() override;
    void monitorDataBlock(std::vector<std::shared_ptr<DataBlockContainer>> &block) override;
    void endOfCycle() override;
    void endOfActivity(Activity &activity) override;
    void reset() override;

    // Accessors
    TH1F*& getHisto1()
    {
      return mPayloadSize;
    }
    TH1F*& getHisto2()
    {
      return mHisto2;
    }

  private:

    TH1F *mPayloadSize, *mHisto2;
    TH1F *mNumberSubblocks;
    TH1F *mSubPayloadSize;
    TH1F *mHistos[25];
};

}
}
}

#endif //QC_MODULE_EXAMPLE_EXAMPLETASK_H
