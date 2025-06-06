cmake_minimum_required(VERSION 3.13)

# 设置项目名称
project(EmbeddedSoftwareFramework C)

# 设置C标准
set(CMAKE_C_STANDARD 99)

# 定义输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# 定义源代码目录
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(INCLUDE_DIR ${SOURCE_DIR}/include)
set(INCLUDE_BASE_DIR ${SOURCE_DIR}/include/base)
set(INCLUDE_PROTOCOL_DIR ${SOURCE_DIR}/include/protocol)
set(INCLUDE_FEATURE_DIR ${SOURCE_DIR}/include/feature)
set(INCLUDE_COMMON_DIR ${SOURCE_DIR}/include/common)
set(DRIVERS_DIR ${SOURCE_DIR}/drivers)
set(DRIVERS_BASE_DIR ${SOURCE_DIR}/drivers/base)
set(DRIVERS_PROTOCOL_DIR ${SOURCE_DIR}/drivers/protocol)
set(DRIVERS_FEATURE_DIR ${SOURCE_DIR}/drivers/feature)
set(PLATFORM_DIR ${SOURCE_DIR}/platform)
set(RTOS_DIR ${SOURCE_DIR}/rtos)
set(SRC_DIR ${SOURCE_DIR}/src)
set(APPLICATIONS_DIR ${SOURCE_DIR}/applications)
set(TESTS_DIR ${SOURCE_DIR}/tests)

# 平台选择选项 - 这些值会通过CMake缓存传递给project_config.h
option(TARGET_STM32 "Target STM32 platform" ON)
option(TARGET_ESP32 "Target ESP32 platform" OFF)
option(TARGET_IT986XX "Target IT986XX platform" OFF)
option(TARGET_FM33LC0XX "Target FM33LC0XX platform" OFF)

# RTOS选择选项
option(USE_RTOS_FREERTOS "Use FreeRTOS" ON)
option(USE_RTOS_THREADX "Use ThreadX" OFF)
option(USE_RTOS_UCOS "Use uCOS" OFF)
option(USE_RTOS_NONE "No RTOS" OFF)

# 应用程序选择
option(APP_SENSOR "Build sensor application" ON)
option(APP_ESP32_EXAMPLE "Build ESP32 example application" OFF)
option(APP_ESP32_ADC_PWM "Build ESP32 ADC PWM example" OFF)
option(APP_FM33LC0XX_MATRIX "Build FM33LC0XX matrix display example" OFF)
option(APP_SENSOR_NODE "Build sensor node application" OFF)

# 功能模块选择
option(ENABLE_ERROR_HANDLING "Enable error handling module" ON)
option(ENABLE_DRIVER_MANAGER "Enable driver manager" ON)
option(ENABLE_MEMORY_MANAGER "Enable memory manager" ON)
option(ENABLE_DEVICE_TREE "Enable device tree" ON)
option(ENABLE_MODULE_SUPPORT "Enable module support" ON)
option(ENABLE_UNIT_TEST "Enable unit test framework" ON)

# 确保只选择了一个平台
if((TARGET_STM32 AND TARGET_ESP32) OR 
   (TARGET_STM32 AND TARGET_IT986XX) OR 
   (TARGET_STM32 AND TARGET_FM33LC0XX) OR
   (TARGET_ESP32 AND TARGET_IT986XX) OR
   (TARGET_ESP32 AND TARGET_FM33LC0XX) OR
   (TARGET_IT986XX AND TARGET_FM33LC0XX))
    message(FATAL_ERROR "只能选择一个目标平台")
endif()

# 确保只选择了一个RTOS
if((USE_RTOS_FREERTOS AND USE_RTOS_THREADX) OR 
   (USE_RTOS_FREERTOS AND USE_RTOS_UCOS) OR 
   (USE_RTOS_FREERTOS AND USE_RTOS_NONE) OR
   (USE_RTOS_THREADX AND USE_RTOS_UCOS) OR
   (USE_RTOS_THREADX AND USE_RTOS_NONE) OR
   (USE_RTOS_UCOS AND USE_RTOS_NONE))
    message(FATAL_ERROR "只能选择一个RTOS选项")
endif()

# 根据选择生成编译定义
if(TARGET_STM32)
    add_definitions(-DTARGET_PLATFORM=PLATFORM_STM32)
elseif(TARGET_ESP32)
    add_definitions(-DTARGET_PLATFORM=PLATFORM_ESP32)
elseif(TARGET_IT986XX)
    add_definitions(-DTARGET_PLATFORM=PLATFORM_IT986XX)
elseif(TARGET_FM33LC0XX)
    add_definitions(-DTARGET_PLATFORM=PLATFORM_FM33LC0XX)
