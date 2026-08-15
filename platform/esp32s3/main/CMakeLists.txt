set(LECTOS_ROOT
        ${CMAKE_CURRENT_LIST_DIR}/../../..
)

idf_component_register(
        SRCS
        "main.cpp"

        "${LECTOS_ROOT}/include/lcore/ThreadMgr.cpp"

        "${LECTOS_ROOT}/include/ldevice/screen/ST7305/ST7305ScreenDriver.cpp"
        "${LECTOS_ROOT}/include/ldevice/screen/ST7305v2/ST7305ScreenDriverV2.cpp"

        INCLUDE_DIRS
        "."
        "${LECTOS_ROOT}"
        "${LECTOS_ROOT}/include"
        "${LECTOS_ROOT}/apps"

        PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_spi
)