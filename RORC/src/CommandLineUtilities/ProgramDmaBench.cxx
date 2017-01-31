/// \file ProgramDmaBench.cxx
///
/// \brief Utility that tests RORC DMA performance
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <queue>
#include <thread>
#include <random>
#include <boost/format.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include "Cru/CruRegisterIndex.h"
#include "ExceptionLogging.h"
#include "InfoLogger/InfoLogger.hxx"
#include "RORC/ChannelFactory.h"
#include "RORC/Parameters.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Thread.h"
#include "Utilities/Util.h"

using namespace AliceO2::Rorc::CommandLineUtilities;
using namespace AliceO2::Rorc;
using namespace AliceO2::InfoLogger;
using std::cout;
using std::endl;
using namespace std::literals;
namespace b = boost;
namespace po = boost::program_options;
namespace bfs = boost::filesystem;

using Page = ChannelMasterInterface::Page;
using PageSharedPtr = ChannelMasterInterface::PageSharedPtr;
using MasterSharedPtr = ChannelMasterInterface::MasterSharedPtr;

namespace {
constexpr auto endm = InfoLogger::StreamOps::endm;

/// Determines how often the status display refreshes
constexpr auto DISPLAY_INTERVAL = 10ms;

/// Max amount of errors that are recorded into the error stream
constexpr int64_t MAX_RECORDED_ERRORS = 1000;

/// Minimum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MIN = 10;
/// Maximum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MAX = 2000;
/// Minimum random pause in milliseconds
constexpr int PAUSE_LENGTH_MIN = 1;
/// Maximum random pause in milliseconds
constexpr int PAUSE_LENGTH_MAX = 500;
// Low priority counter interval
constexpr int LOW_PRIORITY_INTERVAL = 10000;

constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;

/// The data emulator writes to every 8th 32-bit word
constexpr uint32_t PATTERN_STRIDE = 8;

/// Fields: Time(hour:minute:second), Pages, Errors, °C
const std::string PROGRESS_FORMAT_HEADER("  %-8s   %-12s  %-12s  %-10.1f");

/// Fields: Time(hour:minute:second), Errors, °C
const std::string PROGRESS_FORMAT("  %02s:%02s:%02s   %-12s  %-12s  %-10.1f");

auto READOUT_ERRORS_PATH = "readout_errors.txt";
auto READOUT_DATA_PATH_ASCII = "readout_data.txt";
auto READOUT_DATA_PATH_BIN = "readout_data.bin";
auto READOUT_LOG_FORMAT = "readout_log_%d.txt";

}

class BarHammer : public Utilities::Thread
{
  public:
    void start(const std::shared_ptr<ChannelMasterInterface>& channelIn)
    {
      mChannel = channelIn;
      Thread::start([&](std::atomic<bool>* stopFlag) {
        auto channel = mChannel;

        if (channel->getCardType() != CardType::Cru) {
          cout << "BarHammer only supported for CRU\n";
          return;
        }

        if (!channel) {
          return;
        }

        int64_t hammerCount = 0;
        uint32_t writeCounter = 0;
        while (!stopFlag->load() && !Program::isSigInt()) {
          for (int i = 0; i < getMultiplier(); ++i) {
            channel->writeRegister(CruRegisterIndex::DEBUG_READ_WRITE, writeCounter);
            writeCounter++;
          }
          hammerCount++;
        }
        mHammerCount = hammerCount;
      });
    }

    double getCount()
    {
      return double(mHammerCount.load()) * double(getMultiplier());
    }

  private:
    std::shared_ptr<ChannelMasterInterface> mChannel;
    std::atomic<int64_t> mHammerCount;

    int64_t getMultiplier()
    {
      return 10000;
    }
};

