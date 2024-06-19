/*
 * The Clear BSD License
 * Copyright (c) 2018 Adesto Technologies Corporation, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * @ingroup USER_CONFIG Configuration Layer
 */
/**
 * @file    user_config.c
 * @brief   Project definitions exist here.
 */

#include "user_config.h"

void USER_CONFIG_PinInit(uint32_t port, uint32_t pin, enum directionIO direction)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = pin;
	if(direction == OUTPUT)
	{
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	}
	else if(direction == INPUT)
	{
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}
	HAL_GPIO_Init((GPIO_TypeDef*)port, &GPIO_InitStruct);
}

void USER_CONFIG_PinClear(uint32_t port, uint32_t pin)
{
  HAL_GPIO_WritePin((GPIO_TypeDef*)port, pin, GPIO_PIN_RESET);
}

void USER_CONFIG_PinSet(uint32_t port, uint32_t pin)
{
  HAL_GPIO_WritePin((GPIO_TypeDef*)port, pin, GPIO_PIN_SET);
}

uint8_t USER_CONFIG_PinRead(uint32_t port, uint32_t pin)
{
	return (uint8_t) HAL_GPIO_ReadPin((GPIO_TypeDef*)port, pin);
}

void BOARD_InitBootPins(void)
{

}

void BOARD_InitBootClocks(void)
{

}

void BOARD_InitBootPeripherals(void)
{

}

void BOARD_InitDebugConsole(void)
{

}

void USER_CONFIG_BoardInit()
{
  	/* Init board hardware. */
	// Set pinmuxes and clock
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();
}
