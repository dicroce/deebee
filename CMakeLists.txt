cmake_minimum_required (VERSION 3.11.2)

include(FetchContent)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Debug)
endif()

set (CMAKE_CXX_STANDARD 14)

# Dependencies

FetchContent_Declare(
    cppkit
    GIT_REPOSITORY https://github.com/dicroce/cppkit
    GIT_TAG master
)

FetchContent_GetProperties(cppkit)
if(NOT cppkit_POPULATED)
  FetchContent_Populate(cppkit)
  add_subdirectory(${cppkit_SOURCE_DIR} ${cppkit_BINARY_DIR})
endif()

# deebee

add_library (deebee include/sqlite3/sqlite3.h
                    include/deebee/sqlite_conn.h
                    include/deebee/trans.h
                    source/sqlite_conn.cpp
                    source/sqlite3.c)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(deebee PUBLIC "-O0")
    target_compile_options(deebee PUBLIC "-ggdb")
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(deebee PUBLIC "-O3")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

target_include_directories(deebee PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(deebee cppkit)

