/// \file CruFifoTable.h
/// \brief Definition of the CruFifoTable struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstdint>
#include <array>
#include "Utilities/Util.h"

namespace AliceO2 {
namespace Rorc {

static constexpr size_t CRU_DESCRIPTOR_ENTRIES = 128l;

/// A class representing the CRU status and descriptor tables
/// This class is meant to be used as an aliased type, reinterpret_casted from a raw memory pointer
/// Since this is an aggregate type, it does not violate the strict aliasing rule.
/// For more information, see:
///   http://en.cppreference.com/w/cpp/language/reinterpret_cast
///   http://en.cppreference.com/w/cpp/language/aggregate_initialization
/// For more info about the workings of this class, see chapter 6 of the Arria 10 Avalon-MM DMA User Guide
struct CruFifoTable
{
    void resetStatusEntries()
    {
      for (auto& s : statusEntries) {
        s.reset();
      }
    }

    /// Set the registers of a descriptor entry
    /// \param index FIFO index
    /// \param pageLength Size of the page in 32-bit words
    /// \param sourceAddress Page source address in device memory space
    /// \param destinationAddress Page destination address in device memory space
    void setDescriptor(uint32_t index, uint32_t pageLength, volatile void* sourceAddress,
        volatile void* destinationAddress)
    {
      auto& e = descriptorEntries[index];
      e.setControlRegister(index, pageLength);
      e.setSourceAddress(sourceAddress);
      e.setDestinationAddress(destinationAddress);
      e.setReserved();
    }

    /// A class representing a CRU status table entry
    struct StatusEntry
    {
        void reset()
        {
          status = 0;
        }

        bool isPageArrived()
        {
          return status == 1;
        }

        volatile uint32_t status;
    };

    /// A class representing a CRU descriptor table entry
    struct DescriptorEntry
    {

        /// Set the control register
        /// \param index Page index
        /// \param pageLength Size of the page in 32-bit words
        void setControlRegister(uint32_t index, uint32_t pageLength)
        {
#ifndef NDEBUG
          uint32_t MAX_INDEX = 128;
          uint32_t MAX_LENGTH = 8 * 1024 / 4; // Firmware limit of 8 KiB

          if (index > MAX_INDEX) {
            throw std::out_of_range("Page index too high");
          }

          if (pageLength > MAX_LENGTH) {
            throw std::out_of_range("Page length too high");
          }
#endif
          ctrl = (index << 18) + pageLength;
        }

        /// Set the source address registers
        /// \param address Page address in device memory space
        void setSourceAddress(volatile void* address)
        {
          srcHigh = Utilities::getUpper32Bits(uint64_t(address));
          srcLow = Utilities::getLower32Bits(uint64_t(address));
        }

        /// Set the source address registers
        /// \param address Page address in user memory space
        void setDestinationAddress(volatile void* address)
        {
          dstHigh = Utilities::getUpper32Bits(uint64_t(address));
          dstLow = Utilities::getLower32Bits(uint64_t(address));
        }

        void setReserved()
        {
          // Doesn't seem to be needed
//          reserved1 = 0x0;
//          reserved2 = 0x0;
//          reserved3 = 0x0;
        }

        /// Low 32 bits of the DMA source address on the card
        volatile uint32_t srcLow;

        /// High 32 bits of the DMA source address on the card
        volatile uint32_t srcHigh;

        /// Low 32 bits of the DMA destination address on the bus
        volatile uint32_t dstLow;

        /// High 32 bits of the DMA destination address on the bus
        volatile uint32_t dstHigh;

        /// Control register
        volatile uint32_t ctrl;

        /// Reserved field 1
        volatile uint32_t reserved1;

        /// Reserved field 2
        volatile uint32_t reserved2;

        /// Reserved field 3
        volatile uint32_t reserved3;
    };

    /// Array of status entries
    std::array<StatusEntry, CRU_DESCRIPTOR_ENTRIES> statusEntries;

    /// Array of descriptor entries
    std::array<DescriptorEntry, CRU_DESCRIPTOR_ENTRIES> descriptorEntries;
};

static_assert(sizeof(CruFifoTable::StatusEntry) == (1 * 4), "Size of CruFifoTable::StatusEntry invalid");

static_assert(sizeof(CruFifoTable::DescriptorEntry) == (8 * 4), "Size of CruFifoTable::DescriptorEntry invalid");

static_assert(sizeof(CruFifoTable) == 0x1200, "Size of CruFifoTable invalid");

static_assert(CRU_DESCRIPTOR_ENTRIES * sizeof(CruFifoTable::StatusEntry) == 0x200,
    "Size of CruFifoTable::statusEntries invalid");

static_assert(sizeof(CruFifoTable)
    == (CRU_DESCRIPTOR_ENTRIES * sizeof(CruFifoTable::StatusEntry))
     + (CRU_DESCRIPTOR_ENTRIES * sizeof(CruFifoTable::DescriptorEntry)),
     "Size of CruFifoTable invalid");

} // namespace Rorc
} // namespace AliceO2
