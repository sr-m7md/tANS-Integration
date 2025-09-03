cmake_minimum_required(VERSION 4.0)
project(WorkTask1)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)

add_executable(sender sender.cpp
        tANS.h)
target_link_libraries(sender Boost::system)

add_executable(receiver receiver.cpp
        tANS.h)
target_link_libraries(receiver Boost::system)
