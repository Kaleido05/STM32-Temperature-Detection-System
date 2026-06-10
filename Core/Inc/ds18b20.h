#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f1xx_hal.h"

// 引脚定义
#define DS18B20_DQ_PIN GPIO_PIN_1
#define DS18B20_DQ_PORT GPIOA

// 函数声明
uint8_t DS18B20_Init(void);
void DS18B20_Write_Byte(uint8_t dat);
uint8_t DS18B20_Read_Byte(void);
float DS18B20_Read_Temperature(void);

#endif