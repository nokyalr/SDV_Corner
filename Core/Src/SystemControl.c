/*
 * SystemControl.c
 *
 *  Created on: Feb 5, 2026
 *      Author: Rama Syafrizal
 */

#include "SystemControl.h"
#include "functionMath.h"
#include "math.h"

float errorPID, prev_errorPID;
float proposional, integral, derivative, prev_derivativePID;
float alpha_PID;
float Ts_PID = 0.001;

float integral_VSC = 0;

float TCS_e_prev = 0;
float TCS_slip, TCS_e_dot, TCS_s;
float TCS_error;
float TCS_u;

float error_spdcontrol;
float output_spdcontrol;

float universal_PID(float setpoint, float feedback, float Kp, float Ki, float Kd, float N, float out_min, float out_max);
float TCS_PID(float setpoint, float feedback);
float vehicleSpdControl_PID(float setpoint, float feedback);

float vehicleSpdControl(float setpoint, float feedback, float init_Con)
{
	setpoint = vehicleSpdControl_PID(setpoint, feedback);
	init_Con += setpoint;

	return init_Con;
}

float TorqueToSpd(float spd, float torque)
{
	float spdOutput;
	torque = 1 + ((torque / 256) * 0.08);
	spdOutput = spd * torque;

	return spdOutput;
}

float TractionControlSystem(float tcsReff, float vx, float vx_wheel)
{
	TCS_slip = (vx_wheel - vx) / max_f(vx, 0.5);
	TCS_u = TCS_PID(0, TCS_slip);

	return TCS_u + tcsReff;
}

float universal_PID(float setpoint, float feedback, float Kp, float Ki, float Kd, float N, float out_min, float out_max)
{
    errorPID = setpoint - feedback;

    /* Proportional */
    proposional = Kp * errorPID;

    /* Integral */
    integral += Ki * Ts_PID * errorPID;

    /* Derivative with N-filter */
    alpha_PID = (N * Kd) / (Ts_PID + N * Kd);

    derivative = alpha_PID * (prev_derivativePID + errorPID - prev_errorPID);

    prev_derivativePID = derivative;
    prev_errorPID = errorPID;

    /* Output */
    float output = proposional + integral + derivative;

    /* Saturation + anti-windup */
    output = saturate(output, out_min, out_max);

    return output;
}

float vehicleSpdControl_PID(float setpoint, float feedback)
{
    float error = setpoint - feedback;
    error_spdcontrol = error;

    error = saturate(error,-5, 1000);
    /* Proportional */
    float proposional = 12 * error;
//    float proposional = error;

    /* Integral */
    integral_VSC += 0.4 * 0.001 * error;

    /* Output */
    float output = proposional + integral_VSC;
//    float output = proposional;

    /* Saturation + anti-windup */
    output = saturate(output, -100, 100);
    output_spdcontrol = output;

    return output;
}

float TCS_PID(float setpoint, float feedback)
{
	feedback = saturate(feedback, 0, 5);
	float error = setpoint - feedback;
	float P = 26 * error;
	float I = 2.5 * 0.001 * error;
	float output = P * I;
	return output;
}
