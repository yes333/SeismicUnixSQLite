cmake_minimum_required(VERSION 3.14)

project(spDB VERSION 1.1 
             LANGUAGES CXX 
             DESCRIPTION "spDB version modified Sept. 2020")

#set (CMAKE_BUILD_TYPE "Debug")
set (CMAKE_BUILD_TYPE "Release")

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(SPDB_INSTALL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SPDB_INSTALL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(SPDB_INSTALL_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)


add_subdirectory(spFramework)

add_subdirectory(spSqliteUtils)

add_subdirectory(spdbread)

add_subdirectory(spdbwrite)

add_subdirectory(spPython)

#add_subdirectory(spFrameTest)
#add_subdirectory(spUnitTests)
