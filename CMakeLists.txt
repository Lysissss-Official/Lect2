cmake_minimum_required(VERSION 4.2)
project(ApsisUI2)

set(CMAKE_CXX_STANDARD 20)

add_executable(ApsisUI2 main.cpp
        include/UIRender.h
        include/UICtrller.h
        include/UITransor.h
        include/UIStructure.h
        include/easyx/easyx.h
        include/easyx/graphics.h
        compat.c
)

target_link_libraries(ApsisUI2 ${CMAKE_SOURCE_DIR}/lib/libeasyx.a)

#set_target_properties(ApsisUI2 PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_options(ApsisUI2 PRIVATE -static)