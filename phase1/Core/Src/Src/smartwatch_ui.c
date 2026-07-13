/*
 * smartwatch_ui.c
 *
 *  Created on: Jul 3, 2026
 *      Author: Administrator
 */

#include "smartwatch_ui.h"
#include "oled.h"
#include <stdio.h>
#include <string.h>
#include "adc.h"

extern SmartWatchData_t watch_data;   // 如果已有，可忽略
extern ADC_HandleTypeDef hadc1;       // ← 新增这一行

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES  8

/* Weekday strings */
static const char *weekdays_en[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

static void OLED_DrawTime16x24(uint8_t x, uint8_t y, uint8_t hour, uint8_t minute) {
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    OLED_PrintASCIIString(x, y, buf, &afont24x12, OLED_COLOR_NORMAL);  // 使用24x12字体
}


/* ==================== Data Init ==================== */

void UI_InitData(SmartWatchData_t *data) {
    data->hour = 10;
    data->minute = 30;
    data->second = 45;
    data->year = 2026;
    data->month = 6;
    data->day = 27;
    data->weekday = 6;       /* Saturday */
    data->battery_pct = 85;
    data->temp_celsius = 25;
    data->imu_status = 0;
    data->accel.ax = 0.0f;
    data->accel.ay = 0.0f;
    data->accel.az = 9.81f;
    data->gyro.gx = 0.0f;
    data->gyro.gy = 0.0f;
    data->gyro.gz = 0.0f;
    data->angle.pitch = 0.0f;
    data->angle.roll = 0.0f;
    data->step_count = 0;
}

/* ==================== Status Bar ==================== */

void UI_DrawStatusBar(SmartWatchData_t *data) {
	// 电池图标（手绘）
	    for (uint8_t y = 0; y < 8; y++) {
	        for (uint8_t x = 0; x < 14; x++) {
	            if (x == 0 || x == 13)
	                OLED_SetPixel(x, y, OLED_COLOR_NORMAL);
	            else if (x == 14 && (y >= 2 && y <= 5))
	                OLED_SetPixel(14, y, OLED_COLOR_NORMAL);
	            else if (x >= 1 && x <= 12 && y >= 1 && y <= 6)
	                OLED_SetPixel(x, y, OLED_COLOR_NORMAL);  // 填充内部（稍后覆盖）
	        }
	    }
	    // 电池电量填充
	    uint8_t fill_width = (data->battery_pct * 10) / 100;
	    if (fill_width > 10) fill_width = 10;
	    for (uint8_t y = 2; y < 6; y++) {
	        for (uint8_t x = 1; x < 1 + fill_width; x++) {
	            OLED_SetPixel(x, y, OLED_COLOR_NORMAL);
	        }
	    }

	    // 电池百分比（使用小号 ASCII 字体）
	    char bat_str[5];
	    snprintf(bat_str, sizeof(bat_str), "%d%%", data->battery_pct);
	    OLED_PrintASCIIString(18, 0, bat_str, &afont8x6, OLED_COLOR_NORMAL);

	    // 时间
	    char time_str[9];
	    snprintf(time_str, sizeof(time_str), "%02d:%02d", data->hour, data->minute);
	    OLED_PrintASCIIString(60, 0, time_str, &afont8x6, OLED_COLOR_NORMAL);

	    // IMU 状态圆点
	    if (data->imu_status) {
	        OLED_DrawFilledCircle(123, 3, 2, OLED_COLOR_NORMAL);
	    } else {
	        OLED_DrawCircle(123, 3, 2, OLED_COLOR_NORMAL);
	    }

	    // 分割线（y=7）
	    OLED_DrawLine(0, 7, 127, 7, OLED_COLOR_NORMAL);
}

/* ==================== Page Indicator ==================== */

void UI_DrawPageIndicator(uint8_t current, uint8_t total) {
	// 清除底部区域（y=56~63）
	    for (uint8_t y = 56; y < 64; y++)
	        for (uint8_t x = 0; x < 128; x++)
	            OLED_SetPixel(x, y, OLED_COLOR_REVERSED);  // 清除

	    // 分割线 y=56
	    OLED_DrawLine(0, 56, 127, 56, OLED_COLOR_NORMAL);

	    // 画点
	    uint8_t dot_spacing = 14;
	    uint8_t total_width = (total - 1) * dot_spacing;
	    uint8_t start_x = (128 - total_width) / 2;
	    for (uint8_t i = 0; i < total; i++) {
	        uint8_t cx = start_x + i * dot_spacing;
	        if (i == current)
	            OLED_DrawFilledCircle(cx, 60, 2, OLED_COLOR_NORMAL);
	        else
	            OLED_DrawCircle(cx, 60, 2, OLED_COLOR_NORMAL);
	    }
}

/* ==================== Page 0: Watch Face ==================== */

void UI_DrawWatchFace(SmartWatchData_t *data) {
	OLED_NewFrame();

    UI_DrawStatusBar(data);

    /* Large time display */
    uint8_t time_x = (128 - 60) / 2;
    OLED_DrawTime16x24(time_x, 30, data->hour, data->minute);

    /* Date line: page 5 */
    char date_str[32];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d %s",
             data->year, data->month, data->day,
             weekdays_en[data->weekday]);
    uint8_t date_len = strlen(date_str);
    uint8_t date_x = (128 - date_len * 6) / 2;
    OLED_PrintASCIIString(date_x, 14, date_str, &afont8x6, OLED_COLOR_NORMAL);

    /* IMU quick status: page 6 */
    char imu_str[32];
    if (data->imu_status) {
    	snprintf(imu_str, sizeof(imu_str), "P:%+3d R:%+3d  T:%dC",
    	         (int)data->angle.pitch, (int)data->angle.roll, data->temp_celsius);
    } else {
        snprintf(imu_str, sizeof(imu_str), "MPU6050: --");
    }

    char step_str[20];
    snprintf(step_str, sizeof(step_str), "Steps: %lu", data->step_count);
    OLED_PrintASCIIString(4, 56, step_str, &afont8x6, OLED_COLOR_NORMAL);

    uint8_t imu_len = strlen(imu_str);
    uint8_t imu_x = (128 - imu_len * 6) / 2;
    OLED_PrintASCIIString(imu_x, 24, imu_str, &afont8x6, OLED_COLOR_NORMAL);

    //UI_DrawPageIndicator(PAGE_WATCH_FACE, PAGE_MAX);
}

