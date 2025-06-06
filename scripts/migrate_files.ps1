# 迁移接口文件到对应目录

# 定义目标目录
$baseDir = "e:\Software_Structure\include\base\"
$protocolDir = "e:\Software_Structure\include\protocol\"
$featureDir = "e:\Software_Structure\include\feature\"
$commonDir = "e:\Software_Structure\include\common\"

# 确保目录存在
if (-not (Test-Path $baseDir)) { New-Item -ItemType Directory -Path $baseDir -Force | Out-Null }
if (-not (Test-Path $protocolDir)) { New-Item -ItemType Directory -Path $protocolDir -Force | Out-Null }
if (-not (Test-Path $featureDir)) { New-Item -ItemType Directory -Path $featureDir -Force | Out-Null }
if (-not (Test-Path $commonDir)) { New-Item -ItemType Directory -Path $commonDir -Force | Out-Null }

# 基础层文件迁移
$baseFiles = @(
    "adc_api.h",
    "can_api.h",
    "cpu_freq_api.h",
    "display_api.h",
    "dma_api.h",
    "filesystem_api.h",
    "flash_api.h",
    "gpio_api.h",
    "i2c_api.h",
    "interrupt_api.h",
    "platform_api.h",
    "power_api.h",
    "pwm_api.h",
    "sdio_api.h",
    "spi_api.h",
    "timer_api.h",
    "uart_api.h",
    "usb_api.h",
    "watchdog_api.h"
)

# 协议层文件迁移
$protocolFiles = @(
    "bluetooth_api.h",
    "coap_client_api.h",
    "device_auth_api.h",
    "https_server_api.h",
    "http_client_api.h",
    "mqtt_client_api.h",
    "network_api.h",
    "ota_api.h",
    "protocol_converter_api.h",
    "tls_api.h"
)

# 功能层文件迁移
$featureFiles = @(
    "access_control_api.h",
    "ai_api.h",
    "audio_api.h",
    "biometric_api.h",
    "camera_api.h",
    "device_manager_api.h",
    "media_manager_api.h",
    "security_api.h",
    "sensor_api.h",
    "touch_api.h",
    "update_manager_api.h"
)

# 通用层文件迁移
$commonFiles = @(
    "app_framework.h",
    "config.h",
    "driver_api.h",
    "error_api.h",
    "event_bus_api.h",
    "json_api.h",
    "log_api.h",
    "rtos_api.h",
    "unit_test.h"
)

# 创建备份目录
$backupPath = "e:\Software_Structure\include\backup"
if (-not (Test-Path $backupPath)) {
    New-Item -ItemType Directory -Path $backupPath -Force | Out-Null
}

# 执行基础层文件迁移
foreach ($file in $baseFiles) {
    $srcPath = "e:\Software_Structure\include\$file"
    $dstPath = "$baseDir$file"
    
    if (Test-Path $srcPath) {
        # 备份原文件
        Copy-Item -Path $srcPath -Destination "$backupPath\$file" -Force
        
        # 移动文件
        Move-Item -Path $srcPath -Destination $dstPath -Force
        
        Write-Output "已移动 $file 到 base 目录"
    } else {
        Write-Output "文件 $file 不存在于根目录"
    }
}

# 执行协议层文件迁移
foreach ($file in $protocolFiles) {
    $srcPath = "e:\Software_Structure\include\$file"
    $dstPath = "$protocolDir$file"
    
    if (Test-Path $srcPath) {
        # 备份原文件
        Copy-Item -Path $srcPath -Destination "$backupPath\$file" -Force
        
        # 移动文件
        Move-Item -Path $srcPath -Destination $dstPath -Force
        
        Write-Output "已移动 $file 到 protocol 目录"
    } else {
        Write-Output "文件 $file 不存在于根目录"
    }
}

# 执行功能层文件迁移
foreach ($file in $featureFiles) {
    $srcPath = "e:\Software_Structure\include\$file"
    $dstPath = "$featureDir$file"
    
    if (Test-Path $srcPath) {
        # 备份原文件
        Copy-Item -Path $srcPath -Destination "$backupPath\$file" -Force
        
        # 移动文件
        Move-Item -Path $srcPath -Destination $dstPath -Force
        
        Write-Output "已移动 $file 到 feature 目录"
    } else {
        Write-Output "文件 $file 不存在于根目录"
    }
}

# 执行通用层文件迁移
foreach ($file in $commonFiles) {
    $srcPath = "e:\Software_Structure\include\$file"
    $dstPath = "$commonDir$file"
    
    if (Test-Path $srcPath) {
        # 备份原文件
        Copy-Item -Path $srcPath -Destination "$backupPath\$file" -Force
        
        # 移动文件
        Move-Item -Path $srcPath -Destination $dstPath -Force
        
        Write-Output "已移动 $file 到 common 目录"
    } else {
        Write-Output "文件 $file 不存在于根目录"
    }
}

Write-Output "接口文件迁移完成！"
