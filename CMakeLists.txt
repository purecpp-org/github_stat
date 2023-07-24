cmake_minimum_required(VERSION 3.23)
project(github_stat)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        cinatra/include
        ormpp/include
        ormpp/thirdparty/sqlite3
)

add_definitions(-DORMPP_ENABLE_SQLITE3)

add_library(sqlite3 ormpp/thirdparty/sqlite3/shell.c ormpp/thirdparty/sqlite3/sqlite3.c)

add_executable(github_stat main.cpp)

target_link_libraries(github_stat pthread -ldl sqlite3)
