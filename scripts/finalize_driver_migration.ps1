# 最终完成驱动目录迁移的脚本
# 该脚本复制剩余的驱动文件到分层结构中，并清理空目录

# 设置错误处理
$ErrorActionPreference = "Stop"

Write-Host "=== 开始最终驱动目录结构调整 ===" -ForegroundColor Green
Write-Host ""

# 步骤1: 确保所有基础层驱动正确迁移
$baseDrivers = @(
    "can", "display", "dma", "flash", "gpio", "i2c", 
    "power", "pwm", "sdio", "spi", "timer", "uart", "usb"
)

foreach ($driver in $baseDrivers) {
    $sourceDir = "e:\Software_Structure\drivers\$driver"
    $targetDir = "e:\Software_Structure\drivers\base\$driver"
    
    if (Test-Path $sourceDir) {
        # 确保目标目录存在
        if (-not (Test-Path $targetDir)) {
            New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
            Write-Host "创建目录: $targetDir" -ForegroundColor Yellow
        }
        
        # 复制所有源文件
        $files = Get-ChildItem -Path $sourceDir -File -Filter "*.c"
        foreach ($file in $files) {
            $targetPath = Join-Path -Path $targetDir -ChildPath $file.Name
            Copy-Item -Path $file.FullName -Destination $targetPath -Force
            Write-Host "复制文件: $($file.Name) -> base\$driver\" -ForegroundColor Cyan
        }
    }
}

# 步骤2: 清理空目录 (递归查找并删除空目录)
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

# 创建驱动文件备份目录
$backupDir = "e:\Software_Structure\drivers\backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
New-Item -Path $backupDir -ItemType Directory -Force | Out-Null
Write-Host "创建驱动备份目录: $backupDir" -ForegroundColor Yellow

# 备份原目录中的所有文件
$originalDirs = @("adc", "can", "display", "dma", "flash", "gpio", "i2c", 
                "network", "power", "security", "pwm", "sdio", "spi", "timer", "uart", "usb")

foreach ($dir in $originalDirs) {
    $sourceDir = "e:\Software_Structure\drivers\$dir"
    if (Test-Path $sourceDir) {
        $targetDir = "$backupDir\$dir"
        Copy-Item -Path $sourceDir -Destination $targetDir -Recurse -Force
        Write-Host "备份目录: $dir -> backup" -ForegroundColor Cyan
    }
}

Write-Host ""
Write-Host "是否继续清理空目录? (Y/N)" -ForegroundColor Yellow
$cleanupConfirmation = Read-Host

if ($cleanupConfirmation -eq "Y" -or $cleanupConfirmation -eq "y") {
    $driversRoot = "e:\Software_Structure\drivers"
    $emptyDirsRemoved = Remove-EmptyDirs -Path $driversRoot
    Write-Host "已删除 $emptyDirsRemoved 个空目录" -ForegroundColor Green
}

Write-Host ""
Write-Host "=== 操作完成 ===" -ForegroundColor Green
Write-Host "驱动目录结构已最终调整!" -ForegroundColor Green
