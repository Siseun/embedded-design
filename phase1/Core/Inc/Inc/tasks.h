/*
 * tasks.h
 *
 *  Created on: Jul 5, 2026
 *      Author: Administrator
 */

#ifndef __TASKS_H
#define __TASKS_H

void taskTimeKeep(void *pvParameters);
void taskSensor(void *pvParameters);
void taskDisplay(void *pvParameters);
void taskMenu(void *pvParameters);
void taskBluetooth(void *pvParameters);

#endif