endif()

if(USE_RTOS_FREERTOS)
    add_definitions(-DUSE_RTOS=RTOS_FREERTOS)
elseif(USE_RTOS_THREADX)
    add_definitions(-DUSE_RTOS=RTOS_THREADX)
elseif(USE_RTOS_UCOS)
    add_definitions(-DUSE_RTOS=RTOS_UCOS)
elseif(USE_RTOS_NONE)
    add_definitions(-DUSE_RTOS=RTOS_NONE)
endif()

# 定义通用头文件目录
include_directories(
    ${INCLUDE_DIR}
    ${INCLUDE_BASE_DIR}
    ${INCLUDE_PROTOCOL_DIR}
    ${INCLUDE_FEATURE_DIR}
    ${INCLUDE_COMMON_DIR}
    ${PLATFORM_DIR}/mcu
    ${RTOS_DIR}/api
    ${DRIVERS_DIR}  # 添加驱动目录，确保驱动互相引用时可以找到头文件
)

# 根据目标平台添加额外的包含目录
if(TARGET_ESP32)
    # 配置ESP32特定的编译选项和包含目录
    # 注意：需要根据ESP-IDF的安装位置调整
    set(ESP32_IDF_PATH "$ENV{IDF_PATH}")
    if(ESP32_IDF_PATH)
        include_directories(
            ${ESP32_IDF_PATH}/components/esp_common/include
            ${ESP32_IDF_PATH}/components/esp_system/include
            ${ESP32_IDF_PATH}/components/esp_timer/include
            ${ESP32_IDF_PATH}/components/driver/include
            ${ESP32_IDF_PATH}/components/esp_hw_support/include
            ${ESP32_IDF_PATH}/components/esp_rom/include
            ${ESP32_IDF_PATH}/components/hal/include
            ${ESP32_IDF_PATH}/components/soc/include
        )
    else()
        message(WARNING "ESP32_IDF_PATH not found. Please set it correctly.")
    endif()
endif()

if(TARGET_STM32)
    # 配置STM32特定的编译选项和包含目录
    # 注意：需要根据STM32 HAL库的安装位置调整
    set(STM32_HAL_PATH "$ENV{STM32_HAL_PATH}")
    if(STM32_HAL_PATH)
        include_directories(
            ${STM32_HAL_PATH}/Inc
            ${STM32_HAL_PATH}/CMSIS/Device/ST/STM32F4xx/Include
            ${STM32_HAL_PATH}/CMSIS/Include
        )
    else()
        message(WARNING "STM32_HAL_PATH not found. Please set it correctly.")
    endif()
endif()

if(TARGET_FM33LC0XX)
    # 配置FM33LC0XX特定的编译选项和包含目录
    set(FM33LC0XX_SDK_PATH "$ENV{FM33LC0XX_SDK_PATH}")
    if(FM33LC0XX_SDK_PATH)
        include_directories(
            ${FM33LC0XX_SDK_PATH}/include
        )
    else()
        message(WARNING "FM33LC0XX_SDK_PATH not found. Please set it correctly.")
    endif()
endif()

# 配置RTOS
if(USE_RTOS_FREERTOS)
    # 添加FreeRTOS源文件和包含目录
    set(FREERTOS_PATH "$ENV{FREERTOS_PATH}")
    if(FREERTOS_PATH)
        include_directories(
            ${FREERTOS_PATH}/include
            ${RTOS_DIR}/freertos
        )
        
        # 可以添加FreeRTOS移植层包含目录
        if(TARGET_STM32)
            include_directories(${FREERTOS_PATH}/portable/GCC/ARM_CM4F)
        elseif(TARGET_ESP32)
            include_directories(${FREERTOS_PATH}/portable/GCC/Xtensa_ESP32)
        endif()
    else()
        message(WARNING "FREERTOS_PATH not found. Please set it correctly.")
    endif()
endif()

if(USE_RTOS_THREADX)
    # 添加ThreadX源文件和包含目录
    set(THREADX_PATH "$ENV{THREADX_PATH}")
    if(THREADX_PATH)
        include_directories(
            ${THREADX_PATH}/common/inc
            ${RTOS_DIR}/threadx
        )
    else()
        message(WARNING "THREADX_PATH not found. Please set it correctly.")
    endif()
endif()

# 功能模块定义
if(ENABLE_ERROR_HANDLING)
    add_definitions(-DCONFIG_ERROR_HANDLING_ENABLED=1)
endif()

if(ENABLE_DRIVER_MANAGER)
    add_definitions(-DCONFIG_DRIVER_MANAGER_ENABLED=1)
endif()

if(ENABLE_MEMORY_MANAGER)
    add_definitions(-DCONFIG_MEMORY_MANAGER_ENABLED=1)
