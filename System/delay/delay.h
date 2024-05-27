#ifndef DELAY_H_
#define DELAY_H_

#include "stm32f4xx.h"

void delay_init(u8 SYSCLK);
void delay_s(u16 ns);
void delay_ms(u16 nms);
void delay_us(u32 nus);
#endif
