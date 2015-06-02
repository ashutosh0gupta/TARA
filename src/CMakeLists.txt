cmake_minimum_required(VERSION 2.8.7)

project(TARA)

IF(WIN32)
    SET(Boost_USE_STATIC_LIBS    ON)
    SET(Boost_USE_STATIC_RUNTIME ON)
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
       string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
ELSE(WIN32)
    SET(Boost_USE_STATIC_LIBS    OFF)
    SET(Boost_USE_STATIC_RUNTIME OFF)
ENDIF(WIN32)

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

IF(NOT WIN32)
    ADD_DEFINITIONS(-std=c++11 -Wall)
ENDIF(NOT WIN32)

if (HB_UNICODE AND NOT WIN32)
  ADD_DEFINITIONS(-DHB_UNICODE)
ENDIF(HB_UNICODE AND NOT WIN32)

include_directories (${CMAKE_CURRENT_BINARY_DIR} "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/library" "${PROJECT_BINARY_DIR}/../z3/src/api" "${PROJECT_BINARY_DIR}/../z3/src/api/c++")
include_directories(${Boost_INCLUDE_DIRS}) 
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

add_subdirectory("library")

add_executable(tara main.cpp options_cmd.cpp)

SET(LIBZ3 z3)

target_link_libraries (tara tarastatic ${LIBZ3} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
