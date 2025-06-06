# 驱动迁移和include路径更新脚本
# 该脚本用于将驱动文件移动到按接口层次组织的目录结构中，并更新include路径

# 设置错误处理
$ErrorActionPreference = "Stop"

# 设置日志文件
$logFile = "e:\Software_Structure\scripts\driver_migration_log.txt"
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$logFile = $logFile.Replace(".txt", "_${timestamp}.txt")

# 日志记录函数
function Write-Log {
    param (
        [string]$Message,
        [string]$Type = "INFO"
    )
    
    $logMessage = "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') [$Type] $Message"
    Add-Content -Path $logFile -Value $logMessage
    
    # 同时输出到控制台
    switch ($Type) {
        "INFO" { Write-Host $Message }
        "SUCCESS" { Write-Host $Message -ForegroundColor Green }
        "WARNING" { Write-Host $Message -ForegroundColor Yellow }
        "ERROR" { Write-Host $Message -ForegroundColor Red }
        default { Write-Host $Message }
    }
}

Write-Log "=== 开始驱动目录结构调整和include路径更新 ===" "SUCCESS"
Write-Log ""

# 步骤1: 确保目标目录结构存在
$baseDirectories = @(
    "e:\Software_Structure\drivers\base",
    "e:\Software_Structure\drivers\protocol",
    "e:\Software_Structure\drivers\feature"
)

