cmake_minimum_required(VERSION 4.1.2)
project(ApsisUI2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

add_executable(ApsisUI2
        src/main.cpp
        src/screen.cpp
        src/compat.c
        third_party/pocketpy/pocketpy.c
        include/lcore/ThreadMgr.h
        include/lcore/ThreadMgr.cpp
        include/lapp/Application.h
        include/lui/base/UIStructure.h
        include/lui/base/UICtrller.h
        include/lui/base/UIRender.h
        include/lui/base/UITransor.h
        include/lui/platform/linux7/UICtrllerLinux7.h
        include/lui/platform/linux7/UITransorLinux7.h
        include/lui/platform/linux7/theme/UIRenderApsisUI2.h
        apps/demo_app.h
        apps/pyconsole_app.h
        apps/launcher_app.h
        include/lui/extension/font/font_con_24.h
        include/lui/extension/font/FontBase.h
        include/lcore/VMMgr.h
        include/lui/extension/image/ExUIImage.h
        include/lui/extension/font/ExUIFont.h
        include/lui/platform/linux7/theme/UIHelperApsisUI2.h
)

target_include_directories(ApsisUI2 PRIVATE
        .
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