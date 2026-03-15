# Main component source files
set(SRCS
    "main.cpp"
    "led_engine/led_engine.cpp"
    "led_engine/music_sync.cpp"
    "matter/matter_device.cpp"
    "network/network_manager.cpp"
    "storage/storage_manager.cpp"
)

# Select HAL implementation based on target
if(CONFIG_IDF_TARGET_ESP32S3)
    list(APPEND SRCS "hal/hal_esp32s3.cpp")
elseif(CONFIG_IDF_TARGET_ESP32C3)
    list(APPEND SRCS "hal/hal_esp32c3.cpp")
elseif(CONFIG_IDF_TARGET_ESP32C6)
    list(APPEND SRCS "hal/hal_esp32c6.cpp")
endif()

idf_component_register(
    SRCS
        ${SRCS}
    INCLUDE_DIRS
        "."
        "hal"
        "led_engine"
        "matter"
        "network"
        "storage"
        "config"
        "util"
    PRIV_REQUIRES
        driver
        esp_wifi
        esp_event
        nvs_flash
        esp_netif
        led_strip
        esp_matter
        esp_matter_console
        app_reset
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
