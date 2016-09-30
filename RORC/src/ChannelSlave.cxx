/// \file ChannelSlave.cxx
/// \brief Implementation of the ChannelSlave class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelSlave.h"
#include <iostream>

namespace AliceO2 {
namespace Rorc {

ChannelSlave::ChannelSlave(int serial, int channel)
 : mSerialNumber(serial),
   mChannelNumber(channel),
   mRorcDevice(serial),
   mPdaBar(mRorcDevice.getPciDevice(), channel)
{
}

ChannelSlave::~ChannelSlave()
{
}

uint32_t ChannelSlave::readRegister(int index)
{
  // TODO Range check
  // TODO Access restriction
  return mPdaBar[index];
}

void ChannelSlave::writeRegister(int index, uint32_t value)
{
  // TODO Range check
  // TODO Access restriction
  mPdaBar[index] = value;
}

} // namespace Rorc
} // namespace AliceO2
