/// \file Common.h
/// \brief Definition of common functions for the ReadoutCard utilities.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <boost/program_options.hpp>

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Common {

/// Create a string showing individual bits of the given 32-bit value
std::string make32bitString(uint32_t bits);

/// Create a string showing the given 32-bit value in hexadecimal format
std::string make32hexString(uint32_t bits);

/// Create a string representation of a register address
std::string makeRegisterAddressString(int address);

/// Create a string representation of a register address and its value
std::string makeRegisterString(int address, uint32_t value);

} // namespace Common
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
