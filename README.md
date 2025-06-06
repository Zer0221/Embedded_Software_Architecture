# 嵌入式设备软件平台化架构（分层重构版）

## 项目概述

本项目实现了一个嵌入式设备的软件平台化架构，并进行了分层重构，将接口按功能和抽象级别分为四个主要层次：

1. **基础层（Base Layer）**：低级硬件接口，如UART、GPIO、ADC等
2. **协议层（Protocol Layer）**：通信协议接口，如HTTP、MQTT、蓝牙等
3. **功能层（Feature Layer）**：高级功能接口，如摄像头、生物识别、访问控制等
4. **通用层（Common Layer）**：通用工具和定义，如驱动API、错误处理、日志等

这种分层设计使代码更加模块化，便于维护和扩展，同时也使得接口之间的依赖关系更加清晰。

## 架构设计

整个架构分为以下几层：

1. **应用层**：实现具体的应用功能，通过调用抽象接口与底层硬件交互。
2. **功能层**：提供高级功能，如摄像头、生物识别等。
3. **协议层**：提供通信协议接口，如HTTP、MQTT等。
4. **基础层**：提供低级硬件接口，如UART、GPIO、ADC等。
5. **通用层**：提供通用工具和定义，如驱动API、错误处理、日志等。
6. **硬件驱动层**：最底层的硬件驱动代码，通常由芯片厂商提供。

## 文档结构

项目文档已经整理和分类，存放在`docs/`目录中：

```bash
docs/
  ├── architecture/  # 软件架构相关文档
  │   ├── 软件架构重构最终总结报告.md  # 重构的最终总结报告
  │   ├── 分层软件架构设计.md          # 分层架构设计文档
  │   └── 系统架构设计.md              # 系统整体架构设计
  │
  ├── guides/        # 用户指南和使用手册
  │   ├── 移植指南_新.md               # 综合移植指南
  │   ├── 显示驱动指南_新.md           # 显示驱动使用和移植指南
  │   └── 电源管理模块使用指南.md      # 电源管理模块指南
  │
  ├── api/           # API接口文档
  │   └── API使用指南.md               # 系统API使用说明
  │
  └── backup/        # 旧版文档备份（仅供参考）
```
```bash
更多详细信息，请查看 [文档目录](docs/README.md)。
│   ├── esp32_example.c # ESP32示例应用
│   └── esp32_adc_pwm_example.c # ESP32 ADC和PWM示例应用
├── build/             # 编译输出目录
├── docs/              # 文档
│   ├── 分层软件架构设计.md  # 分层架构设计文档
│   ├── 综合移植指南.md     # 架构移植指南
│   ├── 显示驱动指南.md     # 显示驱动指南
│   └── 系统架构设计.md     # 系统架构设计
├── drivers/           # 驱动实现
│   ├── base/          # 基础层驱动实现
│   │   ├── adc/       # ADC驱动
│   │   ├── gpio/      # GPIO驱动
│   │   ├── uart/      # UART驱动
│   │   └── ...
│   ├── protocol/      # 协议层驱动实现
│   │   ├── network/   # 网络驱动
│   │   └── ...
│   └── feature/       # 功能层驱动实现
│       ├── camera/    # 摄像头驱动
│       ├── security/  # 安全功能驱动
│       └── ...
├── include/           # 接口头文件
│   ├── base/          # 基础层接口
│   │   ├── adc_api.h  # ADC接口
│   │   ├── gpio_api.h # GPIO接口
│   │   └── ...
│   ├── protocol/      # 协议层接口
│   │   ├── http_client_api.h # HTTP客户端接口
│   │   ├── mqtt_client_api.h # MQTT客户端接口
│   │   └── ...
│   ├── feature/       # 功能层接口
│   │   ├── camera_api.h # 摄像头接口
│   │   ├── biometric_api.h # 生物识别接口
│   │   └── ...
│   └── common/        # 通用层接口
│       ├── driver_api.h # 驱动通用接口
│       ├── error_api.h  # 错误码定义
│       └── ...
│   ├── gpio_api.h     # GPIO接口
│   ├── i2c_api.h      # I2C接口
│   ├── platform_api.h # 平台接口
│   ├── power_api.h    # 电源管理接口
│   ├── pwm_api.h      # PWM接口
│   ├── rtos_api.h     # RTOS接口
│   ├── unit_test.h    # 单元测试框架接口
│   ├── spi_api.h      # SPI接口
│   └── uart_api.h     # UART接口
├── platform/          # 平台适配层
│   └── mcu/           # MCU平台
│       ├── esp32_platform.c    # ESP32平台实现
│       ├── esp32_platform.h    # ESP32平台定义
│       ├── stm32_platform.c    # STM32平台实现
│       └── stm32_platform.h    # STM32平台定义
├── rtos/              # RTOS适配层
│   ├── api/           # RTOS API定义
│   ├── freertos/      # FreeRTOS适配
│   │   └── freertos_adapter.c  # FreeRTOS适配实现
│   ├── threadx/       # ThreadX适配
│   │   └── threadx_adapter.c   # ThreadX适配实现
│   └── ucos/          # μC/OS适配
├── src/               # 源代码
│   └── error.c        # 错误处理实现
└── tests/             # 测试代码
```

## 接口说明

### 平台接口

平台接口封装了与具体MCU平台相关的操作，如初始化、延时等。

主要接口：
- `platform_init()`: 平台初始化
- `platform_deinit()`: 平台去初始化
- `platform_delay_ms()`: 毫秒级延时
- `platform_delay_us()`: 微秒级延时
- `platform_get_time_ms()`: 获取系统运行时间

