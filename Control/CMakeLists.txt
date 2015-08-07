# INFOLOGGER CMAKELISTS.TXT
#
# @author  sylvain.chapeland@cern.ch


cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")


# Set the name of your project here
project("Control")

# Set the version number of your project here (format is MAJOR.MINOR.PATCHLEVEL - e.g. 1.0.0)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

## Git (and its revision)
#find_package(Git)
## GetGitRevisionDescription module to retreive branch and revision information from Git
## Starting with Git 1.9 the module will be part of official cMake distribution, until then it has to be
## part of the application
## The Git module will trigger a reconfiguration for each pull that will bring a new revision on the local repository
#set (VCS_REVISION "-1")
#if(GIT_FOUND)
#    include(GetGitRevisionDescription)
#    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
#    message(STATUS "GIT branch ${GIT_REFSPEC}")
#    message(STATUS "GIT revision ${GIT_SHA1}")
#    set (VCS_REVISION ${GIT_SHA1})
#endif()

# C++11 needed
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-std=c++11 COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


include_directories(
)
file(
  GLOB_RECURSE
  source_files
  src/*
  test/*
)

include_directories (
  include
  src
)

# documentation

include(FindDoxygen)

# Add an option for the user to enable or not the documentation generation. 
# Either set it on the command line or use ccmake. It is an advanced option.
option(BUILD_DOCS "Build doxygen documentation" ON)
mark_as_advanced(BUILD_DOCS)

if(NOT DOXYGEN_DOT_FOUND) 
  message(WARNING "Graphviz doesn't seem to be installed. Doxygen will not be able to generate graphs. Consider installing this package.")
endif(NOT DOXYGEN_DOT_FOUND) 

if(BUILD_DOCS AND DOXYGEN_FOUND)
  # Configure the doxygen config file with current settings:
  configure_file(documentation-config.doxygen.in
                 ${CMAKE_CURRENT_BINARY_DIR}/documentation-config.doxygen @ONLY)
  add_custom_target(doxygen ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/documentation-config.doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation using doxygen" VERBATIM)

endif(BUILD_DOCS AND DOXYGEN_FOUND)

set(ZOOKEEPERPATH "/opt/zookeeper-3.4.6")

INCLUDE_DIRECTORIES(${ZOOKEEPERPATH}/src/c/include ${ZOOKEEPERPATH}/src/c/generated)
LINK_DIRECTORIES(${ZOOKEEPERPATH}/src/c/.libs/)

set(BOOSTPATH "/opt/boost_1_58_0/")

INCLUDE_DIRECTORIES(${BOOSTPATH})
LINK_DIRECTORIES(${BOOSTPATH}/stage/lib)

add_executable (
  zdaq_test
  src/testMain.cpp
)

add_library (
  zdaq
  src/zdaq.cpp
  src/zdaq_ctrl.cpp
)

target_link_libraries (
  zdaq_test
  zdaq
  pthread
  zookeeper_mt
  zmq
)

add_executable (
  testStateMachine
  test/testStateMachine.cxx
  src/StateMachine.cxx
)

target_link_libraries (
  testStateMachine
  zookeeper_mt
  boost_system
)