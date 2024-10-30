cmake_minimum_required(VERSION 3.6)
project(brutforce)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Xclang -fopenmp")

find_package(OpenSSL REQUIRED)

add_executable(brutforce main.cpp
        calc_md5.cpp
        pthread_bruteforce.cpp
        )

target_link_libraries(brutforce PRIVATE OpenSSL::SSL INTERFACE OpenSSL::Crypto)
