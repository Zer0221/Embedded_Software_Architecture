# 简化版驱动迁移脚本
# 该脚本用于将驱动文件移动到按接口层次组织的目录结构中

# 设置错误处理
$ErrorActionPreference = "Stop"

Write-Host "=== 开始驱动目录结构调整 ===" -ForegroundColor Green
Write-Host ""

# 步骤1: 确保目标目录结构存在
$baseDirectories = @(
    "e:\Software_Structure\drivers\base",
    "e:\Software_Structure\drivers\protocol",
    "e:\Software_Structure\drivers\feature"
)

foreach ($dir in $baseDirectories) {
    if (-not (Test-Path $dir)) {
        New-Item -Path $dir -ItemType Directory -Force | Out-Null
        Write-Host "创建目录: $dir" -ForegroundColor Yellow
    }
}

# 步骤2: 定义驱动与接口层级的映射关系
$driverLayerMapping = @{
    # 基础层驱动
    "adc" = "base"
    "can" = "base"
    "display" = "base"
    "dma" = "base"
    "flash" = "base"
    "gpio" = "base"
    "i2c" = "base"
    "power" = "base"
    "pwm" = "base"
    "sdio" = "base"
    "spi" = "base"
    "timer" = "base"
    "uart" = "base"
    "usb" = "base"
    "watchdog" = "base"
    "interrupt" = "base"
    "cpu_freq" = "base"
    "platform" = "base"
    "filesystem" = "base"
    
    # 协议层驱动
    "network" = "protocol"
    "bluetooth" = "protocol"
    "mqtt" = "protocol"
    "http" = "protocol"
    "https" = "protocol"
    "websocket" = "protocol"
    "tls" = "protocol"
    "coap" = "protocol"
    "device_auth" = "protocol"
    "ota" = "protocol"
    "protocol_converter" = "protocol"
    "security" = "protocol"
    
    # 功能层驱动
    "camera" = "feature"
    "biometric" = "feature"
    "audio" = "feature"
    "touch" = "feature"
    "sensor" = "feature"
    "ai" = "feature"
    "access_control" = "feature"
    "device_manager" = "feature"
    "update_manager" = "feature"
    "media_manager" = "feature"
}

# 步骤3: 处理基础层驱动
$driversRoot = "e:\Software_Structure\drivers"
$baseDrivers = @("adc", "can", "display", "dma", "flash", "gpio", "i2c", "power", "pwm", "sdio", "spi", "timer", "uart", "usb", "watchdog")

foreach ($driver in $baseDrivers) {
    $sourceDir = Join-Path -Path $driversRoot -ChildPath $driver
    if (Test-Path $sourceDir) {
        $targetDir = Join-Path -Path $driversRoot -ChildPath "base\$driver"
        
        # 创建目标目录
        if (-not (Test-Path $targetDir)) {
            New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
            Write-Host "创建目录: $targetDir" -ForegroundColor Yellow
        }
        
        # 复制文件
        $files = Get-ChildItem -Path $sourceDir -File
        foreach ($file in $files) {
            $targetPath = Join-Path -Path $targetDir -ChildPath $file.Name
            if (-not (Test-Path $targetPath)) {
                Copy-Item -Path $file.FullName -Destination $targetPath -Force
                Write-Host "复制文件: $($file.Name) -> base\$driver\" -ForegroundColor Cyan
            }
        }
    }
}

# 步骤4: 处理协议层驱动
$protocolDrivers = @("network", "security")

foreach ($driver in $protocolDrivers) {
    $sourceDir = Join-Path -Path $driversRoot -ChildPath $driver
    if (Test-Path $sourceDir) {
        $targetDir = Join-Path -Path $driversRoot -ChildPath "protocol\$driver"
        
        # 创建目标目录
        if (-not (Test-Path $targetDir)) {
            New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
            Write-Host "创建目录: $targetDir" -ForegroundColor Yellow
        }
        
        # 复制文件
        $files = Get-ChildItem -Path $sourceDir -File
        foreach ($file in $files) {
            $targetPath = Join-Path -Path $targetDir -ChildPath $file.Name
            if (-not (Test-Path $targetPath)) {
                Copy-Item -Path $file.FullName -Destination $targetPath -Force
                Write-Host "复制文件: $($file.Name) -> protocol\$driver\" -ForegroundColor Cyan
            }
        }
    }
}

# 步骤5: 处理功能层驱动 (如camera等) - 目前似乎已经在正确位置

# 步骤6: 总结
Write-Host ""
Write-Host "=== 操作完成 ===" -ForegroundColor Green
Write-Host "驱动目录结构已按照接口层次进行了调整!" -ForegroundColor Green
Write-Host "请手动检查结果，并在确认无误后删除原始文件" -ForegroundColor Yellow
