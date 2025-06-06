# 清理原始drivers目录的PowerShell脚本
# 此脚本将删除已经迁移到新结构的原始驱动目录

# 定义一个函数来备份目录
function Backup-Directory {
    param (
        [string]$sourcePath
    )
    
    $backupRoot = "e:\Software_Structure\backup\drivers"
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $backupPath = "$backupRoot\$timestamp"
    
    # 创建备份目录
    if (-not (Test-Path $backupRoot)) {
        New-Item -Path $backupRoot -ItemType Directory -Force | Out-Null
    }
    New-Item -Path $backupPath -ItemType Directory -Force | Out-Null
    
    # 复制目录到备份位置
    Copy-Item -Path "$sourcePath\*" -Destination $backupPath -Recurse -Force
    Write-Host "已备份 $sourcePath 到 $backupPath" -ForegroundColor Green
    
    return $backupPath
}

# 获取原始驱动目录列表
$originalDriverDirs = @(
    "adc",
    "can",
    "dma",
    "flash",
    "gpio",
    "i2c",
    "power",
    "pwm",
    "sdio",
    "spi",
    "timer",
    "uart",
    "usb",
    "security",
    "network",
    "display"
)

$driverRoot = "e:\Software_Structure\drivers"

# 显示删除前的提示
Write-Host "即将删除以下原始驱动目录:" -ForegroundColor Yellow
foreach ($dir in $originalDriverDirs) {
    $fullPath = Join-Path -Path $driverRoot -ChildPath $dir
    if (Test-Path $fullPath) {
        Write-Host "  - $fullPath" -ForegroundColor Yellow
    }
}

# 自动确认删除
$confirmation = "y"
Write-Host "自动确认删除目录" -ForegroundColor Cyan

if ($confirmation -ne "y") {
    Write-Host "操作已取消" -ForegroundColor Red
    exit
}

# 备份所有驱动目录
$backupPath = Backup-Directory -sourcePath $driverRoot

Write-Host "所有驱动目录已备份到: $backupPath" -ForegroundColor Green

# 删除原始驱动目录
foreach ($dir in $originalDriverDirs) {
    $fullPath = Join-Path -Path $driverRoot -ChildPath $dir
    if (Test-Path $fullPath) {
        Remove-Item -Path $fullPath -Recurse -Force
        Write-Host "已删除: $fullPath" -ForegroundColor Green
    }
    else {
        Write-Host "目录不存在，跳过: $fullPath" -ForegroundColor Yellow
    }
}

Write-Host "清理完成" -ForegroundColor Green
