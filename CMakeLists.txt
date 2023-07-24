cmake_minimum_required(VERSION 3.23)
project(github_stat)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        cinatra/include
        ormpp/include
)

add_definitions(-DORMPP_ENABLE_SQLITE3)

add_executable(github_stat main.cpp)

target_link_libraries(github_stat sqlite3)
