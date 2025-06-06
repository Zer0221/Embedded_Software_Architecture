$moves = @(
    @{from="e:\Software_Structure\drivers\uart\esp32_uart.c"; to="e:\Software_Structure\drivers\base\uart\esp32_uart.c"},
    @{from="e:\Software_Structure\drivers\gpio\esp32_gpio.c"; to="e:\Software_Structure\drivers\base\gpio\esp32_gpio.c"},
    @{from="e:\Software_Structure\drivers\gpio\fm33lc0xx_gpio.c"; to="e:\Software_Structure\drivers\base\gpio\fm33lc0xx_gpio.c"},
    @{from="e:\Software_Structure\drivers\gpio\stm32_gpio.c"; to="e:\Software_Structure\drivers\base\gpio\stm32_gpio.c"},
    @{from="e:\Software_Structure\drivers\adc\esp32_adc.c"; to="e:\Software_Structure\drivers\base\adc\esp32_adc.c"},
    @{from="e:\Software_Structure\drivers\adc\fm33lc0xx_adc.c"; to="e:\Software_Structure\drivers\base\adc\fm33lc0xx_adc.c"},
    @{from="e:\Software_Structure\drivers\pwm\esp32_pwm.c"; to="e:\Software_Structure\drivers\base\pwm\esp32_pwm.c"},
    @{from="e:\Software_Structure\drivers\pwm\fm33lc0xx_pwm.c"; to="e:\Software_Structure\drivers\base\pwm\fm33lc0xx_pwm.c"},
    @{from="e:\Software_Structure\drivers\spi\esp32_spi.c"; to="e:\Software_Structure\drivers\base\spi\esp32_spi.c"},
    @{from="e:\Software_Structure\drivers\spi\stm32_spi.c"; to="e:\Software_Structure\drivers\base\spi\stm32_spi.c"},
    @{from="e:\Software_Structure\drivers\i2c\esp32_i2c.c"; to="e:\Software_Structure\drivers\base\i2c\esp32_i2c.c"},
    @{from="e:\Software_Structure\drivers\i2c\stm32_i2c.c"; to="e:\Software_Structure\drivers\base\i2c\stm32_i2c.c"},
    @{from="e:\Software_Structure\drivers\flash\esp32_flash.c"; to="e:\Software_Structure\drivers\base\flash\esp32_flash.c"},
    @{from="e:\Software_Structure\drivers\flash\fm33lc0xx_flash.c"; to="e:\Software_Structure\drivers\base\flash\fm33lc0xx_flash.c"},
    @{from="e:\Software_Structure\drivers\flash\stm32_flash.c"; to="e:\Software_Structure\drivers\base\flash\stm32_flash.c"},
    @{from="e:\Software_Structure\drivers\dma\stm32_dma.c"; to="e:\Software_Structure\drivers\base\dma\stm32_dma.c"},
    @{from="e:\Software_Structure\drivers\timer\fm33lc0xx_timer.c"; to="e:\Software_Structure\drivers\base\timer\fm33lc0xx_timer.c"},
    @{from="e:\Software_Structure\drivers\sdio\esp32_sdio.c"; to="e:\Software_Structure\drivers\base\sdio\esp32_sdio.c"},
    @{from="e:\Software_Structure\drivers\sdio\stm32_sdio.c"; to="e:\Software_Structure\drivers\base\sdio\stm32_sdio.c"},
    @{from="e:\Software_Structure\drivers\usb\esp32_usb.c"; to="e:\Software_Structure\drivers\base\usb\esp32_usb.c"},
    @{from="e:\Software_Structure\drivers\usb\stm32_usb.c"; to="e:\Software_Structure\drivers\base\usb\stm32_usb.c"},
    @{from="e:\Software_Structure\drivers\can\esp32_can.c"; to="e:\Software_Structure\drivers\base\can\esp32_can.c"},
    @{from="e:\Software_Structure\drivers\can\stm32_can.c"; to="e:\Software_Structure\drivers\base\can\stm32_can.c"},
    @{from="e:\Software_Structure\drivers\display\tm1681_driver.c"; to="e:\Software_Structure\drivers\base\display\tm1681_driver.c"},
    @{from="e:\Software_Structure\drivers\power\esp32_power.c"; to="e:\Software_Structure\drivers\base\power\esp32_power.c"},
    @{from="e:\Software_Structure\drivers\power\stm32_power.c"; to="e:\Software_Structure\drivers\base\power\stm32_power.c"},
    @{from="e:\Software_Structure\drivers\network\esp32_wifi.c"; to="e:\Software_Structure\drivers\protocol\network\esp32_wifi.c"},
    @{from="e:\Software_Structure\drivers\security\esp32_security.c"; to="e:\Software_Structure\drivers\feature\security\esp32_security.c"},
    @{from="e:\Software_Structure\drivers\security\stm32_security.c"; to="e:\Software_Structure\drivers\feature\security\stm32_security.c"}
)

foreach ($move in $moves) {
    $fromPath = $move.from
    $toPath = $move.to
    
    if (Test-Path $fromPath) {
        # 确保目标目录存在
        $toDir = Split-Path -Path $toPath -Parent
        if (-not (Test-Path $toDir)) {
            New-Item -ItemType Directory -Path $toDir -Force | Out-Null
        }
        
        # 拷贝文件
        Copy-Item -Path $fromPath -Destination $toPath -Force
        Write-Host "Copied: $fromPath -> $toPath"
    } else {
        Write-Host "Source file not found: $fromPath"
    }
}

Write-Host "File copying complete."
