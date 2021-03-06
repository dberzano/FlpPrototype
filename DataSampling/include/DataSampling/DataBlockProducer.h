///
/// \file    DataBlockProducer.h
/// \author  Barthelemy von Haller
///

#ifndef DATA_SAMPLING_DATABLOCKPRODUCER_H
#define DATA_SAMPLING_DATABLOCKPRODUCER_H

#include <DataFormat/DataBlock.h>

#include <iostream>
#include <random>

namespace AliceO2 {
namespace DataSampling {

/// \brief This class produces DataBlocks containing random data.
///
/// It can return the block in memory or save it to file. In the latter case, it can
/// append or truncate the file. The content of the payload is just the alphabet.
/// If the flag random is set to true in the constructor, the payload size follows a normal distribution
/// around 1024 bytes with a standard deviation of 256.
/// Usage :
/// \code
///  DataBlockProducer producer(false, payloadSize); // creates a first block
///  producer.get(); // get a the block but it is const. Copy if needed.
///  producer.regenerate(); // generate a new block
///  producer.saveToFile(path); // save the current block to a file
/// \endcode
/// \todo move to DataFormat package
///
/// \author Barthelemy von Haller
class DataBlockProducer
{
  public:
    /// \brief Create a data block producer and an initial data block.
    DataBlockProducer(bool random = true, uint32_t payloadSize = 1024 /*bytes*/, bool isOwner = true);
    /// \brief Destructor
    virtual ~DataBlockProducer();

    /// \brief Get a copy of the latest data block generated.
    DataBlock* get() const;
    /// \brief Save the current data block to the file.
    void saveToFile(std::string pathToFile, bool append = true) const;
    /// \brief Destroy the current data block and create a new one.
    void regenerate();

  private:
    DataBlock *mCurrent;
    uint32_t mCurrentPayloadSize;
    bool mIsRandom;
    std::default_random_engine mGenerator;
    uint32_t mCurrentId;
    bool mIsOwner;
};

}
}

#endif //DATA_SAMPLING_DATABLOCKPRODUCER_H
