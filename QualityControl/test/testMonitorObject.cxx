///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/MonitorObject.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cassert>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(mo)
{
  AliceO2::QualityControl::Core::MonitorObject obj;
  BOOST_CHECK_EQUAL(obj.getName(), "");
  obj.setName("test");
  BOOST_CHECK_EQUAL(obj.getName(), "test");
  obj.setQuality(Quality::Medium);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Medium);

  obj.addCheck("first", "class1", "lib1");
  obj.addCheck("second", "class1", "lib1");
  obj.addCheck("third", "class2", "lib1");
  obj.addCheck("first", "class2", "lib1");
  auto checkers1 = obj.getChecks();
  BOOST_CHECK_EQUAL(checkers1[0].name, "first");
  BOOST_CHECK_EQUAL(checkers1[0].className, "class1");
  BOOST_CHECK_EQUAL(checkers1[0].libraryName, "lib1");
  BOOST_CHECK_EQUAL(checkers1[1].name, "second");
  BOOST_CHECK_EQUAL(checkers1[1].className, "class1");
  BOOST_CHECK_EQUAL(checkers1[1].libraryName, "lib1");
}

} /* namespace ObjectsManager */
} /* namespace QualityControl */
} /* namespace AliceO2 */
