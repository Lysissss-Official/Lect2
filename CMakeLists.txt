cmake_minimum_required(VERSION 4.1.2)
project(ApsisUI2)

set(CMAKE_CXX_STANDARD 20)

add_executable(ApsisUI2 main.cpp
        include/UIRender.h
        include/UICtrller.h
        include/UITransor.h
        include/UIStructure.h)