class ProgramDmaBench: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"DMA Benchmark", "Test RORC DMA performance", "./rorc-dma-bench --id=12345 --channel=0"};
    }

    virtual void addOptions(po::options_description& options)
    {
      Options::addOptionChannel(options);
      Options::addOptionCardId(options);
      Options::addOptionsChannelParameters(options);
      options.add_options()
          ("reset",
              po::bool_switch(&mOptions.resetChannel),
              "Reset channel during initialization")
          ("to-file-ascii",
              po::bool_switch(&mOptions.fileOutputAscii),
              "Read out to file in ASCII format")
          ("to-file-bin",
              po::bool_switch(&mOptions.fileOutputBin),
              "Read out to file in binary format (only contains raw data from pages)")
          ("pages",
              po::value<int64_t>(&mOptions.maxPages)->default_value(1500),
              "Amount of pages to transfer. Give <= 0 for infinite.")
          ("rand-pause-sw",
              po::bool_switch(&mOptions.randomPauseSoft),
              "Randomly pause readout using software method")
          ("rand-readout",
              po::bool_switch(&mOptions.randomReadout),
              "Readout in non-sequential order")
          ("no-errorcheck",
              po::bool_switch(&mOptions.noErrorCheck),
              "Skip error checking")
          ("no-pagereset",
              po::bool_switch(&mOptions.noPageReset),
              "Do not reset page to default values")
          ("bar-hammer",
              po::bool_switch(&mOptions.barHammer),
              "Stress the BAR with repeated writes and measure performance")
              ;
    }

    virtual void run(const po::variables_map& map)
    {
      if (mOptions.fileOutputAscii && mOptions.fileOutputBin) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("File output can't be both ASCII and binary"));
      }
      if (mOptions.fileOutputAscii) {
        mReadoutStream.open("readout_data.txt");
      }
      if (mOptions.fileOutputBin) {
        mReadoutStream.open("readout_data.bin", std::ios::binary);
      }

      mInfinitePages = (mOptions.maxPages <= 0);

      auto cardId = Options::getOptionCardId(map);
      int channelNumber = Options::getOptionChannel(map);
      auto params = Options::getOptionsParameterMap(map);
      mPageSize = params.getDmaPageSize().get();
      params.setCardId(cardId);
      params.setChannelNumber(channelNumber);
      params.setGeneratorDataSize(mPageSize);

      // Get master lock on channel
      mChannel = ChannelFactory().getMaster(params);

      if (mOptions.resetChannel) {
        cout << "Resetting channel...";
        mChannel->resetChannel(ResetLevel::Rorc);
        cout << " done!\n";
      }

      cout << "### Starting benchmark" << endl;

      mChannel->startDma();

      // Get started
      if (isVerbose()) {
        printStatusHeader();
      }

      if (mOptions.barHammer) {
        if (mChannel->getCardType() != CardType::Cru) {
          BOOST_THROW_EXCEPTION(ParameterException()
              << ErrorInfo::Message("BarHammer option currently only supported for CRU\n"));
        }
        Utilities::resetSmartPtr(mBarHammer);
        mBarHammer->start(mChannel);
      }

      mRunTime.start = std::chrono::high_resolution_clock::now();

      if (mOptions.randomReadout) {
        dmaLoopReadoutRandom();
      } else {
        dmaLoop();
      }
      mRunTime.end = std::chrono::high_resolution_clock::now();

      if (mBarHammer) {
        mBarHammer->join();
      }

      freeExcessPages(10ms);
      mChannel->stopDma();

      outputErrors();
      outputStats();

      cout << "### Benchmark complete" << endl;
      cout << "Pushed " << mPushCount << " pages" << endl;
    }

  private:
    void dmaLoop()
    {
      while (!mDmaLoopBreak) {
        // Check if we need to stop in the case of a page limit
        if (!mInfinitePages && mReadoutCount >= mOptions.maxPages) {
          mDmaLoopBreak = true;
          cout << "\n\nMaximum amount of pages reached\n";
          break;
        }

        // Note: these low priority tasks are not run on every cycle, to reduce overhead
        lowPriorityTasks();

        // Keep the readout queue filled
        mPushCount += mChannel->fillFifo();

        // Read out a page if available
        if (auto sharedPage = ChannelMasterInterface::popPage(mChannel)) {
          readoutPage(sharedPage);
          mReadoutCount++;
        }
      }
    }

    void dmaLoopReadoutRandom()
    {
      constexpr int READOUT_THRESHOLD = 200; ///< Amount of pages to "cache" before reading out randomly
      std::default_random_engine generator;
      std::vector<PageSharedPtr> pages;

      while (!mDmaLoopBreak) {
        // Check if we need to stop in the case of a page limit
        if (!mInfinitePages && mReadoutCount >= mOptions.maxPages) {
          mDmaLoopBreak = true;
          cout << "\n\nMaximum amount of pages reached\n";
          break;
        }

        // Note: these low priority tasks are not run on every cycle, to reduce overhead
        lowPriorityTasks();

        // Keep the readout queue filled
        mPushCount += mChannel->fillFifo();

        // Read out a page if available
        if (auto sharedPage = ChannelMasterInterface::popPage(mChannel)) {
          pages.push_back(sharedPage);
        }

        if (pages.size() > READOUT_THRESHOLD) {
          std::uniform_int_distribution<size_t> distribution(0, pages.size() - 1);
          size_t index = distribution(generator);

          readoutPage(pages[index]);

          pages.erase(pages.begin() + index);
          mReadoutCount++;
        }
      }
    }

    /// Free the pages that were pushed in excess
    void freeExcessPages(std::chrono::milliseconds timeout)
    {
      auto start = std::chrono::high_resolution_clock::now();
      int popped = 0;
      while ((std::chrono::high_resolution_clock::now() - start) < timeout) {
        if (auto page = ChannelMasterInterface::popPage(mChannel)) {
          popped++;
        }
      }
      cout << "Freed " << popped << " excess pages\n";
    }

    volatile uint32_t* pageData(Page& page)
    {
      return page.getAddressU32();
    }

    uint32_t getEventNumber(Page& page)
    {
      return pageData(page)[0] / 256;
    }

    void readoutPage(PageSharedPtr sharedPage)
    {
      auto& page = *sharedPage.get();

      // Read out to file
      if (mOptions.fileOutputAscii || mOptions.fileOutputBin) {
        printToFile(page, mReadoutCount);
      }

      // Data error checking
      if (!mOptions.noErrorCheck) {
        if (mDataGeneratorCount == -1) {
          // First page initializes the counter
          mDataGeneratorCount = getEventNumber(page);
        }

        bool hasError = checkErrors(getCurrentGeneratorPattern(), page, mReadoutCount, mDataGeneratorCount);
        if (hasError && mOptions.resyncCounter) {
          // Resync the counter
          mDataGeneratorCount = getEventNumber(page);
        }
      }

      // Setting the buffer to the default value after the readout
      if (!mOptions.noPageReset) {
        resetPage(page);
      }

      mDataGeneratorCount++;
    }

    GeneratorPattern::type getCurrentGeneratorPattern()
    {
      // Get first 2 bits of DMA configuration register, these contain the generator pattern
//      uint32_t conf = bar(CruRegisterIndex::DMA_CONFIGURATION) && 0b11;
      uint32_t conf = 0; // We're just gonna pretend it's always incremental right now
      return conf == 0 ? GeneratorPattern::Incremental
           : conf == 1 ? GeneratorPattern::Alternating
           : conf == 2 ? GeneratorPattern::Constant
           : GeneratorPattern::Unknown;
    }

    /// Checks and reports errors
    bool checkErrors(GeneratorPattern::type pattern, const Page& _page, int64_t eventNumber, uint32_t counter)
    {
      auto check = [&](auto patternFunction) {
        volatile uint32_t* page = _page.getAddressU32();
        auto pageSize32 = _page.getSize() / sizeof(int32_t);
        for (uint32_t i = 0; i < pageSize32; i += PATTERN_STRIDE)
        {
          uint32_t expectedValue = patternFunction(i);
          uint32_t actualValue = page[i];
          if (actualValue != expectedValue) {
            // Report error
            mErrorCount++;
            if (isVerbose() && mErrorCount < MAX_RECORDED_ERRORS) {
              mErrorStream << boost::format("event:%d i:%d exp:0x%x val:0x%x\n")
                  % eventNumber % i % expectedValue % actualValue;
            }
            return true;
          }
        }
        return false;
      };

      switch (pattern) {
        case GeneratorPattern::Incremental: return check([&](uint32_t i) { return counter * 256 + i / 8; });
        case GeneratorPattern::Alternating: return check([&](uint32_t)   { return 0xa5a5a5a5; });
        case GeneratorPattern::Constant:    return check([&](uint32_t)   { return 0x12345678; });
        default: ;
      }

      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("Unrecognized generator pattern")
          << ErrorInfo::GeneratorPattern(pattern));
    }

    void resetPage(Page& page)
    {
      auto pageSize32 = page.getSize() / sizeof(int32_t);
      for (size_t i = 0; i < pageSize32; i++) {
        pageData(page)[i] = BUFFER_DEFAULT_VALUE;
      }
    }

    void lowPriorityTasks()
    {
      // This stuff doesn't need to be run every cycle, so we reduce the overhead.
      if (mLowPriorityCount < LOW_PRIORITY_INTERVAL) {
        mLowPriorityCount++;
        return;
      }
      mLowPriorityCount = 0;

      // Handle a SIGINT abort
      if (isSigInt()) {
        // We want to finish the readout cleanly if possible, so we stop pushing and try to wait a bit until the
        // queue is empty
        cout << "\n\nInterrupted\n";
        mDmaLoopBreak = true;
        return;
      }

      // Status display updates
      if (isVerbose() && isStatusDisplayInterval()) {
        updateStatusDisplay();
      }

      // Random pauses in software: a thread sleep
      if (mOptions.randomPauseSoft) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now >= mRandomPausesSoft.next) {
          cout << b::format("sw pause %-4d ms\n") % mRandomPausesSoft.length.count() << std::flush;
          std::this_thread::sleep_for(mRandomPausesSoft.length);

          // Schedule next pause
          auto now = std::chrono::high_resolution_clock::now();
          mRandomPausesSoft.next = now + std::chrono::milliseconds(Utilities::getRandRange(NEXT_PAUSE_MIN, NEXT_PAUSE_MAX));
          mRandomPausesSoft.length = std::chrono::milliseconds(Utilities::getRandRange(PAUSE_LENGTH_MIN, PAUSE_LENGTH_MAX));
        }
      }
    }

    void updateStatusDisplay()
     {
       using namespace std::chrono;
       auto diff = high_resolution_clock::now() - mRunTime.start;
       auto second = duration_cast<seconds>(diff).count() % 60;
       auto minute = duration_cast<minutes>(diff).count() % 60;
       auto hour = duration_cast<hours>(diff).count();

       auto format = b::format(PROGRESS_FORMAT);
       format % hour % minute % second; // Time
       format % mReadoutCount; // Pages
       mOptions.noErrorCheck ? format % "n/a" : format % mErrorCount; // Errors
       format % "n/a"; // TODO Temperature
       cout << '\r' << format;

       // This takes care of adding a "line" to the stdout every so many seconds
       {
         int interval = 60;
         auto second = duration_cast<seconds>(diff).count() % interval;
         if (mDisplayUpdateNewline && second == 0) {
           cout << '\n';
           mDisplayUpdateNewline = false;
         }
         if (second >= 1) {
           mDisplayUpdateNewline = true;
         }
       }
     }

     void printStatusHeader()
     {
       auto line1 = b::format(PROGRESS_FORMAT_HEADER) % "Time" % "Pages" % "Errors" % "°C";
       auto line2 = b::format(PROGRESS_FORMAT) % "00" % "00" % "00" % '-' % '-' % '-';
       cout << '\n' << line1;
       cout << '\n' << line2;
     }

     bool isStatusDisplayInterval()
     {
       auto now = std::chrono::high_resolution_clock::now();
       if (std::chrono::duration_cast<std::chrono::milliseconds, int64_t>(now - mLastDisplayUpdate)
           > DISPLAY_INTERVAL) {
         mLastDisplayUpdate = now;
         return true;
       }
       return false;
     }

     void outputStats()
     {
       // Calculating throughput
       double runTime = std::chrono::duration<double>(mRunTime.end - mRunTime.start).count();
       double bytes = double(mReadoutCount) * mPageSize;
       double GB = bytes / (1000 * 1000 * 1000);
       double GBs = GB / runTime;
       double Gbs = GBs * 8;
       double GiB = bytes / (1024 * 1024 * 1024);
       double GiBs = GiB / runTime;
       double Gibs = GiBs * 8;

       auto put = [&](auto label, auto value) { cout << b::format("  %-10s  %-10s\n") % label % value; };
       cout << '\n';
       put("Seconds", runTime);
       put("Pages", mReadoutCount);
       if (bytes > 0.00001) {
         put("Bytes", bytes);
         put("GB", GB);
         put("GB/s", GBs);
         put("Gb/s", Gbs);
         put("GiB", GiB);
         put("GiB/s", GiBs);
         put("Gibit/s", Gibs);
         put("Errors", mErrorCount);
       }

       if (mOptions.barHammer) {
         size_t writeSize = sizeof(uint32_t);
         double hammerCount = mBarHammer->getCount();
         double bytes = hammerCount * writeSize;
         double MB = bytes / (1000 * 1000);
         double MBs = MB / runTime;
         put("BAR writes", hammerCount);
         put("BAR write size (bytes)", writeSize);
         put("BAR MB", MB);
         put("BAR MB/s", MBs);
       }

       cout << '\n';
     }

    void outputErrors()
    {
      auto errorStr = mErrorStream.str();

      if (isVerbose()) {
        size_t maxChars = 2000;
        if (!errorStr.empty()) {
          cout << "Errors:\n";
          cout << errorStr.substr(0, maxChars);
          if (errorStr.length() > maxChars) {
            cout << "\n... more follow (" << (errorStr.length() - maxChars) << " characters)\n";
          }
        }
      }

      std::ofstream stream(READOUT_ERRORS_PATH);
      stream << errorStr;
    }

    void printToFile(Page& handle, int64_t pageNumber)
    {
      auto page = handle.getAddressU32();
      auto pageSize = handle.getSize();
      auto pageSize32 = pageSize / sizeof(int32_t);

      if (mOptions.fileOutputAscii) {
        mReadoutStream << "Event #" << pageNumber << '\n';
        int perLine = 8;

        for (int i = 0; i < pageSize32; i += perLine) {
          for (int j = 0; j < perLine; ++j) {
            mReadoutStream << page[i + j] << ' ';
          }
          mReadoutStream << '\n';
        }
        mReadoutStream << '\n';
      } else if (mOptions.fileOutputBin) {
        // TODO Is there a more elegant way to write from volatile memory?
        mReadoutStream.write(reinterpret_cast<char*>(const_cast<uint32_t*>(page)), pageSize);
      }
    }

    std::shared_ptr<ChannelMasterInterface> mChannel;

    /// Program options
    struct _OptionsStruct {
        int64_t maxPages = 0; ///< Limit of pages to push
        bool fileOutputAscii = false;
        bool fileOutputBin = false;
        bool resetChannel = false;
        bool randomPauseSoft = false;
        bool noErrorCheck = false;
        bool noPageReset = false;
        bool resyncCounter = false;
        bool randomReadout = false;
        bool barHammer = false;
    } mOptions;

    bool mDmaLoopBreak = false;
    bool mInfinitePages = false;
    int64_t mPushCount = 0;
    int64_t mReadoutCount = 0;
    int64_t mErrorCount = 0;
    int64_t mDataGeneratorCount = -1;
    int mLowPriorityCount = 0;

    /// Stream for file readout, only opened if enabled by the --file program options
    std::ofstream mReadoutStream;

    /// Stream for error output
    std::ostringstream mErrorStream;

    using TimePoint = std::chrono::high_resolution_clock::time_point;

    struct RunTime
    {
        TimePoint start; ///< Start of run time
        TimePoint end; ///< End of run time
    } mRunTime;

    /// Time of the last display update
    TimePoint mLastDisplayUpdate;

    /// Indicates the display must add a newline to the table
    bool mDisplayUpdateNewline;

    /// Enables / disables the pushing loop
    bool mPushEnabled = true;

    struct RandomPausesSoft
    {
        TimePoint next; ///< Next pause at this time
        std::chrono::milliseconds length; ///< Next pause has this length
    } mRandomPausesSoft;

    size_t mPageSize;

    std::unique_ptr<BarHammer> mBarHammer;
};

int main(int argc, char** argv)
{
  return ProgramDmaBench().execute(argc, argv);
}