foreach ($dir in $baseDirectories) {
    if (-not (Test-Path $dir)) {
        New-Item -Path $dir -ItemType Directory -Force | Out-Null
        Write-Log "创建目录: $dir" "INFO"
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

# 步骤3: 查找所有驱动文件
Write-Log "查找驱动文件..." "INFO"
$driverFiles = Get-ChildItem -Path "e:\Software_Structure\drivers" -Include "*.c" -Recurse -File

Write-Log "找到 $($driverFiles.Count) 个驱动文件" "INFO"
Write-Log ""

# 步骤4: 创建迁移计划
$migrationPlan = @()

foreach ($file in $driverFiles) {
    # 确定驱动类型
    $fileName = $file.Name
    $driverType = $null
    
    # 提取驱动类型 (例如: fm33lc0xx_adc.c -> adc)
    foreach ($driver in $driverLayerMapping.Keys) {
        if ($fileName -match "_${driver}\.c$" -or $fileName -match "${driver}_") {
            $driverType = $driver
            break
        }
    }
    
    # 如果无法确定驱动类型，尝试从目录名确定
    if (-not $driverType) {
        $parentDir = Split-Path -Leaf (Split-Path -Parent $file.FullName)
        if ($driverLayerMapping.ContainsKey($parentDir)) {
            $driverType = $parentDir
        }
    }
      # 如果仍然无法确定，跳过此文件
    if (-not $driverType) {
        Write-Log "警告: 无法确定文件 '$fileName' 的驱动类型，跳过处理" "WARNING"
        continue
    }
    
    # 确定驱动层级
    $layer = $driverLayerMapping[$driverType]
    
    # 创建目标目录
    $targetDir = "e:\Software_Structure\drivers\$layer\$driverType"
    if (-not (Test-Path $targetDir)) {
        New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
    }
    
    # 构建目标文件路径
    $targetPath = Join-Path -Path $targetDir -ChildPath $fileName
    
    # 将文件添加到迁移计划中
    $migrationPlan += @{
        SourcePath = $file.FullName
        TargetPath = $targetPath
        DriverType = $driverType
        Layer = $layer
        FileName = $fileName
    }
}

# 步骤5: 显示迁移计划
Write-Log "迁移计划:" "INFO"

# 按层级组织迁移计划
$planByLayer = @{}
foreach ($item in $migrationPlan) {
    if (-not $planByLayer.ContainsKey($item.Layer)) {
        $planByLayer[$item.Layer] = @()
    }
    $planByLayer[$item.Layer] += $item
}

# 显示每个层级的迁移文件
foreach ($layer in $planByLayer.Keys | Sort-Object) {
    $count = $planByLayer[$layer].Count
    Write-Log "[$layer] 层级 - $count 个文件:" "INFO"
    
    foreach ($item in $planByLayer[$layer]) {
        if ($item.SourcePath -ne $item.TargetPath) {
            Write-Log "  $($item.FileName) -> $($item.Layer)/$($item.DriverType)/" "INFO"
        }
    }
}

Write-Log ""
Write-Log "是否执行迁移计划? (Y/N)" "WARNING"
$confirmation = Read-Host

if ($confirmation -ne "Y" -and $confirmation -ne "y") {
    Write-Log "操作已取消" "ERROR"
    exit
}

# 步骤6: 执行迁移计划
$migratedCount = 0
$updatedCount = 0
$errorCount = 0
$skippedCount = 0

Write-Log "开始执行文件迁移..." "SUCCESS"

foreach ($item in $migrationPlan) {
    try {
        # 如果源路径和目标路径不同，移动文件
        if ($item.SourcePath -ne $item.TargetPath) {
            # 检查目标文件是否已经存在
            if (Test-Path $item.TargetPath) {
                Write-Log "警告: 目标文件 $($item.TargetPath) 已存在，跳过移动" "WARNING"
                $skippedCount++
            } else {
                # 创建目标目录(如果不存在)
                $targetDir = Split-Path -Parent $item.TargetPath
                if (-not (Test-Path $targetDir)) {
                    New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
                    Write-Log "创建目录: $targetDir" "INFO"
                }
                
                # 复制文件到新位置
                Copy-Item -Path $item.SourcePath -Destination $item.TargetPath -Force
                Write-Log "已复制: $($item.SourcePath) -> $($item.TargetPath)" "SUCCESS"
                $migratedCount++
            }
        }
          # 更新include路径
        $content = Get-Content -Path $item.TargetPath -Raw -ErrorAction SilentlyContinue
        if ($content) {
            $originalContent = $content
            $includeChanges = 0
            
            # 更新基础层接口include路径
            $baseApis = @(
                "adc_api.h", "can_api.h", "gpio_api.h", "i2c_api.h", "spi_api.h", "uart_api.h", 
                "pwm_api.h", "timer_api.h", "flash_api.h", "dma_api.h", "display_api.h", 
                "power_api.h", "cpu_freq_api.h", "platform_api.h", "watchdog_api.h", 
                "interrupt_api.h", "usb_api.h", "sdio_api.h", "filesystem_api.h"
            )
            foreach ($api in $baseApis) {
                $pattern = "#include\s+`"$api`""
                $replacement = "#include `"base/$api`""
                if ($content -match $pattern) {
                    $content = $content -replace $pattern, $replacement
                    $includeChanges++
                    Write-Log "  更新include: $pattern -> $replacement" "INFO"
                }
            }
            
            # 更新协议层接口include路径
            $protocolApis = @(
                "network_api.h", "http_client_api.h", "https_server_api.h", "mqtt_client_api.h", 
                "websocket_api.h", "bluetooth_api.h", "coap_client_api.h", "tls_api.h", 
                "device_auth_api.h", "ota_api.h", "protocol_converter_api.h"
            )
            foreach ($api in $protocolApis) {
                $pattern = "#include\s+`"$api`""
                $replacement = "#include `"protocol/$api`""
                if ($content -match $pattern) {
                    $content = $content -replace $pattern, $replacement
                    $includeChanges++
                    Write-Log "  更新include: $pattern -> $replacement" "INFO"
                }
            }
              # 更新功能层接口include路径
            $featureApis = @(
                "camera_api.h", "biometric_api.h", "security_api.h", "audio_api.h", 
                "media_manager_api.h", "touch_api.h", "sensor_api.h", "ai_api.h", 
                "access_control_api.h", "device_manager_api.h", "update_manager_api.h"
            )
            foreach ($api in $featureApis) {
                $pattern = "#include\s+`"$api`""
                $replacement = "#include `"feature/$api`""
                if ($content -match $pattern) {
                    $content = $content -replace $pattern, $replacement
                    $includeChanges++
                    Write-Log "  更新include: $pattern -> $replacement" "INFO"
                }
            }
            
            # 更新通用层接口include路径
            $commonApis = @(
                "error_api.h", "driver_api.h", "log_api.h", "config.h", "json_api.h", 
                "rtos_api.h", "event_bus_api.h", "app_framework.h", "unit_test.h"
            )
            foreach ($api in $commonApis) {
                $pattern = "#include\s+`"$api`""
                $replacement = "#include `"common/$api`""
                if ($content -match $pattern) {
                    $content = $content -replace $pattern, $replacement
                    $includeChanges++
                    Write-Log "  更新include: $pattern -> $replacement" "INFO"
                }
            }
            
            # 如果内容有变更，写回文件
            if ($content -ne $originalContent) {
                Set-Content -Path $item.TargetPath -Value $content
                Write-Log "更新文件include路径: $($item.TargetPath) ($includeChanges 处更改)" "SUCCESS"
                $updatedCount++
            }
        }
    }
    catch {
        Write-Log "错误: 处理文件 $($item.FileName) 时出错: $_" "ERROR"
        $errorCount++
    }
}

# 步骤7: 询问是否删除原始文件
Write-Host ""
Write-Host "是否删除原始文件? (Y/N)" -ForegroundColor Yellow
$deleteConfirmation = Read-Host

if ($deleteConfirmation -eq "Y" -or $deleteConfirmation -eq "y") {
    $deletedCount = 0
    
    # 创建要删除的文件列表
    $filesToDelete = @()
    foreach ($item in $migrationPlan) {
        if ($item.SourcePath -ne $item.TargetPath) {
            # 检查目标文件是否存在
            if (Test-Path $item.TargetPath) {
                $filesToDelete += $item
            }
        }
    }
    
    # 显示即将删除的文件
    Write-Host "即将删除以下 $($filesToDelete.Count) 个文件:" -ForegroundColor Yellow
    foreach ($item in $filesToDelete) {
        Write-Host "  $($item.SourcePath)" -ForegroundColor Gray
    }
    
    Write-Host "确认删除这些文件? (Y/N)" -ForegroundColor Red
    $finalConfirm = Read-Host
    
    if ($finalConfirm -eq "Y" -or $finalConfirm -eq "y") {
        foreach ($item in $filesToDelete) {
            try {
                # 删除原始文件
                Remove-Item -Path $item.SourcePath -Force
                Write-Host "已删除: $($item.SourcePath)" -ForegroundColor Gray
                $deletedCount++
            }
            catch {
                Write-Host "错误: 删除文件 $($item.SourcePath) 时出错: $_" -ForegroundColor Red
            }
        }
    }
    
    Write-Host "已删除 $deletedCount 个原始文件" -ForegroundColor Green
}

# 步骤8: 清理空目录
Write-Host ""
Write-Host "是否清理空目录? (Y/N)" -ForegroundColor Yellow
$cleanupConfirmation = Read-Host

if ($cleanupConfirmation -eq "Y" -or $cleanupConfirmation -eq "y") {
    $driversRoot = "e:\Software_Structure\drivers"
    $emptyDirsRemoved = 0
    
    # 递归查找并删除空目录
    function Remove-EmptyDirs {
        param (
            [string]$Path
        )
        
        $emptyDirsCount = 0
        
        # 获取所有子目录
        $dirs = Get-ChildItem -Path $Path -Directory
        
        foreach ($dir in $dirs) {
            # 递归处理子目录
            $emptyDirsCount += Remove-EmptyDirs -Path $dir.FullName
            
            # 检查目录是否为空
            $items = Get-ChildItem -Path $dir.FullName -Force
            if ($items.Count -eq 0) {
                try {
                    Remove-Item -Path $dir.FullName -Force
                    Write-Host "删除空目录: $($dir.FullName)" -ForegroundColor Gray
                    $emptyDirsCount++
                }
                catch {
                    Write-Host "错误: 删除目录 $($dir.FullName) 时出错: $_" -ForegroundColor Red
                }
            }
        }
        
        return $emptyDirsCount
    }
    
    $emptyDirsRemoved = Remove-EmptyDirs -Path $driversRoot
    Write-Host "已删除 $emptyDirsRemoved 个空目录" -ForegroundColor Green
}

# 步骤9: 输出总结
Write-Host ""
Write-Host "=== 操作完成 ===" -ForegroundColor Green
Write-Host "迁移文件: $migratedCount 个" -ForegroundColor Green
Write-Host "更新include路径: $updatedCount 个" -ForegroundColor Green
if ($errorCount -gt 0) {
    Write-Host "处理出错: $errorCount 个" -ForegroundColor Red
}
Write-Host ""
Write-Host "驱动目录结构已按照接口层次进行了调整!" -ForegroundColor Green
