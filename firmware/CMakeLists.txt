# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Normalize all paths to forward slashes (fixes Windows \U escape issues in CMake)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CMAKE_CURRENT_SOURCE_DIR_CLEAN)

# Set IDF_PATH relative to this project
set(IDF_PATH "${CMAKE_CURRENT_SOURCE_DIR_CLEAN}/../esp-idf")
set(ENV{IDF_PATH} "${IDF_PATH}")

# Set ESP_MATTER_PATH to esp-matter-v2
if(NOT DEFINED ENV{ESP_MATTER_PATH})
    set(ENV{ESP_MATTER_PATH} "${CMAKE_CURRENT_SOURCE_DIR_CLEAN}/../esp-matter-v2")
else()
    file(TO_CMAKE_PATH "$ENV{ESP_MATTER_PATH}" _esp_matter_clean)
    set(ENV{ESP_MATTER_PATH} "${_esp_matter_clean}")
endif()

set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})

# Set device HAL path based on target
if(NOT DEFINED ENV{ESP_MATTER_DEVICE_PATH})
    if("${IDF_TARGET}" STREQUAL "esp32s3" OR "${IDF_TARGET}" STREQUAL "")
        set(ENV{ESP_MATTER_DEVICE_PATH} ${ESP_MATTER_PATH}/device_hal/device/esp32s3_devkit_c)
    elseif("${IDF_TARGET}" STREQUAL "esp32c3")
        set(ENV{ESP_MATTER_DEVICE_PATH} ${ESP_MATTER_PATH}/device_hal/device/esp32c3_devkit_m)
    elseif("${IDF_TARGET}" STREQUAL "esp32c6")
        set(ENV{ESP_MATTER_DEVICE_PATH} ${ESP_MATTER_PATH}/device_hal/device/esp32c6_devkit_c)
    else()
        message(FATAL_ERROR "Unsupported IDF_TARGET: ${IDF_TARGET}")
    endif()
else()
    file(TO_CMAKE_PATH "$ENV{ESP_MATTER_DEVICE_PATH}" _device_path_clean)
    set(ENV{ESP_MATTER_DEVICE_PATH} "${_device_path_clean}")
endif()

# MATTER_SDK_PATH follows esp-matter-v2 convention (nested connectedhomeip)
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)

# Include IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Include esp-matter common cmake helpers
include(${ESP_MATTER_PATH}/examples/common/cmake_common/components_include.cmake)
include($ENV{ESP_MATTER_DEVICE_PATH}/esp_matter_device.cmake)

# Component directories
set(EXTRA_COMPONENT_DIRS
    "${ESP_MATTER_PATH}/examples/common"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components"
    "${ESP_MATTER_PATH}/device_hal/device"
    ${extra_components_dirs_append}
)

project(matter_led_ecosystem)

set(PROJECT_VER "0.1.0")
set(PROJECT_VER_NUMBER 1)

idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)
