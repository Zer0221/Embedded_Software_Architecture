$appFiles = @(
    "e:\Software_Structure\applications\esp32_adc_pwm_example.c",
    "e:\Software_Structure\applications\esp32_example.c",
    "e:\Software_Structure\applications\fm33lc0xx_matrix_display_example.c",
    "e:\Software_Structure\applications\main.c",
    "e:\Software_Structure\applications\sensor_app.c",
    "e:\Software_Structure\applications\sensor_node.c",
    "e:\Software_Structure\tests\test_adc.c",
    "e:\Software_Structure\tests\test_main.c",
    "e:\Software_Structure\tests\test_power.c",
    "e:\Software_Structure\tests\test_pwm.c",
    "e:\Software_Structure\tests\unit_test.c"
)

foreach ($filePath in $appFiles) {
    if (Test-Path $filePath) {
        # 读取文件内容
        $content = Get-Content -Path $filePath -Raw
        
        # 更新包含路径
        $content = $content -replace '#include "driver_api.h"', '#include "common/driver_api.h"'
        $content = $content -replace '#include "error_api.h"', '#include "common/error_api.h"'
        $content = $content -replace '#include "log_api.h"', '#include "common/log_api.h"'
        $content = $content -replace '#include "json_api.h"', '#include "common/json_api.h"'
        $content = $content -replace '#include "rtos_api.h"', '#include "common/rtos_api.h"'
        $content = $content -replace '#include "config.h"', '#include "common/config.h"'
        $content = $content -replace '#include "event_bus_api.h"', '#include "common/event_bus_api.h"'
        $content = $content -replace '#include "unit_test.h"', '#include "common/unit_test.h"'
        $content = $content -replace '#include "app_framework.h"', '#include "common/app_framework.h"'
        
        # 更新基础层包含路径
        $content = $content -replace '#include "uart_api.h"', '#include "base/uart_api.h"'
        $content = $content -replace '#include "gpio_api.h"', '#include "base/gpio_api.h"'
        $content = $content -replace '#include "adc_api.h"', '#include "base/adc_api.h"'
        $content = $content -replace '#include "pwm_api.h"', '#include "base/pwm_api.h"'
        $content = $content -replace '#include "spi_api.h"', '#include "base/spi_api.h"'
        $content = $content -replace '#include "i2c_api.h"', '#include "base/i2c_api.h"'
        $content = $content -replace '#include "flash_api.h"', '#include "base/flash_api.h"'
        $content = $content -replace '#include "dma_api.h"', '#include "base/dma_api.h"'
        $content = $content -replace '#include "timer_api.h"', '#include "base/timer_api.h"'
        $content = $content -replace '#include "sdio_api.h"', '#include "base/sdio_api.h"'
        $content = $content -replace '#include "usb_api.h"', '#include "base/usb_api.h"'
        $content = $content -replace '#include "can_api.h"', '#include "base/can_api.h"'
        $content = $content -replace '#include "display_api.h"', '#include "base/display_api.h"'
        $content = $content -replace '#include "power_api.h"', '#include "base/power_api.h"'
        $content = $content -replace '#include "watchdog_api.h"', '#include "base/watchdog_api.h"'
        $content = $content -replace '#include "interrupt_api.h"', '#include "base/interrupt_api.h"'
        $content = $content -replace '#include "platform_api.h"', '#include "base/platform_api.h"'
        $content = $content -replace '#include "cpu_freq_api.h"', '#include "base/cpu_freq_api.h"'
        $content = $content -replace '#include "filesystem_api.h"', '#include "base/filesystem_api.h"'
        
        # 更新协议层包含路径
        $content = $content -replace '#include "http_client_api.h"', '#include "protocol/http_client_api.h"'
        $content = $content -replace '#include "https_server_api.h"', '#include "protocol/https_server_api.h"'
        $content = $content -replace '#include "mqtt_client_api.h"', '#include "protocol/mqtt_client_api.h"'
        $content = $content -replace '#include "websocket_api.h"', '#include "protocol/websocket_api.h"'
        $content = $content -replace '#include "tls_api.h"', '#include "protocol/tls_api.h"'
        $content = $content -replace '#include "bluetooth_api.h"', '#include "protocol/bluetooth_api.h"'
        $content = $content -replace '#include "coap_client_api.h"', '#include "protocol/coap_client_api.h"'
        $content = $content -replace '#include "protocol_converter_api.h"', '#include "protocol/protocol_converter_api.h"'
        $content = $content -replace '#include "device_auth_api.h"', '#include "protocol/device_auth_api.h"'
        $content = $content -replace '#include "ota_api.h"', '#include "protocol/ota_api.h"'
        $content = $content -replace '#include "network_api.h"', '#include "protocol/network_api.h"'
        
        # 更新功能层包含路径
        $content = $content -replace '#include "camera_api.h"', '#include "feature/camera_api.h"'
        $content = $content -replace '#include "audio_api.h"', '#include "feature/audio_api.h"'
        $content = $content -replace '#include "media_manager_api.h"', '#include "feature/media_manager_api.h"'
        $content = $content -replace '#include "touch_api.h"', '#include "feature/touch_api.h"'
        $content = $content -replace '#include "access_control_api.h"', '#include "feature/access_control_api.h"'
        $content = $content -replace '#include "biometric_api.h"', '#include "feature/biometric_api.h"'
        $content = $content -replace '#include "security_api.h"', '#include "feature/security_api.h"'
        $content = $content -replace '#include "ai_api.h"', '#include "feature/ai_api.h"'
        $content = $content -replace '#include "sensor_api.h"', '#include "feature/sensor_api.h"'
        $content = $content -replace '#include "device_manager_api.h"', '#include "feature/device_manager_api.h"'
        $content = $content -replace '#include "update_manager_api.h"', '#include "feature/update_manager_api.h"'
        
        # 保存更新后的内容
        Set-Content -Path $filePath -Value $content
        Write-Host "Updated: $filePath"
    } else {
        Write-Host "File not found: $filePath"
    }
}

Write-Host "Application file updating complete."