/* ==================== Page 1: IMU Sensor Detail ==================== */

void UI_DrawIMU(SmartWatchData_t *data) {
	OLED_NewFrame();

    UI_DrawStatusBar(data);

    OLED_PrintASCIIString(4, 9, "IMU MPU6050", &afont16x8, OLED_COLOR_NORMAL);

    for (uint8_t x = 0; x < SSD1306_WIDTH; x++)
    	OLED_SetPixel(x, 26, OLED_COLOR_NORMAL);

    if (!data->imu_status) {
    	OLED_PrintASCIIString(4, 28, "MPU6050: NOT FOUND",&afont8x6, OLED_COLOR_NORMAL);
    	OLED_PrintASCIIString(4, 36, "PB10/PB11  I2C2",&afont8x6, OLED_COLOR_NORMAL);
    	OLED_PrintASCIIString(4, 44, "Auto-retry every 5s",&afont8x6, OLED_COLOR_NORMAL);
    } else {
        char buf[24];

        /* Accel: "X+0.1 Y+0.3 Z+9.8" = 17 chars = 102px */
        snprintf(buf, sizeof(buf), "X%+d.%d Y%+d.%d Z%+d.%d",
                 (int)data->accel.ax, abs((int)((data->accel.ax - (int)data->accel.ax) * 10)),
                 (int)data->accel.ay, abs((int)((data->accel.ay - (int)data->accel.ay) * 10)),
                 (int)data->accel.az, abs((int)((data->accel.az - (int)data->accel.az) * 10)));
        OLED_PrintASCIIString(0, 28, buf,&afont8x6, OLED_COLOR_NORMAL);

        /* Gyro: "x+0 y+0 z+0 d/s" = 14 chars = 84px */
        snprintf(buf, sizeof(buf), "x%+d y%+d z%+d d/s",
                 (int)data->gyro.gx, (int)data->gyro.gy, (int)data->gyro.gz);
        OLED_PrintASCIIString(0, 36, buf,&afont8x6, OLED_COLOR_NORMAL);

        /* Pitch & Roll: "P+0 R+0 deg" = 12 chars = 72px */
        snprintf(buf, sizeof(buf), "P%+d R%+d deg",
                 (int)data->angle.pitch, (int)data->angle.roll);
        OLED_PrintASCIIString(0, 44, buf,&afont8x6, OLED_COLOR_NORMAL);

        /* Temperature + I2C status */
        snprintf(buf, sizeof(buf), "T:%dC %s",
                 data->temp_celsius,
				 data->imu_status ? "OK" : "ERR");
        OLED_PrintASCIIString(0, 44, buf,&afont8x6, OLED_COLOR_NORMAL);

        /* Attitude indicator (right side, away from text) */
        uint8_t cx = 114, cy = 40, r = 7;

        OLED_DrawCircle(cx, cy, r, OLED_COLOR_NORMAL);
        OLED_DrawLine(cx - r, cy, cx + r, cy, OLED_COLOR_NORMAL);
        OLED_DrawLine(cx, cy - r, cx, cy + r, OLED_COLOR_NORMAL);

        int8_t dot_x = cx + (int8_t)(data->angle.roll * 0.12f);
        int8_t dot_y = cy + (int8_t)(data->angle.pitch * 0.12f);

        if (dot_x < cx - r + 2) dot_x = cx - r + 2;
        if (dot_x > cx + r - 2) dot_x = cx + r - 2;
        if (dot_y < cy - r + 2) dot_y = cy - r + 2;
        if (dot_y > cy + r - 2) dot_y = cy + r - 2;

        OLED_DrawFilledCircle(dot_x, dot_y, 2, OLED_COLOR_NORMAL);

    }

    UI_DrawPageIndicator(PAGE_IMU, PAGE_MAX);
}

