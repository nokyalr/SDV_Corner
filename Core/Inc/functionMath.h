/*
 * functionMath.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Rama Syafrizal
 */

#ifndef INC_FUNCTIONMATH_H_
#define INC_FUNCTIONMATH_H_

#define PI_F        3.14159265358979323846f
#define RAD2DEG_F   (180.0f / PI_F)
#define DEG2RAD_F   (PI_F / 180.0f)

static inline float map(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static inline float saturate(float x, float min, float max)
{
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

static inline float rad2deg(float rad)
{
    return rad * RAD2DEG_F;
}

static inline float deg2rad(float deg)
{
    return deg * DEG2RAD_F;
}

static inline float max_f(float a, float b)
{
    return (a > b) ? a : b;
}


#endif /* INC_FUNCTIONMATH_H_ */
