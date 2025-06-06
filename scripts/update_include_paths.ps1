## 文件：e:\Software_Structure\scripts\update_include_paths.ps1
## 此脚本用于更新源文件中的include路径，添加适当的层次前缀

# 设置错误处理
$ErrorActionPreference = "Stop"

Write-Host "=== 开始更新include路径 ===" -ForegroundColor Green
Write-Host ""

# 定义层次映射
$baseApis = @(
    "adc_api.h", "can_api.h", "gpio_api.h", "i2c_api.h", "spi_api.h", "uart_api.h", 
    "pwm_api.h", "timer_api.h", "flash_api.h", "dma_api.h", "display_api.h", 
    "power_api.h", "cpu_freq_api.h", "platform_api.h", "watchdog_api.h", 
    "interrupt_api.h", "usb_api.h", "sdio_api.h", "filesystem_api.h"
)

$protocolApis = @(
    "network_api.h", "http_client_api.h", "https_server_api.h", "mqtt_client_api.h", 
    "websocket_api.h", "bluetooth_api.h", "coap_client_api.h", "tls_api.h", 
    "device_auth_api.h", "ota_api.h", "protocol_converter_api.h"
)

$featureApis = @(
    "camera_api.h", "biometric_api.h", "security_api.h", "audio_api.h", 
    "media_manager_api.h", "touch_api.h", "sensor_api.h", "ai_api.h", 
    "access_control_api.h", "device_manager_api.h", "update_manager_api.h"
)

$commonApis = @(
    "error_api.h", "driver_api.h", "log_api.h", "config.h", "json_api.h", 
    "rtos_api.h", "event_bus_api.h", "app_framework.h", "unit_test.h"
)

# 定义源文件路径，可根据需要进行调整
$sourcePaths = @(
    "e:\Software_Structure\drivers",
    "e:\Software_Structure\src",
    "e:\Software_Structure\applications",
    "e:\Software_Structure\rtos",
    "e:\Software_Structure\platform",
    "e:\Software_Structure\tests"
)

# 查找所有C和H文件
$sourceFiles = @()
foreach ($path in $sourcePaths) {
    $sourceFiles += Get-ChildItem -Path $path -Include "*.c", "*.h" -Recurse -File
}

Write-Host "找到 $($sourceFiles.Count) 个源文件" -ForegroundColor Cyan
Write-Host ""

# 更新include路径
$updatedFiles = 0
$errorFiles = 0

foreach ($file in $sourceFiles) {
    try {
        $content = Get-Content -Path $file.FullName -Raw -ErrorAction SilentlyContinue
        if ($content) {
            $originalContent = $content
            
            # 更新基础层接口include路径
            foreach ($api in $baseApis) {
                $content = $content -replace "#include\s+`"$api`"", "#include `"base/$api`""
            }
            
            # 更新协议层接口include路径
            foreach ($api in $protocolApis) {
                $content = $content -replace "#include\s+`"$api`"", "#include `"protocol/$api`""
            }
            
            # 更新功能层接口include路径
            foreach ($api in $featureApis) {
                $content = $content -replace "#include\s+`"$api`"", "#include `"feature/$api`""
            }
            
            # 更新通用层接口include路径
            foreach ($api in $commonApis) {
                $content = $content -replace "#include\s+`"$api`"", "#include `"common/$api`""
            }
            
            # 如果内容有变更，写回文件
            if ($content -ne $originalContent) {
                Set-Content -Path $file.FullName -Value $content
                Write-Host "已更新文件: $($file.FullName)" -ForegroundColor Green
                $updatedFiles++
            }
        }
    }
    catch {
        Write-Host "错误: 处理文件 $($file.FullName) 时出错: $_" -ForegroundColor Red
        $errorFiles++
    }
}

# 输出总结
Write-Host ""
Write-Host "=== 操作完成 ===" -ForegroundColor Green
Write-Host "更新文件: $updatedFiles 个" -ForegroundColor Green
if ($errorFiles -gt 0) {
    Write-Host "处理出错: $errorFiles 个" -ForegroundColor Red
}
Write-Host ""
Write-Host "include路径已更新!" -ForegroundColor Green
