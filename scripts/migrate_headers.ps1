$headerMoves = @(
    # 将base层接口文件迁移到base目录
    @{from="e:\Software_Structure\include\adc_api.h"; to="e:\Software_Structure\include\base\adc_api.h"},
    @{from="e:\Software_Structure\include\can_api.h"; to="e:\Software_Structure\include\base\can_api.h"},
    @{from="e:\Software_Structure\include\cpu_freq_api.h"; to="e:\Software_Structure\include\base\cpu_freq_api.h"},
    @{from="e:\Software_Structure\include\display_api.h"; to="e:\Software_Structure\include\base\display_api.h"},
    @{from="e:\Software_Structure\include\dma_api.h"; to="e:\Software_Structure\include\base\dma_api.h"},
    @{from="e:\Software_Structure\include\filesystem_api.h"; to="e:\Software_Structure\include\base\filesystem_api.h"},
    @{from="e:\Software_Structure\include\flash_api.h"; to="e:\Software_Structure\include\base\flash_api.h"},
    @{from="e:\Software_Structure\include\gpio_api.h"; to="e:\Software_Structure\include\base\gpio_api.h"},
    @{from="e:\Software_Structure\include\i2c_api.h"; to="e:\Software_Structure\include\base\i2c_api.h"},
    @{from="e:\Software_Structure\include\interrupt_api.h"; to="e:\Software_Structure\include\base\interrupt_api.h"},
    @{from="e:\Software_Structure\include\platform_api.h"; to="e:\Software_Structure\include\base\platform_api.h"},
    @{from="e:\Software_Structure\include\power_api.h"; to="e:\Software_Structure\include\base\power_api.h"},
    @{from="e:\Software_Structure\include\pwm_api.h"; to="e:\Software_Structure\include\base\pwm_api.h"},
    @{from="e:\Software_Structure\include\sdio_api.h"; to="e:\Software_Structure\include\base\sdio_api.h"},
    @{from="e:\Software_Structure\include\spi_api.h"; to="e:\Software_Structure\include\base\spi_api.h"},
    @{from="e:\Software_Structure\include\timer_api.h"; to="e:\Software_Structure\include\base\timer_api.h"},
    @{from="e:\Software_Structure\include\uart_api.h"; to="e:\Software_Structure\include\base\uart_api.h"},
    @{from="e:\Software_Structure\include\usb_api.h"; to="e:\Software_Structure\include\base\usb_api.h"},
    @{from="e:\Software_Structure\include\watchdog_api.h"; to="e:\Software_Structure\include\base\watchdog_api.h"},

    # 将protocol层接口文件迁移到protocol目录
    @{from="e:\Software_Structure\include\bluetooth_api.h"; to="e:\Software_Structure\include\protocol\bluetooth_api.h"},
    @{from="e:\Software_Structure\include\coap_client_api.h"; to="e:\Software_Structure\include\protocol\coap_client_api.h"},
    @{from="e:\Software_Structure\include\device_auth_api.h"; to="e:\Software_Structure\include\protocol\device_auth_api.h"},
    @{from="e:\Software_Structure\include\http_client_api.h"; to="e:\Software_Structure\include\protocol\http_client_api.h"},
    @{from="e:\Software_Structure\include\https_server_api.h"; to="e:\Software_Structure\include\protocol\https_server_api.h"},
    @{from="e:\Software_Structure\include\mqtt_client_api.h"; to="e:\Software_Structure\include\protocol\mqtt_client_api.h"},
    @{from="e:\Software_Structure\include\network_api.h"; to="e:\Software_Structure\include\protocol\network_api.h"},
    @{from="e:\Software_Structure\include\ota_api.h"; to="e:\Software_Structure\include\protocol\ota_api.h"},
    @{from="e:\Software_Structure\include\protocol_converter_api.h"; to="e:\Software_Structure\include\protocol\protocol_converter_api.h"},
    @{from="e:\Software_Structure\include\tls_api.h"; to="e:\Software_Structure\include\protocol\tls_api.h"},
    @{from="e:\Software_Structure\include\websocket_api.h"; to="e:\Software_Structure\include\protocol\websocket_api.h"},

    # 将feature层接口文件迁移到feature目录
    @{from="e:\Software_Structure\include\access_control_api.h"; to="e:\Software_Structure\include\feature\access_control_api.h"},
    @{from="e:\Software_Structure\include\ai_api.h"; to="e:\Software_Structure\include\feature\ai_api.h"},
    @{from="e:\Software_Structure\include\audio_api.h"; to="e:\Software_Structure\include\feature\audio_api.h"},
    @{from="e:\Software_Structure\include\biometric_api.h"; to="e:\Software_Structure\include\feature\biometric_api.h"},
    @{from="e:\Software_Structure\include\camera_api.h"; to="e:\Software_Structure\include\feature\camera_api.h"},
    @{from="e:\Software_Structure\include\device_manager_api.h"; to="e:\Software_Structure\include\feature\device_manager_api.h"},
    @{from="e:\Software_Structure\include\media_manager_api.h"; to="e:\Software_Structure\include\feature\media_manager_api.h"},
    @{from="e:\Software_Structure\include\security_api.h"; to="e:\Software_Structure\include\feature\security_api.h"},
    @{from="e:\Software_Structure\include\sensor_api.h"; to="e:\Software_Structure\include\feature\sensor_api.h"},
    @{from="e:\Software_Structure\include\touch_api.h"; to="e:\Software_Structure\include\feature\touch_api.h"},
    @{from="e:\Software_Structure\include\update_manager_api.h"; to="e:\Software_Structure\include\feature\update_manager_api.h"},

    # 将common层接口文件迁移到common目录
    @{from="e:\Software_Structure\include\app_framework.h"; to="e:\Software_Structure\include\common\app_framework.h"},
    @{from="e:\Software_Structure\include\config.h"; to="e:\Software_Structure\include\common\config.h"},
    @{from="e:\Software_Structure\include\driver_api.h"; to="e:\Software_Structure\include\common\driver_api.h"},
    @{from="e:\Software_Structure\include\error_api.h"; to="e:\Software_Structure\include\common\error_api.h"},
    @{from="e:\Software_Structure\include\event_bus_api.h"; to="e:\Software_Structure\include\common\event_bus_api.h"},
    @{from="e:\Software_Structure\include\json_api.h"; to="e:\Software_Structure\include\common\json_api.h"},
    @{from="e:\Software_Structure\include\log_api.h"; to="e:\Software_Structure\include\common\log_api.h"},
    @{from="e:\Software_Structure\include\rtos_api.h"; to="e:\Software_Structure\include\common\rtos_api.h"},
    @{from="e:\Software_Structure\include\unit_test.h"; to="e:\Software_Structure\include\common\unit_test.h"}
)

foreach ($move in $headerMoves) {
    $fromPath = $move.from
    $toPath = $move.to
    
    if (Test-Path $fromPath) {
        # 确保目标目录存在
        $toDir = Split-Path -Path $toPath -Parent
        if (-not (Test-Path $toDir)) {
            New-Item -ItemType Directory -Path $toDir -Force | Out-Null
        }
        
        # 拷贝文件并更新包含路径
        $content = Get-Content -Path $fromPath -Raw
        
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
        
        # 保存更新后的内容到目标文件
        Set-Content -Path $toPath -Value $content
        Write-Host "Updated and copied: $fromPath -> $toPath"
    } else {
        Write-Host "Source file not found: $fromPath"
    }
}

Write-Host "Header file copying and updating complete."
