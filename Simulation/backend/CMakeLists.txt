cmake_minimum_required(VERSION 3.12)
project(crow_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add CPR cmake directory to search path
list(APPEND CMAKE_PREFIX_PATH "/usr/lib/cmake/cpr")
list(APPEND CMAKE_MODULE_PATH "/usr/lib/cmake/cpr")

# Find packages
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS regex)
find_package(OpenSSL REQUIRED)
# find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(cpr REQUIRED CONFIG)

# Collect all source and header files
file(GLOB_RECURSE SOURCE_FILES 
    "src/*.cpp"
)

file(GLOB_RECURSE HEADER_FILES 
    "include/*.hpp"
    "include/*.h"
    "src/*.hpp"
    "src/*.h"
)

# Add executable with all sources
add_executable(main 
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# Include directories - making include directory first in search path
target_include_directories(main
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(main
    PRIVATE
    Threads::Threads
    Boost::regex
    OpenSSL::SSL
    # OpenSSL::Crypto
    # ZLIB::ZLIB
    cpr::cpr
)

# Add compiler flags
target_compile_options(main
    PRIVATE
    -Wall
    -Wextra
    -O2
)

install(TARGETS main DESTINATION /usr/local/bin)
install(DIRECTORY include/ DESTINATION /usr/local/include)