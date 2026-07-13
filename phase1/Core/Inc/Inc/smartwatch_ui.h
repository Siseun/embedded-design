/*
 * smartwatch_ui.h
 *
 *  Created on: Jul 3, 2026
 *      Author: Administrator
 */

#ifndef __SMARTWATCH_UI_H
#define __SMARTWATCH_UI_H

#include "main.h"
#include "mpu6050.h"
#include <stdint.h>

/* UI Pages */
typedef enum {
    PAGE_WATCH_FACE = 0,
    PAGE_IMU,
    PAGE_DEVICE_INFO,
	PAGE_BLUETOOTH,      // 新增蓝牙页
    PAGE_MAX
} UIPage_t;

/* Device data (only real hardware: MPU6050) */
typedef struct {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  weekday;       /* 0=Sun, 1=Mon ... 6=Sat */
    uint8_t  battery_pct;
    int8_t   temp_celsius;  /* from MPU6050 */
    /* MPU6050 sensor data */
    uint8_t  imu_status;        /* 1 = detected, 0 = not found */
    uint32_t step_count;          // <--- 新增5
    MPU6050_Accel_t accel;
    MPU6050_Gyro_t  gyro;
    MPU6050_Angle_t angle;

    /* 新增蓝牙相关字段 */
        uint8_t  ble_connected;     /* 0=未连接, 1=已连接 */
        char     ble_info[32];      /* 显示信息，如 "HC-05 USART2 38400" */
} SmartWatchData_t;

/* 应用状态 */
typedef enum {
    APP_STATE_WATCHFACE,
    APP_STATE_MENU,
    APP_STATE_DATA
} AppState_t;

/* 数据页类型（用于叶子节点显示） */
typedef enum {
    DATA_PAGE_TIME,
    DATA_PAGE_DATE,
    DATA_PAGE_STEPS,
    DATA_PAGE_ATTITUDE,
    DATA_PAGE_ACCEL,
    DATA_PAGE_TEMP,
    DATA_PAGE_STEP_COUNT,
    DATA_PAGE_DISTANCE,
    DATA_PAGE_CALORIES,
    DATA_PAGE_SET_TIME,
    DATA_PAGE_BLUETOOTH_PAIR,
    DATA_PAGE_BRIGHTNESS,
    DATA_PAGE_FACTORY_RESET,
    DATA_PAGE_BATTERY,
    DATA_PAGE_FIRMWARE,
    DATA_PAGE_MAC
} DataPage_t;

/* 菜单项结构 */
typedef struct MenuItem {
    const char *title;          // 显示名称
    uint8_t parent;             // 父节点索引（0=根）
    uint8_t child_start;        // 子项起始索引（0表示无）
    uint8_t child_count;        // 子项数量（不含返回项）
    void (*action)(void);       // 叶子节点操作函数
} MenuItem_t;

/* 菜单上下文 */
typedef struct {
    uint8_t current_menu_index; // 当前显示的菜单节点索引
    uint8_t selected;           // 当前选中项在列表中的位置（含返回项）
    uint8_t history[5];         // 导航历史栈
    uint8_t history_depth;
} MenuContext_t;

/* 全局变量（在 c 文件中定义） */
extern AppState_t app_state;
extern DataPage_t current_data_page;
extern MenuContext_t menu_ctx;
extern const MenuItem_t menu_items[];

/* ==================== UI Framework API ==================== */

void UI_InitData(SmartWatchData_t *data);
void UI_DrawPage(UIPage_t page, SmartWatchData_t *data);
void UI_DrawStatusBar(SmartWatchData_t *data);
void UI_DrawPageIndicator(uint8_t current, uint8_t total);
/* 新增绘制函数 */
void UI_DrawMenu(void);
void UI_DrawDataPage(void);

#endif /* __SMARTWATCH_UI_H */
