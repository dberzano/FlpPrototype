///
/// \file    DataBlockProducer.cxx
/// \author  Barthelemy von Haller
///

#include "DataFormat/DataBlock.h"
#include "DataSampling/DataBlockProducer.h"
#include <fstream>

using namespace std;

namespace AliceO2 {
namespace DataSampling {

DataBlockProducer::DataBlockProducer(bool random, uint32_t payloadSize, bool isOwner) : mCurrent(nullptr),
                                                                                        mCurrentPayloadSize(
                                                                                          payloadSize),
                                                                                        mIsRandom(random),
                                                                                        mCurrentId(0),
                                                                                        mIsOwner(isOwner)
{
  if(mIsOwner) {
    regenerate();
  }
}

DataBlockProducer::~DataBlockProducer()
{
  if(mIsOwner) {
    delete[] mCurrent->data;
    delete mCurrent;
  }
}

/// Ownership remains with DataBlockProducer.
DataBlock *DataBlockProducer::get() const
{
  return mCurrent;
}

/// Note : the file is opened and closed each time.
void DataBlockProducer::saveToFile(std::string pathToFile, bool append) const
{
  std::ios_base::openmode extra_mode = append ? std::ios::app : std::ios::trunc;
  std::fstream file(pathToFile, std::ios::out | std::ios::binary | extra_mode);
  file.write((char *) &mCurrent->header, sizeof(DataBlockHeaderBase));
  file.write(mCurrent->data, mCurrentPayloadSize);
  file.close();
}

void DataBlockProducer::regenerate()
{
  // delete current data if we are the owner
  if (mIsOwner) {
    if (mCurrent) {
      if (mCurrent->data) {
        delete[] mCurrent->data;
      }
      delete mCurrent;
      mCurrent = nullptr;
    }
  }

  // generate payload size if needed
  if (mIsRandom) {
    std::normal_distribution<float> distribution(1024*1024, 256);
    double temp = distribution(mGenerator);
    temp = (temp < 1) ? 1 : temp; // we don't want to cast a negative number to uint
    mCurrentPayloadSize = (uint32_t) temp;
  }

  // create data block
  mCurrent = new DataBlock();
  mCurrent->header.blockType = 0xba; // whatever
  mCurrent->header.headerSize = 0x60; // just the header base -> 96 bits
  mCurrent->header.dataSize = mCurrentPayloadSize * 8;
  mCurrent->header.id = mCurrentId++;
  char *buffer = new char[mCurrentPayloadSize];
  for (unsigned int i = 0; i < mCurrentPayloadSize; i++) {
    if (mIsRandom) {
      buffer[i] = (char) ((rand() % 26) + 'a');
    } else {
      buffer[i] = (char) ((i % 26) + 'a');
    }
  }
  mCurrent->data = buffer;
}

}
}
