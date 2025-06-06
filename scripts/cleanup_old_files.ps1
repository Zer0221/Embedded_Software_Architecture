# 清理多余文件

# 基础层文件
$baseFiles = @(
    "e:\Software_Structure\include\adc_api.h",
    "e:\Software_Structure\include\can_api.h",
    "e:\Software_Structure\include\cpu_freq_api.h",
    "e:\Software_Structure\include\display_api.h",
    "e:\Software_Structure\include\dma_api.h",
    "e:\Software_Structure\include\filesystem_api.h",
    "e:\Software_Structure\include\flash_api.h",
    "e:\Software_Structure\include\gpio_api.h",
    "e:\Software_Structure\include\i2c_api.h",
    "e:\Software_Structure\include\interrupt_api.h",
    "e:\Software_Structure\include\platform_api.h",
    "e:\Software_Structure\include\power_api.h",
    "e:\Software_Structure\include\pwm_api.h",
    "e:\Software_Structure\include\sdio_api.h",
    "e:\Software_Structure\include\spi_api.h",
    "e:\Software_Structure\include\timer_api.h",
    "e:\Software_Structure\include\uart_api.h",
    "e:\Software_Structure\include\usb_api.h",
    "e:\Software_Structure\include\watchdog_api.h"
)

# 协议层文件
$protocolFiles = @(
    "e:\Software_Structure\include\bluetooth_api.h",
    "e:\Software_Structure\include\coap_client_api.h",
    "e:\Software_Structure\include\device_auth_api.h",
    "e:\Software_Structure\include\http_client_api.h",
    "e:\Software_Structure\include\https_server_api.h",
    "e:\Software_Structure\include\mqtt_client_api.h",
    "e:\Software_Structure\include\network_api.h",
    "e:\Software_Structure\include\ota_api.h",
    "e:\Software_Structure\include\protocol_converter_api.h",
    "e:\Software_Structure\include\tls_api.h",
    "e:\Software_Structure\include\websocket_api.h"
)

# 功能层文件
$featureFiles = @(
    "e:\Software_Structure\include\access_control_api.h",
    "e:\Software_Structure\include\ai_api.h",
    "e:\Software_Structure\include\audio_api.h",
    "e:\Software_Structure\include\biometric_api.h",
    "e:\Software_Structure\include\camera_api.h",
    "e:\Software_Structure\include\device_manager_api.h",
    "e:\Software_Structure\include\media_manager_api.h",
    "e:\Software_Structure\include\security_api.h",
    "e:\Software_Structure\include\sensor_api.h",
    "e:\Software_Structure\include\touch_api.h",
    "e:\Software_Structure\include\update_manager_api.h"
)

# 通用层文件
$commonFiles = @(
    "e:\Software_Structure\include\app_framework.h",
    "e:\Software_Structure\include\config.h",
    "e:\Software_Structure\include\driver_api.h",
    "e:\Software_Structure\include\error_api.h",
    "e:\Software_Structure\include\event_bus_api.h",
    "e:\Software_Structure\include\json_api.h",
    "e:\Software_Structure\include\log_api.h",
    "e:\Software_Structure\include\rtos_api.h",
    "e:\Software_Structure\include\unit_test.h"
)

# 合并所有文件列表
$allFiles = $baseFiles + $protocolFiles + $featureFiles + $commonFiles

# 创建备份目录
$backupDir = "e:\Software_Structure\backup_old_files"
if (-not (Test-Path $backupDir)) {
    New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
    Write-Host "Created backup directory: $backupDir"
}

# 移动文件到备份目录（而不是直接删除）
foreach ($file in $allFiles) {
    if (Test-Path $file) {
        $fileName = Split-Path -Leaf $file
        Copy-Item -Path $file -Destination "$backupDir\$fileName" -Force
        Remove-Item -Path $file -Force
        Write-Host "Moved to backup and removed: $file"
    } else {
        Write-Host "File not found: $file"
    }
}

Write-Host "Cleanup completed. Old files have been backed up to $backupDir"
