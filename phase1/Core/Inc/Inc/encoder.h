/*
 * encoder.h
 *
 *  Created on: Jul 5, 2026
 *      Author: Administrator
 */

#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"

/* 编码器事件定义 */
#define ENC_UP     1
#define ENC_DOWN   2
#define ENC_CLICK  3

/* 扫描编码器状态机（在 taskMenu 中每10ms调用） */
void Encoder_Scan(void);

#endif
