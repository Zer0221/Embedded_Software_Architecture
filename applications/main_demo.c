/**
 * @file main_demo.c
 * @brief 演示主程序 - 展示所有模块的使用
 */

#include <stdio.h>
#include "common/project_config.h"
#include "common/memory_manager.h"
#include "common/driver_manager.h"
#include "common/device_tree.h"
#include "common/module_support.h"
#include "common/error_handling.h"

// 外部函数声明
extern void run_memory_manager_examples(void);
extern int register_sample_device(void);

// 主函数
int main(void) {
    int ret;
    
    printf("\n====================================\n");
    printf("  软件架构演示程序\n");
    printf("====================================\n\n");
    
    // 初始化错误处理系统
    printf("初始化错误处理系统...\n");
    error_handling_init();
    
    // 初始化内存管理器
    printf("初始化内存管理器...\n");
    ret = mem_init();
    if (ret != 0) {
        printf("内存管理器初始化失败: %d\n", ret);
        return ret;
    }
    
    // 初始化驱动管理器
    printf("初始化驱动管理器...\n");
    ret = driver_manager_init();
    if (ret != 0) {
        printf("驱动管理器初始化失败: %d\n", ret);
        return ret;
    }
    
    // 初始化设备树
    printf("初始化设备树...\n");
    ret = device_tree_init();
    if (ret != 0) {
        printf("设备树初始化失败: %d\n", ret);
        return ret;
    }
    
    // 初始化模块系统
    printf("初始化模块系统...\n");
    ret = module_system_init();
    if (ret != 0) {
        printf("模块系统初始化失败: %d\n", ret);
        return ret;
    }
    
    // 注册示例设备
    printf("注册示例设备...\n");
    ret = register_sample_device();
    if (ret != 0) {
        printf("注册示例设备失败: %d\n", ret);
        return ret;
    }
    
    // 初始化所有驱动
    printf("初始化所有驱动...\n");
    ret = driver_init_all();
    if (ret != 0) {
        printf("驱动初始化失败: %d\n", ret);
        return ret;
    }
    
    // 初始化并启动所有模块
    printf("初始化所有模块...\n");
    ret = module_init_all();
    if (ret != 0) {
        printf("模块初始化失败: %d\n", ret);
        // 继续执行，不返回错误
    }
    
    printf("启动所有模块...\n");
    ret = module_start_all();
    if (ret != 0) {
        printf("模块启动失败: %d\n", ret);
        // 继续执行，不返回错误
    }
    
    // 运行内存管理器示例
    printf("\n运行内存管理器示例...\n");
    run_memory_manager_examples();
    
    // 获取并打印驱动信息
    printf("\n驱动信息:\n");
    driver_info_t *drivers[10];
    uint8_t driver_count;
    if (driver_get_all(drivers, 10, &driver_count) == 0) {
        printf("已注册驱动数量: %d\n", driver_count);
        for (uint8_t i = 0; i < driver_count; i++) {
            printf("  [%d] %s (%s) - 版本: %s, 状态: %d\n", 
                   i, drivers[i]->name, drivers[i]->description, 
                   drivers[i]->version, drivers[i]->status);
        }
    }
    
    // 获取并打印设备信息
    printf("\n设备信息:\n");
    device_node_t *device_nodes[10];
    uint8_t device_count;
    if (device_find_nodes_by_type(DEVICE_TYPE_MISC, device_nodes, 10, &device_count) == 0) {
        printf("杂项设备数量: %d\n", device_count);
        for (uint8_t i = 0; i < device_count; i++) {
            printf("  [%d] %s - 兼容性: %s, 状态: %d\n", 
                   i, device_nodes[i]->name, device_nodes[i]->compatible, 
                   device_nodes[i]->status);
            
            // 打印设备属性
            printf("    属性:\n");
            for (uint8_t j = 0; j < device_nodes[i]->property_count; j++) {
                device_property_t *prop = &device_nodes[i]->properties[j];
                printf("      %s: ", prop->name);
                
                switch (prop->type) {
                    case PROP_TYPE_INT:
                        printf("%d (整型)\n", prop->value.i);
                        break;
                    case PROP_TYPE_UINT:
                        printf("%u (无符号整型)\n", prop->value.u);
                        break;
                    case PROP_TYPE_BOOL:
                        printf("%s (布尔型)\n", prop->value.b ? "true" : "false");
                        break;
                    case PROP_TYPE_STRING:
                        printf("%s (字符串)\n", prop->value.s);
                        break;
                    case PROP_TYPE_ARRAY:
                        printf("[数组，大小: %u]\n", prop->value.a.size);
                        break;
                    case PROP_TYPE_POINTER:
                        printf("%p (指针)\n", prop->value.p);
                        break;
                    default:
                        printf("未知类型\n");
                        break;
                }
            }
        }
    }
    
    // 获取并打印模块信息
    printf("\n模块信息:\n");
    module_info_t *modules[10];
    uint8_t module_count;
    if (module_get_all(modules, 10, &module_count) == 0) {
        printf("已注册模块数量: %d\n", module_count);
        for (uint8_t i = 0; i < module_count; i++) {
            printf("  [%d] %s (%s) - 版本: %s, 优先级: %d, 状态: %d\n", 
                   i, modules[i]->name, modules[i]->description, 
                   modules[i]->version, modules[i]->priority, modules[i]->status);
            
            // 打印模块依赖
            if (modules[i]->dependency_count > 0) {
                printf("    依赖项:\n");
                for (uint8_t j = 0; j < modules[i]->dependency_count; j++) {
                    printf("      %s (%s)\n", 
                           modules[i]->dependencies[j].name,
                           modules[i]->dependencies[j].optional ? "可选" : "必需");
                }
            }
        }
    }
    
    // 停止所有模块
    printf("\n停止所有模块...\n");
    module_stop_all();
    
    printf("\n演示程序结束.\n");
    return 0;
}
