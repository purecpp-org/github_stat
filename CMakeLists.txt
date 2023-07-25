cmake_minimum_required(VERSION 3.23)
project(github_stat)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        cinatra/include
        ormpp/include
        ormpp/thirdparty/sqlite3
        iguana
)

# --------------------- Gcc
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
    #-ftree-slp-vectorize with coroutine cause link error. disable it util gcc fix.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize")
endif()

# --------------------- Msvc
# Resolves C1128 complained by MSVC: number of sections exceeded object file format limit: compile with /bigobj.
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
# Resolves C4737 complained by MSVC: C4737: Unable to perform required tail call. Performance may be degraded. "Release-Type only"
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/EHa>")

find_package(OpenSSL REQUIRED)

add_definitions(-DORMPP_ENABLE_SQLITE3)
add_definitions(-DCINATRA_ENABLE_SSL)

add_library(sqlite3 ormpp/thirdparty/sqlite3/shell.c ormpp/thirdparty/sqlite3/sqlite3.c)

add_executable(github_stat main.cpp)

target_link_libraries(github_stat ${OPENSSL_LIBRARIES} pthread -ldl sqlite3)
