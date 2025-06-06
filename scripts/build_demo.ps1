# 构建并运行演示程序

# 检查目录
$softwareDir = "e:\Software_Structure"
if(-not (Test-Path $softwareDir)) {
    Write-Host "软件目录 $softwareDir 不存在"
    exit 1
}

# 创建构建目录
$buildDir = "$softwareDir\build\demo"
if(-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# 进入构建目录
Set-Location $buildDir

# 配置CMake
Write-Host "配置构建系统..."
cmake -G "MinGW Makefiles" `
    -DTARGET_STM32=ON `
    -DUSE_RTOS_FREERTOS=ON `
    -DENABLE_ERROR_HANDLING=ON `
    -DENABLE_DRIVER_MANAGER=ON `
    -DENABLE_MEMORY_MANAGER=ON `
    -DENABLE_DEVICE_TREE=ON `
    -DENABLE_MODULE_SUPPORT=ON `
    -DENABLE_UNIT_TEST=ON `
    -DAPP_SENSOR=OFF `
    $softwareDir

# 构建项目
Write-Host "构建项目..."
cmake --build .

Write-Host "构建完成."
