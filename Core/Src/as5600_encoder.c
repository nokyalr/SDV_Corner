/*
 * as5600_encoder.c
 *
 *  Created on: Jan 21, 2026
 *      Author: Rama Syafrizal
 */


#include "as5600_encoder.h"

/* ================= PRIVATE VARIABLES ================= */
static I2C_HandleTypeDef *as5600_i2c = NULL;

static float angle_prev = 0.0f;
static int32_t revolution_count = 0;

/* ================= PRIVATE FUNCTION ================= */
static uint16_t AS5600_ReadRaw(void)
{
    uint8_t rawData[2];

    if (as5600_i2c == NULL)
        return 0;

    HAL_I2C_Mem_Read(as5600_i2c,
                     AS5600_I2C_ADDR,
                     0x0E,                       // ANGLE register
                     I2C_MEMADD_SIZE_8BIT,
                     rawData,
                     2,
                     HAL_MAX_DELAY);

    return ((rawData[0] & 0x0F) << 8) | rawData[1];
}

/* ================= PUBLIC FUNCTIONS ================= */
HAL_StatusTypeDef AS5600_Encoder_Init(I2C_HandleTypeDef *hi2c)
{
    as5600_i2c = hi2c;

    /* Initialize previous angle */
    angle_prev = AS5600_ReadAngleDeg();
    revolution_count = 0;

    return HAL_OK;
}

/* --------------------------------------------------- */
float AS5600_ReadAngleDeg(void)
{
    uint16_t raw = AS5600_ReadRaw();
    return (raw * 360.0f) / 4096.0f;
}

/* --------------------------------------------------- */
float AS5600_GetLinearAngleDeg(void)
{
    float angle_now = AS5600_ReadAngleDeg();
    float delta = angle_now - angle_prev;

    /* Wrap-around detection */
    if (delta > 180.0f)
    {
        revolution_count--;
    }
    else if (delta < -180.0f)
    {
        revolution_count++;
    }

    angle_prev = angle_now;

    return (revolution_count * 360.0f) + angle_now;
}

/* --------------------------------------------------- */
void AS5600_Encoder_Reset(void)
{
    revolution_count = 0;
    angle_prev = AS5600_ReadAngleDeg();
}
