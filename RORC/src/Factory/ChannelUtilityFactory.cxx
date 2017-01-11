/// \file ChannelUtilityFactory.cxx
/// \brief Implementation of the ChannelUtilityFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Factory/ChannelUtilityFactory.h"
#include <map>
#include "RORC/CardType.h"
#include "Dummy/DummyChannelMaster.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#  include "Crorc/CrorcChannelMaster.h"
#  include "Cru/CruChannelMaster.h"
#else
#  pragma message("PDA not enabled, ChannelUtilityFactory will always return a dummy implementation")
#endif
#include "Factory/ChannelFactoryUtils.h"

namespace AliceO2 {
namespace ReadoutCard {

using namespace FactoryHelper;
using namespace CardTypeTag;

ChannelUtilityFactory::ChannelUtilityFactory()
{
}

ChannelUtilityFactory::~ChannelUtilityFactory()
{
}

auto ChannelUtilityFactory::getUtility(const Parameters& params) -> UtilitySharedPtr
{
  return makeChannel<ChannelUtilityInterface>(params, DUMMY_SERIAL_NUMBER
    , DummyTag, [&]{ return std::make_shared<DummyChannelMaster>(params); }
#ifdef ALICEO2_RORC_PDA_ENABLED
    , CrorcTag, [&]{ return std::make_shared<CrorcChannelMaster>(params); }
    , CruTag,   [&]{ return std::make_shared<CruChannelMaster>(params); }
#endif
    );
}

} // namespace ReadoutCard
} // namespace AliceO2
