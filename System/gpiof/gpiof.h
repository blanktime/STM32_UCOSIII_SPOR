#ifndef __GPIOF_H
#define __GPIOF_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

#define VDT_PORT          GPIOA
#define VDT_GPIO_CLOCK    RCC_AHB1Periph_GPIOB
#define VDT_ON      GPIO_Pin_1
#define VDT_OFF     GPIO_Pin_2


void VDTPIN_Init(void);

void ON_VDT(void);

void OFF_VDT(void);

void Led_Init(void);

void Key_Init(void);

void Out_Pin(void);

void Power_Pin(void);
#endif 
