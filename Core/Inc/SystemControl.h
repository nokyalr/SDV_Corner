/*
 * SystemControl.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Rama Syafrizal
 */

#ifndef INC_SYSTEMCONTROL_H_
#define INC_SYSTEMCONTROL_H_

float universal_PID(float setpoint, float feedback, float Kp, float Ki, float Kd, float N, float out_min, float out_max);
float vehicleSpdControl(float setpoint, float feedback, float init_Con);
float TorqueToSpd(float spd, float torque);
float TractionControlSystem(float tcsReff, float vx, float vx_wheel);

#endif /* INC_SYSTEMCONTROL_H_ */