### RTOS接口

RTOS接口提供了对操作系统功能的统一抽象，包括任务管理、同步、通信等。

主要接口：
- 任务管理：`rtos_thread_create()`, `rtos_thread_delete()`, `rtos_thread_sleep_ms()`
- 信号量：`rtos_sem_create()`, `rtos_sem_take()`, `rtos_sem_give()`
- 互斥锁：`rtos_mutex_create()`, `rtos_mutex_lock()`, `rtos_mutex_unlock()`
- 消息队列：`rtos_queue_create()`, `rtos_queue_send()`, `rtos_queue_receive()`
- 定时器：`rtos_timer_create()`, `rtos_timer_start()`, `rtos_timer_stop()`
- 事件标志组：`rtos_event_group_create()`, `rtos_event_group_set_bits()`, `rtos_event_group_wait_bits()`
- 内存管理：`rtos_malloc()`, `rtos_free()`

### 驱动接口

各种外设驱动的统一接口定义，包括：

- **I2C接口**：`i2c_init()`, `i2c_master_transmit()`, `i2c_master_receive()`, `i2c_mem_read()`, `i2c_mem_write()`
- **UART接口**：`uart_init()`, `uart_transmit()`, `uart_receive()`, `uart_register_rx_callback()`
- **SPI接口**：`spi_init()`, `spi_transfer()`, `spi_transmit()`, `spi_receive()`
- **GPIO接口**：`gpio_init()`, `gpio_write()`, `gpio_read()`, `gpio_toggle()`, `gpio_register_irq_callback()`

## 支持的平台

目前已实现的平台支持：

1. **STM32**：
   - 实现了GPIO、I2C、SPI、UART等驱动
   - 支持STM32F4xx系列MCU

2. **ESP32**：
   - 实现了GPIO、I2C、SPI、UART、ADC、PWM等驱动
   - 基于ESP-IDF框架

3. **FM33LC0xx**：
   - 实现了GPIO驱动和平台抽象层
   - 支持复旦微FM33LC0xx系列MCU

## RTOS支持

目前已实现的RTOS支持：

1. **FreeRTOS**：
   - 实现了线程、信号量、互斥锁、队列、定时器等基本功能
   - 支持事件标志组和内存管理

2. **ThreadX**：
   - 实现了线程、信号量、互斥锁、队列、定时器等基本功能
   - 支持事件标志组和内存池管理

## 驱动支持

目前已实现的驱动支持：

1. **GPIO**：
   - 支持输入/输出模式、中断配置
   - 支持STM32、ESP32、FM33LC0xx平台

2. **I2C**：
   - 支持主机模式、多种传输速率
   - 支持存储器读写操作
   - 支持STM32、ESP32平台

3. **SPI**：
   - 支持主机模式、多种传输速率
   - 支持全双工数据传输
   - 支持STM32、ESP32平台

4. **UART**：
   - 支持多种波特率、数据位、停止位、校验位配置
   - 支持中断接收和DMA传输
   - 支持STM32、ESP32平台

5. **ADC**：
   - 支持单次采样和连续采样
   - 支持多种分辨率和参考电压源
   - 支持原始值到电压值的转换
   - 支持ESP32平台

6. **PWM**：
   - 支持频率和占空比配置
   - 支持多种对齐模式和极性设置
   - 支持脉冲计数和事件回调
   - 支持ESP32平台

## 单元测试

本项目包含完整的单元测试框架，支持：

1. **测试断言**：
   - 支持基本断言、相等性断言、NULL检查等
   - 提供友好的错误报告

2. **测试套件**：
   - 支持测试套件和测试案例的组织
   - 支持套件初始化和清理
   - 支持测试案例初始化和清理

3. **测试报告**：
   - 提供详细的测试统计信息
   - 支持彩色终端输出

## 支持的RTOS

目前已实现的RTOS支持：

1. **FreeRTOS**：
   - 完整实现了RTOS抽象接口
   - 支持任务、信号量、互斥锁、队列、定时器和事件标志组

2. **ThreadX**：
   - 完整实现了RTOS抽象接口
   - 支持任务、信号量、互斥锁、队列、定时器和事件标志组

## 使用示例

### 平台初始化

```c
/* 初始化平台 */
platform_init();

/* 初始化RTOS */
rtos_init();

/* 启动RTOS调度器 */
rtos_start_scheduler();
```

### I2C使用示例

```c
/* I2C配置 */
i2c_config_t i2c_config = {
    .channel = I2C_CHANNEL_0,
    .speed = I2C_SPEED_STANDARD,
    .addr_10bit = false
};

/* I2C句柄 */
i2c_handle_t i2c_handle;

/* 初始化I2C */
i2c_init(&i2c_config, &i2c_handle);

/* 读取传感器寄存器数据 */
uint8_t reg_addr = 0x00;
uint8_t data[10];
i2c_mem_read(i2c_handle, 0x68, reg_addr, 1, data, 10, 100);
```

### ESP32示例应用

ESP32平台上的完整示例可参考 `applications/esp32_example.c`，该示例展示了：

1. 多任务创建和管理
2. GPIO LED控制
3. I2C传感器数据读取
4. UART通信
5. SPI设备控制
6. 互斥锁保护共享数据

## 移植指南

请参考 `docs/移植指南.md` 获取详细的移植步骤和方法。移植主要包含以下几个部分：

1. 平台适配层移植
2. RTOS适配层移植
3. 驱动适配层移植

## API使用指南

详细的API使用方法请参考 `docs/API使用指南.md`。
