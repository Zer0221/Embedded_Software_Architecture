# 清理空目录
# 此脚本检查并删除驱动程序的空目录

# 要检查的旧目录
$oldDirectories = @(
    "e:\Software_Structure\drivers\adc",
    "e:\Software_Structure\drivers\can",
    "e:\Software_Structure\drivers\display",
    "e:\Software_Structure\drivers\dma",
    "e:\Software_Structure\drivers\flash",
    "e:\Software_Structure\drivers\gpio",
    "e:\Software_Structure\drivers\i2c",
    "e:\Software_Structure\drivers\power",
    "e:\Software_Structure\drivers\pwm",
    "e:\Software_Structure\drivers\sdio",
    "e:\Software_Structure\drivers\spi",
    "e:\Software_Structure\drivers\timer",
    "e:\Software_Structure\drivers\uart",
    "e:\Software_Structure\drivers\usb",
    "e:\Software_Structure\drivers\network",
    "e:\Software_Structure\drivers\security"
)

$removedCount = 0

foreach ($dir in $oldDirectories) {
    if (Test-Path $dir) {
        # Check if directory is empty
        $files = Get-ChildItem -Path $dir -Recurse
        if ($files.Count -eq 0) {
            # Directory is empty, delete it
            Remove-Item -Path $dir -Force -Recurse
            Write-Host "Removed empty directory: $dir"
            $removedCount++
        } else {
            Write-Host "Warning: Directory not empty, keeping: $dir"
            # List files in directory
            $files | ForEach-Object {
                Write-Host "  - $($_.FullName)"
            }
        }
    } else {
        Write-Host "Directory does not exist: $dir"
    }
}

Write-Host "Cleanup completed, removed $removedCount empty directories."
