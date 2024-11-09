cmake_minimum_required(VERSION 3.10)
project(PistacheApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Pistache REQUIRED IMPORTED_TARGET libpistache)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Automatically include all .cpp files in the src directory
file(GLOB SOURCES "src/*.cpp")

add_executable(server src/main.cpp)
target_link_libraries(server PkgConfig::Pistache pthread nlohmann_json::nlohmann_json)