endif()

if(ENABLE_DEVICE_TREE)
    add_definitions(-DCONFIG_DEVICE_TREE_ENABLED=1)
endif()

if(ENABLE_MODULE_SUPPORT)
    add_definitions(-DCONFIG_MODULE_SUPPORT=1)
endif()

if(ENABLE_UNIT_TEST)
    add_definitions(-DCONFIG_UNIT_TEST_ENABLED=1)
endif()

# 收集源文件
set(COMMON_SOURCES "")

# 添加必要的核心源文件
list(APPEND COMMON_SOURCES ${SRC_DIR}/error_handling.c)

if(ENABLE_DRIVER_MANAGER)
    list(APPEND COMMON_SOURCES ${SRC_DIR}/driver_manager.c)
endif()

if(ENABLE_MEMORY_MANAGER)
    list(APPEND COMMON_SOURCES ${SRC_DIR}/memory_manager.c)
endif()

if(ENABLE_DEVICE_TREE)
    list(APPEND COMMON_SOURCES ${SRC_DIR}/device_tree.c)
endif()

if(ENABLE_MODULE_SUPPORT)
    list(APPEND COMMON_SOURCES ${SRC_DIR}/module_support.c)
endif()

if(ENABLE_UNIT_TEST)
    list(APPEND COMMON_SOURCES ${SRC_DIR}/unit_test.c)
endif()

# 基于目标平台选择合适的驱动文件
if(TARGET_ESP32)
    file(GLOB_RECURSE PLATFORM_SPECIFIC_DRIVER_SOURCES 
        ${DRIVERS_DIR}/base/*/esp32_*.c
        ${DRIVERS_DIR}/protocol/*/esp32_*.c
        ${DRIVERS_DIR}/feature/*/esp32_*.c
    )
elseif(TARGET_STM32)
    file(GLOB_RECURSE PLATFORM_SPECIFIC_DRIVER_SOURCES 
        ${DRIVERS_DIR}/base/*/stm32_*.c
        ${DRIVERS_DIR}/protocol/*/stm32_*.c
        ${DRIVERS_DIR}/feature/*/stm32_*.c
    )
elseif(TARGET_IT986XX)
    file(GLOB_RECURSE PLATFORM_SPECIFIC_DRIVER_SOURCES 
        ${DRIVERS_DIR}/base/*/it986xx_*.c
        ${DRIVERS_DIR}/protocol/*/it986xx_*.c
        ${DRIVERS_DIR}/feature/*/it986xx_*.c
    )
elseif(TARGET_FM33LC0XX)
    file(GLOB_RECURSE PLATFORM_SPECIFIC_DRIVER_SOURCES 
        ${DRIVERS_DIR}/base/*/fm33lc0xx_*.c
        ${DRIVERS_DIR}/protocol/*/fm33lc0xx_*.c
        ${DRIVERS_DIR}/feature/*/fm33lc0xx_*.c
    )
endif()

# 收集平台无关的驱动文件
file(GLOB_RECURSE COMMON_DRIVER_SOURCES 
    ${DRIVERS_DIR}/common/*.c
    ${DRIVERS_DIR}/base/*/common_*.c
    ${DRIVERS_DIR}/protocol/*/common_*.c
    ${DRIVERS_DIR}/feature/*/common_*.c
)

# 从COMMON_DRIVER_SOURCES中排除特定平台的文件
if(PLATFORM_SPECIFIC_DRIVER_SOURCES)
    foreach(SPECIFIC_SOURCE ${PLATFORM_SPECIFIC_DRIVER_SOURCES})
        list(APPEND DRIVER_SOURCES ${SPECIFIC_SOURCE})
    endforeach()
else()
    set(DRIVER_SOURCES ${COMMON_DRIVER_SOURCES})
endif()

# 基于目标平台选择合适的平台文件
if(TARGET_ESP32)
    file(GLOB_RECURSE PLATFORM_SOURCES 
        ${PLATFORM_DIR}/mcu/esp32_*.c
    )
elseif(TARGET_STM32)
    file(GLOB_RECURSE PLATFORM_SOURCES 
        ${PLATFORM_DIR}/mcu/stm32_*.c
    )
elseif(TARGET_FM33LC0XX)
    file(GLOB_RECURSE PLATFORM_SOURCES 
        ${PLATFORM_DIR}/mcu/fm33lc0xx_*.c
    )
endif()

