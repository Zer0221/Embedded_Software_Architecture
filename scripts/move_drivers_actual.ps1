# 移动驱动程序文件
# 这个脚本实际移动文件到新的目录结构，而不是仅仅复制

# 基础层驱动
$baseDrivers = @(
    @{src="e:\Software_Structure\drivers\adc\*"; dst="e:\Software_Structure\drivers\base\adc\"},
    @{src="e:\Software_Structure\drivers\can\*"; dst="e:\Software_Structure\drivers\base\can\"},
    @{src="e:\Software_Structure\drivers\display\*"; dst="e:\Software_Structure\drivers\base\display\"},
    @{src="e:\Software_Structure\drivers\dma\*"; dst="e:\Software_Structure\drivers\base\dma\"},
    @{src="e:\Software_Structure\drivers\flash\*"; dst="e:\Software_Structure\drivers\base\flash\"},
    @{src="e:\Software_Structure\drivers\gpio\*"; dst="e:\Software_Structure\drivers\base\gpio\"},
    @{src="e:\Software_Structure\drivers\i2c\*"; dst="e:\Software_Structure\drivers\base\i2c\"},
    @{src="e:\Software_Structure\drivers\power\*"; dst="e:\Software_Structure\drivers\base\power\"},
    @{src="e:\Software_Structure\drivers\pwm\*"; dst="e:\Software_Structure\drivers\base\pwm\"},
    @{src="e:\Software_Structure\drivers\sdio\*"; dst="e:\Software_Structure\drivers\base\sdio\"},
    @{src="e:\Software_Structure\drivers\spi\*"; dst="e:\Software_Structure\drivers\base\spi\"},
    @{src="e:\Software_Structure\drivers\timer\*"; dst="e:\Software_Structure\drivers\base\timer\"},
    @{src="e:\Software_Structure\drivers\uart\*"; dst="e:\Software_Structure\drivers\base\uart\"},
    @{src="e:\Software_Structure\drivers\usb\*"; dst="e:\Software_Structure\drivers\base\usb\"}
)

# 协议层驱动
$protocolDrivers = @(
    @{src="e:\Software_Structure\drivers\network\*"; dst="e:\Software_Structure\drivers\protocol\network\"}
)

# 功能层驱动
$featureDrivers = @(
    @{src="e:\Software_Structure\drivers\security\*"; dst="e:\Software_Structure\drivers\feature\security\"}
)

# 确保目标目录存在
foreach ($driver in $baseDrivers + $protocolDrivers + $featureDrivers) {
    if (-not (Test-Path $driver.dst)) {
        New-Item -ItemType Directory -Path $driver.dst -Force | Out-Null
        Write-Host "Created directory: $($driver.dst)"
    }
}

# 实际移动文件（不仅仅是复制）
foreach ($driver in $baseDrivers + $protocolDrivers + $featureDrivers) {
    $srcPath = $driver.src
    $dstPath = $driver.dst
    
    # 获取源目录和目标目录
    $srcDir = Split-Path -Parent $srcPath
    
    if (Test-Path $srcPath) {
        # 确保源目录中确实有文件
        $files = Get-ChildItem -Path $srcPath
        if ($files.Count -gt 0) {
            # 移动文件
            $files | ForEach-Object {
                $destFile = Join-Path -Path $dstPath -ChildPath $_.Name
                Move-Item -Path $_.FullName -Destination $destFile -Force
                Write-Host "Moved: $($_.FullName) -> $destFile"
            }
        } else {
            Write-Host "No files found in: $srcPath"
        }
    } else {
        Write-Host "Source path not found: $srcPath"
    }
}

Write-Host "Driver migration completed."
