cmake_minimum_required(VERSION 3.10)
project(PistacheApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.10.5  # You can specify the version you need
)
FetchContent_MakeAvailable(json)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Pistache REQUIRED IMPORTED_TARGET libpistache)

# Automatically include all .cpp files in the src directory
file(GLOB SOURCES "src/*.cpp")

add_executable(server src/server.cpp)
target_link_libraries(server PkgConfig::Pistache pthread nlohmann_json::nlohmann_json)