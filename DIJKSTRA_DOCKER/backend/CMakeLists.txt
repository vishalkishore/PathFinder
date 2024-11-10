# cmake_minimum_required(VERSION 3.10)
# project(array_sorter)

# set(CMAKE_CXX_STANDARD 14)

# find_package(cpprestsdk REQUIRED)

# add_executable(array_sorter main.cpp)
# target_link_libraries(array_sorter PRIVATE cpprestsdk::cpprest)

cmake_minimum_required(VERSION 3.10)
project(spt_server)

set(CMAKE_CXX_STANDARD 14)

find_package(cpprestsdk REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(spt_server main.cpp)
target_link_libraries(spt_server 
    PRIVATE 
    cpprestsdk::cpprest
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CMAKE_DL_LIBS}
)

# Explicitly link against libcrypto
target_link_libraries(spt_server PRIVATE -lcrypto)

# Add compiler flags to show all warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Print the link libraries for debugging
get_target_property(LINK_LIBRARIES spt_server LINK_LIBRARIES)
message(STATUS "Link libraries: ${LINK_LIBRARIES}")
