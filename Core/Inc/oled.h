#ifndef __OLED_H
#define __OLED_H

#include "stm32f1xx_hal.h"
#include "i2c.h"

// OLED I2C 地址 (7-bit: 0x3C, 8-bit: 0x78)
#define OLED_ADDR 0x3C

// 函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Num, uint8_t IntLen, uint8_t DecLen);

#endif
