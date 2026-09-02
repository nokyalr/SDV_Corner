/*
 * DCMotorControl.h
 *
 *  Created on: Nov 14, 2025
 *      Author: Rama Syafrizal
 *
 *  .ioc Setup
 *  TIM1 -> Channel 1 : PWM Generation CH1
 *  	 -> if clock 100 : MHz, Prescaler 99, ARR 999
 * 	TIM2 -> Combined Channels : ENCODER MODE
 * 		 -> Encoder -> Encoder Mode : Encoder Mode TI1 and TI2
 *
 */

#ifndef INC_DCMOTORCONTROL_H_
#define INC_DCMOTORCONTROL_H_

//USER INCLUDE
#include "main.h"
//USER DEFINE

//USER SETUP VARIABLE
extern float spdValue;

typedef struct
{
    float Kp;
    float Ki;
    float Kd;
    float N;        // derivative filter coefficient
    float Ts;       // sampling time

    float integrator;
    float prev_error;
    float prev_derivative;

    float out_min;
    float out_max;
} PID_t;

//USER FUNCTION
void motorRun(float reffValue, float initialCondition);
void motorRunQt(float rpmReference);
float motorQtGetActualRpm(void);
void motorRun_init(TIM_HandleTypeDef *htim);
float speedSensor();

//USER MAIN CODE

#endif /* INC_DCMOTORCONTROL_H_ */