if(USE_FREERTOS)
    file(GLOB_RECURSE RTOS_SOURCES 
        ${RTOS_DIR}/freertos/*.c
    )
elseif(USE_UCOS)
    file(GLOB_RECURSE RTOS_SOURCES 
        ${RTOS_DIR}/ucos/*.c
    )
elseif(USE_THREADX)
    file(GLOB_RECURSE RTOS_SOURCES 
        ${RTOS_DIR}/threadx/*.c
    )
else()
    set(RTOS_SOURCES "")
endif()

# 基于选项收集应用程序源文�?
set(APPLICATION_SOURCES "")

# 主应用程序文件始终包�?
list(APPEND APPLICATION_SOURCES ${APPLICATIONS_DIR}/main.c)

# 根据选项添加特定应用程序
if(APP_SENSOR)
    list(APPEND APPLICATION_SOURCES ${APPLICATIONS_DIR}/sensor_app.c)
endif()

if(APP_ESP32_EXAMPLE AND TARGET_ESP32)
    list(APPEND APPLICATION_SOURCES ${APPLICATIONS_DIR}/esp32_example.c)
endif()

if(APP_ESP32_ADC_PWM AND TARGET_ESP32)
    list(APPEND APPLICATION_SOURCES ${APPLICATIONS_DIR}/esp32_adc_pwm_example.c)
endif()

if(APP_FM33LC0XX_MATRIX AND TARGET_FM33LC0XX)
    list(APPEND APPLICATION_SOURCES ${APPLICATIONS_DIR}/fm33lc0xx_matrix_display_example.c)
endif()

if(APP_SENSOR_NODE)
    list(APPEND APPLICATION_SOURCES ${APPLICATIONS_DIR}/sensor_node.c)
endif()

if(ENABLE_TESTS)
    file(GLOB_RECURSE TEST_SOURCES 
        ${TESTS_DIR}/*.c
    )
    add_definitions(-DENABLE_TESTS)
else()
    set(TEST_SOURCES "")
endif()

# 创建主要可执行文�?
add_executable(firmware
    ${COMMON_SOURCES}
    ${DRIVER_SOURCES}
    ${PLATFORM_SOURCES}
    ${RTOS_SOURCES}
    ${APPLICATION_SOURCES}
    ${TEST_SOURCES}
)

# 添加一个静态库目标，包含所有通用组件
add_library(framework_lib STATIC
    ${COMMON_SOURCES}
    ${DRIVER_SOURCES}
    ${PLATFORM_SOURCES}
    ${RTOS_SOURCES}
)

# 可选：添加一个仅包含驱动的库
add_library(drivers_lib STATIC
    ${DRIVER_SOURCES}
)

# 链接所需的库
if(TARGET_ESP32)
    # 链接ESP32特定的库
    target_link_libraries(firmware PRIVATE
        # ESP-IDF�?
        esp32
        freertos
        esp_common
        esp_timer
        driver
        hal
        soc
        esp_hw_support
        esp_system
    )
endif()

if(TARGET_STM32)
    # 链接STM32特定的库
    target_link_libraries(firmware PRIVATE
        # STM32 HAL�?
        stm32f4xx_hal
        stm32f4xx_hal_cortex
        stm32f4xx_hal_gpio
        stm32f4xx_hal_pwr
        stm32f4xx_hal_rcc
    )
endif()

if(TARGET_FM33LC0XX)
    # 链接FM33LC0XX特定的库
    target_link_libraries(firmware PRIVATE
        # FM33LC0XX SDK�?
        fm33lc0xx_sdk
    )
endif()

if(ENABLE_TESTS)
    # 创建测试可执行文�?
    add_executable(run_tests
        ${COMMON_SOURCES}
        ${DRIVER_SOURCES}
        ${PLATFORM_SOURCES}
        ${RTOS_SOURCES}
        ${TEST_SOURCES}
    )
    
    target_compile_definitions(run_tests PRIVATE RUN_TESTS)
endif()

# 设置编译警告选项
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_compile_options(firmware PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
    target_compile_options(framework_lib PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
    target_compile_options(drivers_lib PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
    if(ENABLE_TESTS)
        target_compile_options(run_tests PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
    endif()
endif()

# 安装目标
install(TARGETS firmware framework_lib drivers_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文�?
install(DIRECTORY ${INCLUDE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 安装平台头文�?
install(DIRECTORY ${PLATFORM_DIR}/mcu/
    DESTINATION include/platform
    FILES_MATCHING PATTERN "*.h"
)

# 添加Doxygen文档生成目标（如果找到Doxygen�?
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_INPUT_DIR ${SOURCE_DIR})
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc)
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
    set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # 替换Doxyfile中的变量
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    # 添加文档生成目标
    add_custom_command(
        OUTPUT ${DOXYGEN_INDEX_FILE}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
        COMMENT "生成API文档..."
    )
    add_custom_target(docs DEPENDS ${DOXYGEN_INDEX_FILE})
endif()

