cmake_minimum_required(VERSION 3.6)
project(t_1_1_1_brutforce_md5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")


find_package(OpenSSL REQUIRED)

add_executable(t_1_1_1_brutforce_md5 main.cpp)

target_link_libraries(t_1_1_1_brutforce_md5 PRIVATE OpenSSL::SSL INTERFACE OpenSSL::Crypto)
