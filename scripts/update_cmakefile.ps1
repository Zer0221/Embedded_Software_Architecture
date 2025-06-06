# 更新 CMakeLists.txt 以添加新实现的模块
# 此脚本将现有的构建系统更新为使用新创建的模块实现文件

# 检查源目录
$srcDir = "e:\Software_Structure\src"
if(-not (Test-Path $srcDir)) {
    Write-Host "源目录 $srcDir 不存在"
    exit 1
}

# 检查 CMakeLists.txt
$cmakeFile = "e:\Software_Structure\CMakeLists.txt"
if(-not (Test-Path $cmakeFile)) {
    Write-Host "CMakeLists.txt 文件 $cmakeFile 不存在"
    exit 1
}

# 读取 CMakeLists.txt 内容
$content = Get-Content -Path $cmakeFile -Raw

# 确保已创建的模块实现文件包含在构建中
$newContent = $content -replace "# 收集源文件(\r?\n)set\(COMMON_SOURCES (.*)\)(\r?\n)", @"
# 收集源文件
set(COMMON_SOURCES "")

# 添加必要的核心源文件
list(APPEND COMMON_SOURCES `${SRC_DIR}/error.c)
list(APPEND COMMON_SOURCES `${SRC_DIR}/app_framework.c)

# 添加模块实现文件
if(ENABLE_MEMORY_MANAGER)
    list(APPEND COMMON_SOURCES `${SRC_DIR}/memory_manager.c)
endif()

if(ENABLE_DRIVER_MANAGER)
    list(APPEND COMMON_SOURCES `${SRC_DIR}/driver_manager.c)
endif()

if(ENABLE_DEVICE_TREE)
    list(APPEND COMMON_SOURCES `${SRC_DIR}/device_tree.c)
endif()

if(ENABLE_MODULE_SUPPORT)
    list(APPEND COMMON_SOURCES `${SRC_DIR}/module_support.c)
endif()

"@

# 写入修改后的内容
$newContent | Set-Content -Path $cmakeFile -NoNewline

Write-Host "已成功更新 CMakeLists.txt 文件，添加了新模块实现文件。"
