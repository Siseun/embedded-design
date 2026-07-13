/*
 * encoder.c
 *
 *  Created on: Jul 5, 2026
 *      Author: Administrator
 */

#include "FreeRTOS.h"
#include "queue.h"



#include "cmsis_os.h"
#include "main.h"
#include "encoder.h"


extern QueueHandle_t xMenuEventQueue;  // 在 freertos.c 中定义

/* 编码器状态机状态 */
typedef enum {
    ENC_IDLE,
    ENC_A_HIGH,
    ENC_B_HIGH,
    ENC_BOTH_HIGH
} EncState;

static EncState enc_state = ENC_IDLE;

void Encoder_Scan(void) {
    uint8_t a = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
    uint8_t b = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);

    switch (enc_state) {
        case ENC_IDLE:
            if (a && !b) enc_state = ENC_A_HIGH;
            else if (!a && b) enc_state = ENC_B_HIGH;
            break;
        case ENC_A_HIGH:
            if (!a && !b) { // 反转 → 发送 ENC_DOWN
                uint8_t evt = ENC_DOWN;
                xQueueSend(xMenuEventQueue, &evt, 0);
                enc_state = ENC_IDLE;
            } else if (a && b) {
                enc_state = ENC_BOTH_HIGH;
            }
            break;
        case ENC_B_HIGH:
            if (!a && !b) { // 正转 → 发送 ENC_UP
                uint8_t evt = ENC_UP;
                xQueueSend(xMenuEventQueue, &evt, 0);
                enc_state = ENC_IDLE;
            } else if (a && b) {
                enc_state = ENC_BOTH_HIGH;
            }
            break;
        case ENC_BOTH_HIGH:
            if (!a && !b) enc_state = ENC_IDLE;
            else if (a && !b) enc_state = ENC_A_HIGH;
            else if (!a && b) enc_state = ENC_B_HIGH;
            break;
    }
}
