cmake_minimum_required(VERSION 4.1.2)
project(ApsisUI2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

add_executable(ApsisUI2
        src/main.cpp
        src/compat.c
        third_party/pocketpy/pocketpy.c
)

target_include_directories(ApsisUI2 PRIVATE
        include
        third_party/easyx/include
        third_party/pocketpy
)

target_link_libraries(ApsisUI2
        ${CMAKE_SOURCE_DIR}/third_party/easyx/lib/libeasyx.a
        ws2_32
)

#set_target_properties(ApsisUI2 PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_options(ApsisUI2 PRIVATE -static)