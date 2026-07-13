/*
 * tasks.c
 *
 *  Created on: Jul 5, 2026
 *      Author: Administrator
 */
#include <math.h>
#include "FreeRTOS.h"
#include "tasks.h"
#include "queue.h"
#include "smartwatch_ui.h"
#include "mpu6050.h"
#include "encoder.h"
#include "main.h"


#include "cmsis_os.h"
#include "usart.h"          // 添加这一行
#include <string.h>

extern SmartWatchData_t watch_data;
extern uint8_t current_page;
extern QueueHandle_t xMenuEventQueue;

/* ==================== taskTimeKeep ==================== */
void taskTimeKeep(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t sec_counter = 0;
    for (;;) {
        sec_counter++;
        if (sec_counter >= 60) {
            sec_counter = 0;
            watch_data.minute++;
            if (watch_data.minute >= 60) {
                watch_data.minute = 0;
                watch_data.hour++;
                if (watch_data.hour >= 24) {
                    watch_data.hour = 0;
                    // 简单日期递增（每月按30天近似）
                    watch_data.day++;
                    if (watch_data.day > 30) {
                        watch_data.day = 1;
                        watch_data.month++;
                        if (watch_data.month > 12) {
                            watch_data.month = 1;
                            watch_data.year++;
                        }
                    }
                }
            }
        }
        watch_data.second = sec_counter;

        // 读取蓝牙连接状态（假设接在 PB0）
        watch_data.ble_connected = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void taskSensor(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(20);

    float prev_filtered = 0.0f;
    float max_val = -100.0f;
    float min_val = 100.0f;
    int sample_count = 0;
    float threshold = 9.8f;
    uint32_t last_step_time = 0;
    uint8_t state = 0;
    const float HYSTERESIS = 0.3f;

    static float last_ax = 0.0f;
    static float last_ay = 0.0f;
    static float last_az = 9.81f;

    vTaskDelay(pdMS_TO_TICKS(500));

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        if (!watch_data.imu_status) {
            continue;
        }

        // ---- 读取加速度（失败则用备份） ----
        if (MPU6050_ReadAccel(&watch_data.accel) != 0) {
            watch_data.accel.ax = last_ax;
            watch_data.accel.ay = last_ay;
            watch_data.accel.az = last_az;
        } else {
            last_ax = watch_data.accel.ax;
            last_ay = watch_data.accel.ay;
            last_az = watch_data.accel.az;
        }

        // ========== 新增：计算姿态角 ==========
        MPU6050_CalcAngle(&watch_data.accel, &watch_data.angle);

        // ========== 新增：读取温度（保留上次有效值） ==========
        float temp = MPU6050_ReadTemp();
        if (temp > -100.0f) {   // 排除错误值 -273.0
            watch_data.temp_celsius = (int8_t)temp;
        }

        // ---- 以下为步数检测（不变） ----
        float ax = watch_data.accel.ax;
        float ay = watch_data.accel.ay;
        float az = watch_data.accel.az;
        float accel_mag = sqrtf(ax*ax + ay*ay + az*az);

        float filtered = 0.1f * accel_mag + 0.9f * prev_filtered;
        prev_filtered = filtered;

        if (filtered > max_val) max_val = filtered;
        if (filtered < min_val) min_val = filtered;
        sample_count++;
        if (sample_count >= 50) {
            threshold = (max_val + min_val) / 2.0f;
            max_val = filtered;
            min_val = filtered;
            sample_count = 0;
        }

        float high_th = threshold + HYSTERESIS;
        float low_th  = threshold - HYSTERESIS;
        TickType_t now = xTaskGetTickCount();

        if (filtered > high_th) {
            if (state == 0) {
                if ((now - last_step_time) > pdMS_TO_TICKS(200)) {
                    watch_data.step_count++;
                    last_step_time = now;
                }
                state = 1;
            }
        } else if (filtered < low_th) {
            state = 0;
        }
    }
}

/* ==================== taskMenu ==================== */
void taskMenu(void *pvParameters) {
    uint8_t last_key = 1;      // PB13 默认高电平（上拉）
    uint8_t debounce_cnt = 0;

    for (;;) {
        // 扫描编码器
        Encoder_Scan();

        // 按键检测（PB13）
        uint8_t key = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
        if (key != last_key) {
            debounce_cnt++;
            if (debounce_cnt >= 3) {  // 消抖
                if (key == 0) {       // 按下（低电平）
                    uint8_t evt = ENC_CLICK;
                    xQueueSend(xMenuEventQueue, &evt, 0);
                }
                debounce_cnt = 0;
                last_key = key;
            }
        } else {
            debounce_cnt = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms 轮询
    }
}

/* ==================== taskDisplay ==================== */
void taskDisplay(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t evt;

    for (;;) {
        // 处理队列中的事件（非阻塞）
        while (xQueueReceive(xMenuEventQueue, &evt, 0) == pdTRUE) {
            // 根据当前应用状态处理事件
            switch (app_state) {
                case APP_STATE_WATCHFACE:
                    if (evt == ENC_CLICK) {
                        // 进入主菜单
                        app_state = APP_STATE_MENU;
                        menu_ctx.current_menu_index = 0; // 根节点
                        menu_ctx.selected = 0;
                        menu_ctx.history_depth = 0;
                    }
                    break;

                case APP_STATE_MENU: {
                    const MenuItem_t *cur = &menu_items[menu_ctx.current_menu_index];
                    uint8_t child_count = cur->child_count;
                    uint8_t has_return = (menu_ctx.history_depth > 0);   // 您已修改
                    uint8_t total = child_count + (has_return ? 1 : 0);

                    if (evt == ENC_UP) {
                        if (menu_ctx.selected > 0) menu_ctx.selected--;
                        else menu_ctx.selected = total - 1;
                    } else if (evt == ENC_DOWN) {
                        menu_ctx.selected++;
                        if (menu_ctx.selected >= total) menu_ctx.selected = 0;
                    } else if (evt == ENC_CLICK) {
                        // 判断是否选中了“返回”项
                        if (has_return && menu_ctx.selected == child_count) {
                            // 返回上一级菜单
                            if (menu_ctx.history_depth > 0) {
                                menu_ctx.current_menu_index = menu_ctx.history[--menu_ctx.history_depth];
                                menu_ctx.selected = 0;
                            }
                            // 如果当前是根菜单且没有返回项，但根菜单有“返回表盘”选项，不会进入此分支
                        } else {
                            // 选中了某个子项
                            uint8_t item_index = cur->child_start + menu_ctx.selected;
                            const MenuItem_t *item = &menu_items[item_index];
                            if (item->child_count > 0) {
                                // 进入子菜单
                                if (menu_ctx.history_depth < 5) {
                                    menu_ctx.history[menu_ctx.history_depth++] = menu_ctx.current_menu_index;
                                }
                                menu_ctx.current_menu_index = item_index;
                                menu_ctx.selected = 0;
                            } else {
                                // 叶子节点：执行 action
                                if (item->action) {
                                    item->action(); // 可能会改变 app_state
                                }
                            }
                        }
                    }
                    break;
                }

                case APP_STATE_DATA:
                    if (evt == ENC_CLICK) {
                        // 返回菜单（保留当前位置）
                        app_state = APP_STATE_MENU;
                    }
                    // UP/DOWN 在数据页无作用，忽略
                    break;

                default:
                    break;
            }
        }

        // 根据当前状态绘制相应界面
        switch (app_state) {
            case APP_STATE_WATCHFACE:
                UI_DrawWatchFace(&watch_data);
                OLED_ShowFrame();   // ← 添加这一行
                break;
            case APP_STATE_MENU:
                UI_DrawMenu();
                break;
            case APP_STATE_DATA:
                UI_DrawDataPage();
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
    }
}

/* ==================== taskBluetooth ==================== */
void taskBluetooth(void *pvParameters) {
    uint8_t tx_buffer[5];
    uint32_t frame_counter = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        tx_buffer[0] = 0xAA;
        tx_buffer[1] = 0x01;
        tx_buffer[2] = 0x18;
        tx_buffer[3] = (uint8_t)(frame_counter & 0xFF);
        tx_buffer[4] = 0x55;
        HAL_UART_Transmit(&huart2, tx_buffer, 5, 100);
        frame_counter++;

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}
