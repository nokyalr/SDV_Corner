/*
 * buttonMode.c
 *
 *  Created on: Dec 10, 2025
 *      Author: Rama Syafrizal
 */
#include "buttonMode.h"
#include "udpclient.h"
#include <stdio.h>

uint8_t buttonState = 0;
uint8_t lastButtonState = 1;
uint32_t lastDebounceTime = 0;
uint32_t debounceDelay = 250; // ms

void buttonRun(){
	  buttonState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13); // Tombol internal Nucleo aktif LOW

	  if (buttonState == GPIO_PIN_RESET && lastButtonState == GPIO_PIN_SET)
	  {
		  uint32_t now = HAL_GetTick();
		  if ((now - lastDebounceTime) > debounceDelay)
		  {
			  HIL_ETH_ToggleMode();  // Ganti mode pengiriman
			  lastDebounceTime = now;
		  }
	  }
	  lastButtonState = buttonState;
}

// === 5. LED Indikator Mode ===
// MATLAB → LED MERAH (PB14) ON
// VCU → LED BIRU (PB7) ON

void buttonModeLED(){
	if (currEthMode == SEND_TO_MATLAB)
	{
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);   // Merah ON
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);  // Biru OFF
	}
	else
	{
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // Merah OFF
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);    // Biru ON
	}
}


