/// \file ResetLevel.cxx
/// \brief Implementation of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ParameterTypes/ResetLevel.h"
#include "Utilities/Enum.h"

namespace AliceO2 {
namespace roc {
namespace {

static const auto converter = Utilities::makeEnumConverter<ResetLevel::type>({
  { ResetLevel::Nothing,        "NOTHING" },
  { ResetLevel::Internal,       "INTERNAL" },
  { ResetLevel::InternalDiu,    "INTERNAL_DIU" },
  { ResetLevel::InternalDiuSiu, "INTERNAL_DIU_SIU" },
});

} // Anonymous namespace

bool ResetLevel::includesExternal(const ResetLevel::type& mode)
{
  return mode == ResetLevel::InternalDiu || mode == ResetLevel::InternalDiuSiu;
}

std::string ResetLevel::toString(const ResetLevel::type& level)
{
  return converter.toString(level);
}

ResetLevel::type ResetLevel::fromString(const std::string& string)
{
  return converter.fromString(string);
}

} // namespace roc
} // namespace AliceO2

