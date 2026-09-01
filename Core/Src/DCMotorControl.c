/*
 * DCMotorControl.c
 *
 *  Created on: Nov 14, 2025
 *      Author: Rama Syafrizal
 */

//USER INCLUDE
#include "DCMotorControl.h"
#include "stdlib.h"
#include <stdio.h>
#include "functionMath.h"

//USER DEFINE
#define COUNTS_PER_REVOLUTION 240 //R1
//#define COUNTS_PER_REVOLUTION 400 //L1,R2,L2
#define SAMPLING_TIME_MS 10

TIM_HandleTypeDef *p_htim;
PID_t pid_pos;

//tunning RL
//#define XI 18.0f
//#define ETA 35.0f
//#define LAMBDA 65.0f
//#define PHI 0.01f

#define XI 18.0f
#define ETA 35.0f
#define LAMBDA 65.0f
#define PHI 0.01f

//USER SETUP VARIABLE
int32_t pastCounterValue = 0;
int32_t currentCounterValue = 0;
int32_t deltaCounts;
uint32_t pastTime = 0;
uint32_t currentTime = 0;
uint32_t deltaTime = 10;
uint16_t ccrValue = 0;
uint16_t rpmTocCcr = 0;
float controlPID = 0.0f;
float rpmReff = 0.0f;
float rpmValue = 0.0f;
float rpmSensor = 0.0f;
float spdValue = 0.0f;
float pastRpm = 0.0f;
float rpm_dot = 0.0f;
float error;
float rpmValue_init = 0.0f;
float motorQtActualRpm = 0.0f;

//Low Past Filter
float alphaLPF = 0.05f;          // koefisien filter
//float encoder_raw = 0.0f;    // data encoder terbaru
float encoder_filt = 0.0f;   // output filter
float output_LPF = 0.0f;

//USER FUNCTION

void PID_Init(PID_t *pid,
              float kp, float ki, float kd,
              float N, float Ts,
              float out_min, float out_max)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->N  = N;
    pid->Ts = Ts;

    pid->integrator = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_derivative = 0.0f;

    pid->out_min = out_min;
    pid->out_max = out_max;
}


void motorRun_init(TIM_HandleTypeDef *htim)
{
	p_htim = htim;
	PID_Init(&pid_pos,
	         1.40567f,     // Kp
	         12.0696f,     // Ki
	         0.028171f,    // Kd
	         96.30915f,    // N filter
	         0.001f,    // Ts = 10 ms
	         -1000.0f,  // output min
	         1000.0f);  // output max
}

float LPF_Update(float encoder_raw)
{
    encoder_filt = alphaLPF * encoder_raw +
                   (1.0f - alphaLPF) * encoder_filt;
    return encoder_filt;
}

float PID_Compute(PID_t *pid, float setpoint, float feedback)
{
    error = setpoint - feedback;

    /* Proportional */
    float P = pid->Kp * error;

    /* Integral */
    pid->integrator += pid->Ki * pid->Ts * error;

    /* Derivative with N-filter */
    float alpha = (pid->N * pid->Kd) / (pid->Ts + pid->N * pid->Kd);

    float D = alpha * (pid->prev_derivative + error - pid->prev_error);

    pid->prev_derivative = D;
    pid->prev_error = error;

    /* Output */
    float output = P + pid->integrator + D;

    /* Saturation + anti-windup */
    if (output > pid->out_max)
        output = pid->out_max;
    else if (output < pid->out_min)
        output = pid->out_min;

    return output;
}

float saturation(float x) {
    if (x > 1.0f) return 1.0f;
    else if (x < -1.0f) return -1.0f;
    else return x;
}

float SMC(float reff, float feedback, float fdbck_dot, float k_XI, float k_ETA, float k_LAMBDA, float k_PHI) {
    float err, err_dot, s, u_sw, u_SMC;
    float rpmddot = 0.0f;

    err = reff - feedback;
    err_dot = 0.0f - fdbck_dot;
    s = k_XI * err + err_dot;
    u_sw = k_ETA * s + k_LAMBDA * saturation(s / k_PHI);
    u_SMC = (rpmddot + 23.59f * fdbck_dot + k_XI * err_dot + 207.4f * feedback + u_sw) / 242.2f;

    if (u_SMC > 1000.0f)
        u_SMC = 1000.0f;
    else if (u_SMC < 0.0f)
        u_SMC = 0.0f;

    return u_SMC;
}

//USER MAIN CODE

uint16_t motordir(float direction){
	if(direction > 0)
	{
		HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, SET);
		HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, RESET);
		return direction;
	}else if(direction < 0)
	{
		HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, RESET);
		HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, SET);
		return direction * (-1);
	}else{
		HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, RESET);
		HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, RESET);
		return 0;
	}
}

int16_t rpmSensorDir(float rpmDirection, float dir)
{
	if(dir > 0)
	{
		return rpmDirection;
	}else if(dir < 0)
	{
		return rpmDirection * (-1);
	}else{
		return 0;
	}
}

float speedSensor()
{
	return spdValue;
}

float motorQtGetActualRpm(void)
{
    return motorQtActualRpm;
}

void motorRunQt(float rpmReference)
{
    static int32_t previousCounter = 0;
    int32_t currentCounter = __HAL_TIM_GET_COUNTER(p_htim);
    int32_t deltaCounter = currentCounter - previousCounter;

    motorQtActualRpm = (float)(abs(deltaCounter) * 60 * 1000) /
                       (float)(COUNTS_PER_REVOLUTION * SAMPLING_TIME_MS);
    if (rpmReference < 0.0f)
        motorQtActualRpm = -motorQtActualRpm;
    else if (rpmReference == 0.0f)
        motorQtActualRpm = 0.0f;
    previousCounter = currentCounter;

    if (rpmReference > 120.0f)
        rpmReference = 120.0f;
    else if (rpmReference < -120.0f)
        rpmReference = -120.0f;

    if (rpmReference > 0.0f)
    {
        HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);
    }
    else if (rpmReference < 0.0f)
    {
        HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_SET);
        rpmReference = -rpmReference;
    }
    else
    {
        HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);
    }

    TIM1->CCR1 = (uint16_t)((rpmReference * 1000.0f) / 120.0f);
}

void motorRun(float reffValue, float initialCondition)
{
    currentCounterValue = __HAL_TIM_GET_COUNTER(p_htim);
    deltaCounts = currentCounterValue - pastCounterValue;
    rpmValue = (float)(abs(deltaCounts) * 60 * 1000) / (float)(COUNTS_PER_REVOLUTION * SAMPLING_TIME_MS);
    output_LPF = LPF_Update(rpmValue);

    rpmReff = map(reffValue, -120, 120, -1000, 1000);
    spdValue = map(rpmSensorDir(output_LPF,reffValue),0,1000,0,120);
    rpmReff = motordir(rpmReff);

    //Control
    controlPID = PID_Compute(&pid_pos, rpmReff, rpmValue);

    rpmTocCcr = controlPID; // if not using initialize
//    rpmTocCcr = rpmReff;
    ccrValue = rpmTocCcr;
    TIM1->CCR1 = ccrValue;

    pastCounterValue = currentCounterValue;
}
