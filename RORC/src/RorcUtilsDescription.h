///
/// \file RorcUtilsDescription.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

namespace AliceO2 {
namespace Rorc {
namespace Util {

/// Contains some descriptive text about a RORC utility executable
struct UtilsDescription
{
    inline UtilsDescription(const std::string& name, const std::string& description, const std::string& usage)
        : name(name), description(description), usage(usage)
    {
    }

    /// Name of the utility
    const std::string name;

    /// Short description
    const std::string description;

    /// Usage example
    const std::string usage;
};

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
