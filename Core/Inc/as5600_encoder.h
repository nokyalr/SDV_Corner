/*
 * as5600_encoder.h
 *
 *  Created on: Jan 21, 2026
 *      Author: Rama Syafrizal
 */

#ifndef INC_AS5600_ENCODER_H_
#define INC_AS5600_ENCODER_H_

#include "stm32f7xx_hal.h"

/* ================= CONFIG ================= */
#define AS5600_I2C_ADDR   (0x36 << 1)

/* ================= API ================= */
HAL_StatusTypeDef AS5600_Encoder_Init(I2C_HandleTypeDef *hi2c);

float AS5600_ReadAngleDeg(void);        // 0–360°
float AS5600_GetLinearAngleDeg(void);   // multi-turn (… -360, 0, 360, 720 …)

void AS5600_Encoder_Reset(void);        // reset revolution counter


#endif /* INC_AS5600_ENCODER_H_ */
