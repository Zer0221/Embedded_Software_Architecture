/**
 * @file sample_device.c
 * @brief 示例设备树节点实现
 */

#include <stdio.h>
#include "common/device_tree.h"
#include "common/error_handling.h"

// 设备属性定义
static device_property_t sample_device_properties[] = {
    {
        .name = "version",
        .type = PROP_TYPE_STRING,
        .value.s = "1.0.0"
    },
    {
        .name = "irq",
        .type = PROP_TYPE_INT,
        .value.i = 42
    },
    {
        .name = "active",
        .type = PROP_TYPE_BOOL,
        .value.b = true
    },
    {
        .name = "address",
        .type = PROP_TYPE_UINT,
        .value.u = 0x40000000
    }
};

// 设备节点定义
static device_node_t sample_device_node = {
    .name = "sample_device",
    .type = DEVICE_TYPE_MISC,
    .status = DEVICE_STATUS_DISABLED,
    .compatible = "vendor,sample-device",
    .parent = NULL,               // 父节点会在注册时设置
    .children = NULL,             // 子节点会在添加时动态分配
    .child_count = 0,
    .properties = sample_device_properties,
    .property_count = sizeof(sample_device_properties) / sizeof(sample_device_properties[0]),
    .driver = NULL,               // 关联的驱动会在系统启动时查找并设置
    .private_data = NULL
};

// 注册设备函数
int register_sample_device(void) {
    int ret;
    
    // 注册设备节点
    ret = device_register_node(&sample_device_node);
    if (ret != 0) {
        printf("Failed to register sample device: %d\n", ret);
        return ret;
    }
    
    // 启用设备
    ret = device_set_status(&sample_device_node, DEVICE_STATUS_ENABLED);
    if (ret != 0) {
        printf("Failed to enable sample device: %d\n", ret);
        return ret;
    }
    
    printf("Sample device registered successfully\n");
    return 0;
}
