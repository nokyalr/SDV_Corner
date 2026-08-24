/*
 * stepper_drv8825.c
 *
 *  Created on: Dec 21, 2025
 *      Author: Rama Syafrizal
 */


#include "stepper_drv8825.h"
#include "as5600_encoder.h"
#include "functionMath.h"
#include <stdlib.h>

volatile uint32_t dbg_step_counter = 0;
int32_t error_n;
int32_t errorPosition = 0;
int32_t current_p = 0;
int32_t target;
uint32_t absError;
uint32_t speed;
float read_steer_cmd = 0.0f;
float current_sensor;

/* ================= INTERNAL ================= */

static void Stepper_SetDirection(stepper_t *motor, uint8_t dir)
{
    motor->direction = dir;
    HAL_GPIO_WritePin(
        motor->dir_port,
        motor->dir_pin,
        dir ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
}

/* ============================================ */

float Steering_cmd(float steer_cmd)
{
	read_steer_cmd  = steer_cmd / (51430 * 2297.5);
	read_steer_cmd  = rad2deg(read_steer_cmd);
	read_steer_cmd  = saturate(read_steer_cmd , -5, 5);
	return read_steer_cmd;
}

void Stepper_Init(stepper_t *motor)
{
    motor->current_pos = 0;
    motor->setpoint    = 0;
    motor->error       = 0;
    motor->state       = STEPPER_IDLE;
    motor->max_speed_period = SPEED_FAST;

    /* Pastikan STEP berhenti saat init */
    HAL_TIM_PWM_Stop_IT(motor->htim, motor->tim_channel);
}

/* ENABLE FUNCTION DIHAPUS TOTAL */

/* Set target posisi */
void Stepper_SetSetpoint(stepper_t *motor, float setpoint, float Mz)
{
	Mz = Steering_cmd(Mz);
	setpoint = setpoint - Mz;
	setpoint = setpoint * 100;
    motor->setpoint = map(setpoint,-4000,4000,-120,120);
//    dbg_step_counter++;
}

int32_t positionSensor()
{
	return current_p;
}

/* ===== MAIN CONTROLLER (CALL FROM RTOS TASK) ===== */
void Stepper_ControlUpdate(stepper_t *motor)
{
//    errorPosition = motor->setpoint - motor->current_pos;
	errorPosition = motor->setpoint - AS5600_GetLinearAngleDeg();
    error_n = errorPosition;
    target = motor->setpoint;
    current_sensor = AS5600_GetLinearAngleDeg();
    current_p = current_sensor;


    /* ===== STOP CONDITION ===== */
    if (abs(errorPosition) == STEPPER_DEADBAND)
    {
        if (motor->state == STEPPER_MOVING)
        {
            HAL_TIM_PWM_Stop_IT(motor->htim, motor->tim_channel);
            motor->state = STEPPER_IDLE;
        }
        return;
    }

    uint8_t new_dir = (errorPosition > 0) ? 1 : 0;

    /* ===== SAFE DIRECTION CHANGE ===== */
    if (motor->state == STEPPER_MOVING &&
        new_dir != motor->direction)
    {
        HAL_TIM_PWM_Stop_IT(motor->htim, motor->tim_channel);
        motor->state = STEPPER_IDLE;
    }

    Stepper_SetDirection(motor, new_dir);

    /* ===== SPEED CONTROL (SIMPLE P) ===== */

    /* Absolute error */
    absError = abs(errorPosition);

    /* Clamp error */
    if (absError > ERROR_MAX)
    {
        absError = ERROR_MAX;
    }

    /*
     * Mapping terbalik:
     * error besar  -> speed kecil (200)
     * error kecil  -> speed besar (3000)
     */
    speed = SPEED_SLOW - ((SPEED_SLOW - SPEED_FAST) * absError) / ERROR_MAX;

    /* Safety clamp */
    if (speed < SPEED_FAST)
        speed = SPEED_FAST;

    if (speed > SPEED_SLOW)
        speed = SPEED_SLOW;

    /* Apply */
    motor->step_period = speed;

    __HAL_TIM_SET_AUTORELOAD(motor->htim, motor->step_period);
    __HAL_TIM_SET_COMPARE(
        motor->htim,
        motor->tim_channel,
        motor->step_period / 2
    );

    /* ===== ALWAYS ENSURE MOTOR IS RUNNING ===== */
    if (motor->state != STEPPER_MOVING)
    {
        HAL_TIM_PWM_Start_IT(motor->htim, motor->tim_channel);
        motor->state = STEPPER_MOVING;
    }
}


/* ===== TIMER ISR STEP COUNTER ===== */
void Stepper_TimerCallback(stepper_t *motor)
{
    if (motor->state != STEPPER_MOVING)
        return;

    motor->current_pos += motor->direction ? 1 : -1;
}
