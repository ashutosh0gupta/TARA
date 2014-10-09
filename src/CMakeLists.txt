cmake_minimum_required(VERSION 2.8.7)

project(TARA)

set(Boost_USE_STATIC_LIBS   OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
add_definitions(-DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK)

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.52.0 COMPONENTS regex program_options system filesystem REQUIRED) 

find_package (Threads)

SET (HB_UNICODE TRUE CACHE BOOL "Enable unicode output")
SET (Z3_DEBUG FALSE CACHE BOOL "Link debug verion of Z3")

IF(Z3_DEBUG)
  LINK_DIRECTORIES("${PROJECT_BINARY_DIR}/../z3/buildd")
ELSE(Z3_DEBUG)
  LINK_DIRECTORIES("${PROJECT_BINARY_DIR}/../z3/buildr")
ENDIF(Z3_DEBUG)

ADD_DEFINITIONS(
    -std=c++11 -Wall # Or -std=c++0x
    # Other flags
)

if (HB_UNICODE)
  add_definitions(-DHB_UNICODE)
ENDIF(HB_UNICODE)

include_directories (${CMAKE_CURRENT_BINARY_DIR} "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/library" "${PROJECT_BINARY_DIR}/../z3/src/api" "${PROJECT_BINARY_DIR}/../z3/src/api/c++")
include_directories(${Boost_INCLUDE_DIRS}) 
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

add_subdirectory("library")

add_executable(tara main.cpp options_cmd.cpp)

IF(WIN32)
  set(LIBZ3 libz3)
ELSE(WIN32)
  set(LIBZ3 z3)
ENDIF(WIN32)

target_link_libraries (tara tarastatic ${LIBZ3} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