/* ==================== Page 2: Device Info ==================== */

void UI_DrawDeviceInfo(SmartWatchData_t *data) {
	OLED_NewFrame();

    UI_DrawStatusBar(data);

    OLED_PrintASCIIString(4, 9, "DEV INFO", &afont8x6, OLED_COLOR_NORMAL);

    for (uint8_t x = 0; x < SSD1306_WIDTH; x++)
    	OLED_SetPixel(x, 16, OLED_COLOR_NORMAL);

    OLED_PrintASCIIString(4, 20, "STM32F103C8T6  72MHz",&afont8x6, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(4, 28, "OLED: I2C1 PB6/PB7",&afont8x6, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(4, 36, "IMU:  I2C2 PB10/PB11",&afont8x6, OLED_COLOR_NORMAL);

    /* MPU6050 status */
    if (data->imu_status) {
    	OLED_PrintASCIIString(4, 46, "MPU6050: OK",&afont8x6, OLED_COLOR_NORMAL);
    } else {
    	OLED_PrintASCIIString(4, 48, "MPU6050: NOT FOUND",&afont8x6, OLED_COLOR_NORMAL);
    }

    UI_DrawPageIndicator(PAGE_DEVICE_INFO, PAGE_MAX);
}

// 新增蓝牙页绘制函数
void UI_DrawBluetoothPage(SmartWatchData_t *data) {
    OLED_NewFrame();
    UI_DrawStatusBar(data);

    OLED_PrintASCIIString(4, 9, "BLUETOOTH", &afont16x8, OLED_COLOR_NORMAL);
    for (uint8_t x = 0; x < SSD1306_WIDTH; x++)
        OLED_SetPixel(x, 26, OLED_COLOR_NORMAL);

    // 显示 HC-05 信息
    OLED_PrintASCIIString(4, 28, data->ble_info, &afont8x6, OLED_COLOR_NORMAL);

    // 显示连接状态
    if (data->ble_connected) {
        OLED_PrintASCIIString(4, 36, "Status: Connected", &afont8x6, OLED_COLOR_NORMAL);
    } else {
        OLED_PrintASCIIString(4, 36, "Status: Disconnected", &afont8x6, OLED_COLOR_NORMAL);
    }
}

/* ==================== Page Dispatcher ==================== */

void UI_DrawPage(UIPage_t page, SmartWatchData_t *data) {
    switch (page) {
        case PAGE_WATCH_FACE:  UI_DrawWatchFace(data);  break;
        case PAGE_IMU:         UI_DrawIMU(data);         break;
        case PAGE_DEVICE_INFO: UI_DrawDeviceInfo(data);  break;
        case PAGE_BLUETOOTH:   UI_DrawBluetoothPage(data); break;
        default: break;
    }
    OLED_ShowFrame();
}

/* ==================== 新增：多级菜单相关 ==================== */

// 引用外部全局数据（在 main.c 中定义）
extern SmartWatchData_t watch_data;

// 应用状态变量定义（已在 .h 中声明为 extern，这里实际定义）
AppState_t app_state = APP_STATE_WATCHFACE;
DataPage_t current_data_page = DATA_PAGE_TIME;
MenuContext_t menu_ctx = {0, 0, {0}, 0};

/* ==================== 叶子动作函数（静态） ==================== */

static void action_show_time(void) {
    current_data_page = DATA_PAGE_TIME;
    app_state = APP_STATE_DATA;
}
static void action_show_date(void) {
    current_data_page = DATA_PAGE_DATE;
    app_state = APP_STATE_DATA;
}
static void action_show_steps(void) {
    current_data_page = DATA_PAGE_STEPS;
    app_state = APP_STATE_DATA;
}
static void action_show_attitude(void) {
    current_data_page = DATA_PAGE_ATTITUDE;
    app_state = APP_STATE_DATA;
}
static void action_show_accel(void) {
    current_data_page = DATA_PAGE_ACCEL;
    app_state = APP_STATE_DATA;
}
static void action_show_temp(void) {
    current_data_page = DATA_PAGE_TEMP;
    app_state = APP_STATE_DATA;
}
static void action_show_step_count(void) {
    current_data_page = DATA_PAGE_STEP_COUNT;
    app_state = APP_STATE_DATA;
}
static void action_show_distance(void) {
    current_data_page = DATA_PAGE_DISTANCE;
    app_state = APP_STATE_DATA;
}
static void action_show_calories(void) {
    current_data_page = DATA_PAGE_CALORIES;
    app_state = APP_STATE_DATA;
}
static void action_set_time(void) {
    current_data_page = DATA_PAGE_SET_TIME;
    app_state = APP_STATE_DATA;
}
static void action_bluetooth_pair(void) {
    current_data_page = DATA_PAGE_BLUETOOTH_PAIR;
    app_state = APP_STATE_DATA;
}
static void action_set_brightness(void) {
    current_data_page = DATA_PAGE_BRIGHTNESS;
    app_state = APP_STATE_DATA;
}
static void action_factory_reset(void) {
    current_data_page = DATA_PAGE_FACTORY_RESET;
    app_state = APP_STATE_DATA;
}
static void action_show_battery(void) {
    current_data_page = DATA_PAGE_BATTERY;
    app_state = APP_STATE_DATA;
}
static void action_show_firmware(void) {
    current_data_page = DATA_PAGE_FIRMWARE;
    app_state = APP_STATE_DATA;
}
static void action_show_mac(void) {
    current_data_page = DATA_PAGE_MAC;
    app_state = APP_STATE_DATA;
}
static void action_back_to_watchface(void) {
    app_state = APP_STATE_WATCHFACE;
}

/* ==================== 菜单数据结构定义 ==================== */

const MenuItem_t menu_items[] = {
    /* 0: 根节点（虚拟，不显示） */
    {NULL, 0, 1, 6, NULL},

    /* 1~6: 主菜单项 */
    {"Time",         0, 7,  3, NULL},
    {"Sensor",       0, 10, 3, NULL},
    {"Motion",       0, 13, 3, NULL},
    {"Settings",     0, 16, 4, NULL},
    {"System",       0, 20, 3, NULL},
    {"Exit",         0, 0,  0, action_back_to_watchface},

    /* 7~9: 时间显示子项 */
    {"Current Time",    1, 0, 0, action_show_time},
    {"Date",            1, 0, 0, action_show_date},
    {"Step Count",      1, 0, 0, action_show_steps},

    /* 10~12: 传感器数据子项 */
    {"Attitude",         2, 0, 0, action_show_attitude},
    {"Accel",            2, 0, 0, action_show_accel},
    {"Temperature",      2, 0, 0, action_show_temp},

    /* 13~15: 运动数据子项 */
    {"Today Steps",    3, 0, 0, action_show_step_count},
    {"Distance",       3, 0, 0, action_show_distance},
    {"Calories",       3, 0, 0, action_show_calories},

    /* 16~19: 设置子项 */
    {"Set Time",          4, 0, 0, action_set_time},
    {"Bluetooth Pair",    4, 0, 0, action_bluetooth_pair},
    {"Brightness",        4, 0, 0, action_set_brightness},
    {"Factory Reset",     4, 0, 0, action_factory_reset},

    /* 20~22: 系统信息子项 */
    {"Battery",          5, 0, 0, action_show_battery},
    {"Firmware",         5, 0, 0, action_show_firmware},
    {"Bluetooth MAC",    5, 0, 0, action_show_mac}
};

/* ==================== 绘制菜单 ==================== */

void UI_DrawMenu(void) {
    OLED_NewFrame();

    const MenuItem_t *cur = &menu_items[menu_ctx.current_menu_index];
    uint8_t child_count = cur->child_count;
    uint8_t has_return = (menu_ctx.history_depth > 0);
    uint8_t total_items = child_count + (has_return ? 1 : 0);
    uint8_t selected = menu_ctx.selected;
    if (selected >= total_items) selected = total_items - 1;

    if (menu_ctx.current_menu_index != 0) {
        // 显示当前菜单标题
        OLED_PrintASCIIString(4, 0, cur->title, &afont8x6, OLED_COLOR_NORMAL);
        OLED_DrawLine(0, 8, 127, 8, OLED_COLOR_NORMAL);
        uint8_t y = 12;
        // 子项列表
        for (uint8_t i = 0; i < child_count; i++) {
            uint8_t idx = cur->child_start + i;
            const char *item_title = menu_items[idx].title;
            if (i == selected) {
                OLED_PrintASCIIString(4, y, ">", &afont8x6, OLED_COLOR_NORMAL);
                OLED_PrintASCIIString(12, y, item_title, &afont8x6, OLED_COLOR_NORMAL);
            } else {
                OLED_PrintASCIIString(12, y, item_title, &afont8x6, OLED_COLOR_NORMAL);
            }
            y += 10;
            if (y > 56) break;
        }
        if (has_return) {
            uint8_t ret_idx = child_count;
            if (ret_idx == selected) {
                OLED_PrintASCIIString(4, y, ">", &afont8x6, OLED_COLOR_NORMAL);
                OLED_PrintASCIIString(12, y, "Exit", &afont8x6, OLED_COLOR_NORMAL);
            } else {
                OLED_PrintASCIIString(12, y, "Exit", &afont8x6, OLED_COLOR_NORMAL);
            }
        }
    } else {
        // 根菜单（不显示标题）
        uint8_t y = 4;
        for (uint8_t i = 0; i < child_count; i++) {
            uint8_t idx = cur->child_start + i;
            const char *item_title = menu_items[idx].title;
            if (i == selected) {
                OLED_PrintASCIIString(4, y, ">", &afont8x6, OLED_COLOR_NORMAL);
                OLED_PrintASCIIString(12, y, item_title, &afont8x6, OLED_COLOR_NORMAL);
            } else {
                OLED_PrintASCIIString(12, y, item_title, &afont8x6, OLED_COLOR_NORMAL);
            }
            y += 10;
            if (y > 56) break;
        }
    }

    OLED_ShowFrame();
}

/* ==================== 绘制数据页 ==================== */

static void draw_data_page_title(const char *title) {
    OLED_PrintASCIIString(4, 0, title, &afont8x6, OLED_COLOR_NORMAL);
    OLED_DrawLine(0, 8, 127, 8, OLED_COLOR_NORMAL);
}

void UI_DrawDataPage(void) {
    OLED_NewFrame();
    char buf[32];

    switch (current_data_page) {
        case DATA_PAGE_TIME:
            draw_data_page_title("Current Time");
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", watch_data.hour, watch_data.minute, watch_data.second);
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_DATE:
            draw_data_page_title("Date");
            snprintf(buf, sizeof(buf), "%04d-%02d-%02d", watch_data.year, watch_data.month, watch_data.day);
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_STEPS:
        case DATA_PAGE_STEP_COUNT:
            draw_data_page_title("Step Count");
            snprintf(buf, sizeof(buf), "%lu", watch_data.step_count);
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_ATTITUDE:
            draw_data_page_title("Attitude");
            snprintf(buf, sizeof(buf), "Pitch: %+3d", (int)watch_data.angle.pitch);
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            snprintf(buf, sizeof(buf), "Roll : %+3d", (int)watch_data.angle.roll);
            OLED_PrintASCIIString(4, 30, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_ACCEL:
            draw_data_page_title("Accel  (m/s²)");
            // X 和 Y 保持原样（或也可改为同样风格，按需决定）
            snprintf(buf, sizeof(buf), "X:%+4d", (int)watch_data.accel.ax);
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            snprintf(buf, sizeof(buf), "Y:%+4d", (int)watch_data.accel.ay);
            OLED_PrintASCIIString(4, 30, buf, &afont8x6, OLED_COLOR_NORMAL);

            // Z 使用“整数部分.小数部分”的整型拆分法，与参考代码完全一致
            float z = watch_data.accel.az;
            int   z_int  = (int)z;                        // 截断取整数部分（带符号）
            int   z_frac = abs((int)((z - z_int) * 10));  // 一位小数，取绝对值
            snprintf(buf, sizeof(buf), "Z:%+4d.%d", z_int, z_frac);
            OLED_PrintASCIIString(4, 40, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_TEMP:
            draw_data_page_title("Temperature");
            snprintf(buf, sizeof(buf), "%d °C", watch_data.temp_celsius);
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_DISTANCE:
            draw_data_page_title("Distance");
            snprintf(buf, sizeof(buf), "%d km", (int)(watch_data.step_count * 0.6f / 1000.0f));
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_CALORIES:
            draw_data_page_title("Calories");
            snprintf(buf, sizeof(buf), "%d kcal", (int)(watch_data.step_count * 0.04f));
            OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_SET_TIME:
            draw_data_page_title("Set Time");
            OLED_PrintASCIIString(4, 20, "not implemented", &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_BLUETOOTH_PAIR:
            draw_data_page_title("Bluetooth");
            if (watch_data.ble_connected) {
                OLED_PrintASCIIString(4, 20, "Status: Connected", &afont8x6, OLED_COLOR_NORMAL);
            } else {
                OLED_PrintASCIIString(4, 20, "Status: Disconnected", &afont8x6, OLED_COLOR_NORMAL);
            }
            // 可选：显示更多信息，如模块名称
            OLED_PrintASCIIString(4, 30, "HC-05 (USART2)", &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_BRIGHTNESS:
            draw_data_page_title("Brightness");
            OLED_PrintASCIIString(4, 20, "not implemented", &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_FACTORY_RESET:
            draw_data_page_title("Factory Reset");
            OLED_PrintASCIIString(4, 20, "Confirm", &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_BATTERY:
            draw_data_page_title("Battery");
            {
                    // ADC 读取电池电压（假设分压比 2:1，参考电压 3.3V）
                    HAL_ADC_Start(&hadc1);
                    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
                        uint16_t adc_val = HAL_ADC_GetValue(&hadc1);
                        float battery_v = (float)adc_val / 4096.0f * 3.3f * 2.0f;
                        int battery_mv = (int)(battery_v * 1000);
                        snprintf(buf, sizeof(buf), "%d mV", battery_mv);
                    } else {
                        snprintf(buf, sizeof(buf), "ADC Err");
                    }
                    HAL_ADC_Stop(&hadc1);
                }
                OLED_PrintASCIIString(4, 20, buf, &afont8x6, OLED_COLOR_NORMAL);
                break;

        case DATA_PAGE_FIRMWARE:
            draw_data_page_title("Firmware");
            OLED_PrintASCIIString(4, 20, "SmartWatch v1.0", &afont8x6, OLED_COLOR_NORMAL);
            break;
        case DATA_PAGE_MAC:
            draw_data_page_title("Bluetooth MAC");
            OLED_PrintASCIIString(4, 20, "XX:XX:XX:XX:XX:XX", &afont8x6, OLED_COLOR_NORMAL);
            break;
        default:
            break;
    }
    OLED_ShowFrame();
}

