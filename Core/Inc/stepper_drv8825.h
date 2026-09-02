/*
 * stepper_drv8825.h
 *
 *  Created on: Dec 21, 2025
 *      Author: Rama Syafrizal
 *
 *
 *  .stepper Setup
 *  pada file stm32f7xx_it.c tambahakan program berikut,
 *
 *  -> USER CODE BEGIN Includes =
 *  #include "stepper_drv8825.h"
 *
 *  -> USER CODE BEGIN EV
 *	extern stepper_t stepper1;
 *
 *  -> USER BEGIN 1 =
 *
	void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
	{
		if (htim->Instance == TIM3)
		{
			Stepper_TimerCallback(&stepper1);
		}
	}
 */

#ifndef INC_STEPPER_DRV8825_H_
#define INC_STEPPER_DRV8825_H_

#include "stm32f7xx_hal.h"
#include <stdint.h>

/* ================= USER TUNING ================= */
#define SPEED_FAST   100     // fastest
#define SPEED_SLOW   1000    // slowest
#define STEPPER_DEADBAND           0
#define ERROR_MAX				   20
/* =============================================== */

typedef enum
{
    STEPPER_IDLE = 0,
    STEPPER_MOVING
} stepper_state_t;

typedef struct
{
    volatile int32_t current_pos;
    volatile int32_t setpoint;
    volatile int32_t error;

    uint32_t step_period;
    uint32_t max_speed_period;

    uint8_t direction;
    stepper_state_t state;

    TIM_HandleTypeDef *htim;
    uint32_t tim_channel;

    GPIO_TypeDef *dir_port;
    uint16_t dir_pin;

    GPIO_TypeDef *en_port;
    uint16_t en_pin;

} stepper_t;

extern int32_t current_p;

/* ============ PUBLIC API ============ */

void Stepper_Init(stepper_t *motor);
void Stepper_SetSetpoint(stepper_t *motor, float setpoint, float Mz);
void Stepper_ControlUpdate(stepper_t *motor);
void Stepper_TimerCallback(stepper_t *motor);
int32_t positionSensor();

/* ============ Qt CONTROL API ============ */
float stepperQtGetActualAngle(void);
void stepperRunQt(float angleReference);

#endif /* INC_STEPPER_DRV8825_H_ */
