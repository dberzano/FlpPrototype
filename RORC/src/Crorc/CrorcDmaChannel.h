/// \file CrorcDmaChannel.h
/// \brief Definition of the CrorcDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <mutex>
#include <unordered_map>
#include <boost/circular_buffer_fwd.hpp>
#include <boost/scoped_ptr.hpp>
#include "DmaChannelPdaBase.h"
#include "Crorc.h"
#include "ReadoutCard/Parameters.h"
#include "ReadyFifo.h"
#include "SuperpageQueue.h"

namespace AliceO2 {
namespace roc {

/// Extends DmaChannel object, and provides device-specific functionality
/// Note: the functions prefixed with "crorc" are translated from the functions of the C interface (src/c/rorc/...")
class CrorcDmaChannel final : public DmaChannelPdaBase
{
  public:

    CrorcDmaChannel(const Parameters& parameters);
    virtual ~CrorcDmaChannel() override;

    virtual CardType::type getCardType() override;

    virtual bool injectError() override
    {
        return false;
    }
    virtual boost::optional<int32_t> getSerial() override;
    virtual boost::optional<std::string> getFirmwareInfo() override;

    virtual void pushSuperpage(Superpage superpage) override;

    virtual int getTransferQueueAvailable() override;
    virtual int getReadyQueueSize() override;

    virtual Superpage getSuperpage() override;
    virtual Superpage popSuperpage() override;
    virtual void fillSuperpages() override;

    AllowedChannels allowedChannels();

  protected:

    virtual void deviceStartDma() override;
    virtual void deviceStopDma() override;
    virtual void deviceResetChannel(ResetLevel::type resetLevel) override;

  private:
    /// Limits the number of superpages allowed in the queue
    static constexpr size_t MAX_SUPERPAGES = 32;

    /// Firmware FIFO Size
    static constexpr size_t FIFO_QUEUE_MAX = READYFIFO_ENTRIES;

    using SuperpageQueueType = SuperpageQueue<MAX_SUPERPAGES>;
    using SuperpageQueueEntry = SuperpageQueueType::SuperpageQueueEntry;

    /// Namespace for enum describing the status of a page's arrival
    struct DataArrivalStatus
    {
        enum type
        {
          NoneArrived,
          PartArrived,
          WholeArrived,
        };
    };

    uintptr_t getNextSuperpageBusAddress(const SuperpageQueueEntry& superpage);

    /// C-RORC function helper
    Crorc::Crorc getCrorc()
    {
      return {mPdaBar};
    }

    ReadyFifo* getReadyFifoUser()
    {
      return reinterpret_cast<ReadyFifo*>(mReadyFifoAddressUser);
    }

    ReadyFifo* getReadyFifoBus()
    {
      return reinterpret_cast<ReadyFifo*>(mReadyFifoAddressBus);
    }

    /// Enables data receiving in the RORC
    void startDataReceiving();

    /// Initializes and starts the data generator
    void startDataGenerator();

    /// Pushes a page to the CRORC's Free FIFO
    /// \param readyFifoIndex Index of the Ready FIFO to write the page's transfer status to
    /// \param pageBusAddress Address on the bus to push the page to
    void pushFreeFifoPage(int readyFifoIndex, uintptr_t pageBusAddress);

    /// Check if data has arrived
    DataArrivalStatus::type dataArrived(int index);

    /// Starts pending DMA with given superpage for the initial pages
    void startPendingDma(SuperpageQueueEntry& superpage);

    /// Push a page into a superpage
    void pushIntoSuperpage(SuperpageQueueEntry& superpage);

    /// Get front index of FIFO
    int getFifoFront() const
    {
      return (mFifoBack + mFifoSize) % READYFIFO_ENTRIES;
    };

    /// BAR 0 is needed for DMA engine interaction and various other functions
    Pda::PdaBar mPdaBar;

    /// BAR 2 is needed for configuration
    Pda::PdaBar mPdaBar2;

    /// Memory mapped file for the ReadyFIFO
    boost::scoped_ptr<MemoryMappedFile> mBufferFifoFile;

    /// PDA DMABuffer object for the ReadyFIFO
    boost::scoped_ptr<Pda::PdaDmaBuffer> mPdaDmaBufferFifo;

    /// Userspace address of FIFO in DMA buffer
    uintptr_t mReadyFifoAddressUser;

    /// Bus address of FIFO in DMA buffer
    uintptr_t mReadyFifoAddressBus;

    /// Back index of the firmware FIFO
    int mFifoBack = 0;

    /// Amount of elements in the firmware FIFO
    int mFifoSize = 0;

    /// Queue for superpages
    SuperpageQueueType mSuperpageQueue;

    /// Address of DMA buffer in userspace
    uintptr_t mDmaBufferUserspace = 0;

    /// Indicates deviceStartDma() was called, but DMA was not actually started yet. We do this because we need a
    /// superpage to actually start.
    bool mPendingDmaStart = false;


    // These variables are configuration parameters

    /// DMA page size
    const size_t mPageSize;

    /// Reset level on initialization of channel
    const ResetLevel::type mInitialResetLevel;

    /// Prevents sending the RDYRX and EOBTR commands. TODO This switch is implicitly set when data generator or the
    /// STBRD command is used (???)
    const bool mNoRDYRX;

    /// Enforces that the data reading is carried out with the Start Block Read (STBRD) command
    /// XXX Not sure if this should be a parameter...
    const bool mUseFeeAddress;

    /// Gives the type of loopback
    const LoopbackMode::type mLoopbackMode;

    /// Enables the data generator
    const bool mGeneratorEnabled;

    /// Data pattern for the data generator
    const GeneratorPattern::type mGeneratorPattern;

    /// Maximum number of events
    const int mGeneratorMaximumEvents;

    /// Initial value of the first data in a data block
    const uint32_t mGeneratorInitialValue;

    /// Sets the second word of each fragment when the data generator is used
    const uint32_t mGeneratorInitialWord;

    /// Random seed parameter in case the data generator is set to produce random data
    const int mGeneratorSeed;

    /// Length of data written to each page
    const size_t mGeneratorDataSize;

    /// Use continuous readout mode
    const bool mUseContinuousReadout;

    Crorc::Crorc::DiuConfig mDiuConfig;
};

} // namespace roc
} // namespace AliceO2
