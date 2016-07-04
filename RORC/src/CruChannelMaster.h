///
/// \file CruChannelMaster.h
/// \author Pascal Boeschoten
///

#pragma once

#include "ChannelMaster.h"
#include <array>

namespace AliceO2 {
namespace Rorc {

/// Extends ChannelMaster object, and provides device-specific functionality
class CruChannelMaster : public ChannelMaster
{
  public:

    CruChannelMaster(int serial, int channel, const ChannelParameters& params);
    ~CruChannelMaster();
    virtual void resetCard(ResetLevel::type resetLevel);
    virtual PageHandle pushNextPage();
    virtual bool isPageArrived(const PageHandle& handle);

  protected:

    virtual void deviceStartDma();
    virtual void deviceStopDma();

    /// Name for the CRU shared data object in the shared state file
    inline static const char* crorcSharedDataName()
    {
      return "CruChannelMasterSharedData";
    }

  private:

    struct CruFifoTable {
      static constexpr size_t CRU_DESCRIPTOR_ENTRIES = 128l;

      struct StatusEntry {
          volatile uint32_t status;
      };

      struct DescriptorEntry {
        uint32_t srcLow;
        uint32_t srcHigh;
        uint32_t dstLow;
        uint32_t dstHigh;
        uint32_t ctrl;
        uint32_t reserved1;
        uint32_t reserved2;
        uint32_t reserved3;
      };

      std::array<StatusEntry, CRU_DESCRIPTOR_ENTRIES> statusEntries;
      std::array<DescriptorEntry, CRU_DESCRIPTOR_ENTRIES> descriptorEntries;
    };

    /// Persistent device state/data that resides in shared memory
    class CrorcSharedData
    {
      public:
        CrorcSharedData();
        void initialize();
        int fifoIndexWrite; /// Index of next page available for writing
        int fifoIndexRead; /// Index of oldest non-free page
        int pageIndex; /// Index to the next free page of the DMA buffer
        long long int loopPerUsec; // Some timing parameter used during communications with the card
        double pciLoopPerUsec; // Some timing parameters used during communications with the card
        InitializationState::type initializationState;
    };

    /// Memory mapped data stored in the shared state file
    FileSharedObject::FileSharedObject<CrorcSharedData> crorcSharedData;

    /// Check if data has arrived
    DataArrivalStatus::type dataArrived(int index);

    /// Memory mapped file containing the readyFifo
    TypedMemoryMappedFile<CruFifoTable> mappedFileFifo;

    /// PDA DMABuffer object for the readyFifo
    PdaDmaBuffer bufferFifo;

};

} // namespace Rorc
} // namespace AliceO2