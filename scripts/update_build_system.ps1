# 更新构建系统以适应新的目录结构
# 此脚本会修改CMakeLists.txt文件，使其适配新的分层目录结构

# 定义颜色输出函数
function Write-ColorOutput {
    param (
        [string]$text,
        [string]$color = "White"
    )
    
    Write-Host $text -ForegroundColor $color
}

# 检查CMakeLists.txt是否存在
$cmakeListsPath = "e:\Software_Structure\CMakeLists.txt"
if (-not (Test-Path $cmakeListsPath)) {
    Write-ColorOutput "错误: 未找到CMakeLists.txt文件" "Red"
    exit 1
}

# 备份原始CMakeLists.txt文件
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$backupPath = "e:\Software_Structure\backup\CMakeLists_${timestamp}.txt"

# 确保备份目录存在
if (-not (Test-Path "e:\Software_Structure\backup")) {
    New-Item -Path "e:\Software_Structure\backup" -ItemType Directory -Force | Out-Null
}

Copy-Item -Path $cmakeListsPath -Destination $backupPath
Write-ColorOutput "已备份原始CMakeLists.txt到: $backupPath" "Green"

# 读取原始CMakeLists.txt内容
$content = Get-Content -Path $cmakeListsPath -Raw

# 修改include路径
$newIncludePaths = @"
# Include directories
include_directories(
    \${CMAKE_CURRENT_SOURCE_DIR}/include
    \${CMAKE_CURRENT_SOURCE_DIR}/include/base
    \${CMAKE_CURRENT_SOURCE_DIR}/include/protocol
    \${CMAKE_CURRENT_SOURCE_DIR}/include/feature
    \${CMAKE_CURRENT_SOURCE_DIR}/include/common
    \${CMAKE_CURRENT_SOURCE_DIR}/platform/mcu
    \${CMAKE_CURRENT_SOURCE_DIR}/rtos/api
)
"@

# 查找并替换include目录部分
$includePattern = "(?s)# Include directories.*?(?=\n\n)"
$content = $content -replace $includePattern, $newIncludePaths

# 修改源文件路径
$newSourceGroups = @"
# Source files
file(GLOB_RECURSE SOURCES
    "src/*.c"
    "drivers/base/*/*.c"
    "drivers/protocol/*/*.c"
    "drivers/feature/*/*.c"
    "drivers/common/*.c"
    "platform/mcu/*/*.c"
    "rtos/*/*.c"
    "applications/*.c"
)
"@

# 查找并替换源文件部分
$sourcePattern = "(?s)# Source files.*?(?=\n\n)"
$content = $content -replace $sourcePattern, $newSourceGroups

# 将修改后的内容写回文件
Set-Content -Path $cmakeListsPath -Value $content
Write-ColorOutput "已更新CMakeLists.txt文件" "Green"

# 输出一些提示信息
Write-ColorOutput "CMakeLists.txt已更新，现在构建系统使用了新的分层目录结构:" "Cyan"
Write-ColorOutput "  - 更新了包含目录路径，添加了分层结构的目录" "Cyan"
Write-ColorOutput "  - 更新了源文件搜索路径，适配新的驱动目录结构" "Cyan"
Write-ColorOutput "" "White"
Write-ColorOutput "注意: 您可能需要重新生成构建缓存:" "Yellow"
Write-ColorOutput "  1. 删除build目录: rm -rf build" "Yellow"
Write-ColorOutput "  2. 重新创建build目录: mkdir build" "Yellow"
Write-ColorOutput "  3. 进入build目录: cd build" "Yellow"
Write-ColorOutput "  4. 重新运行CMake: cmake .." "Yellow"
Write-ColorOutput "  5. 构建项目: cmake --build ." "Yellow"
