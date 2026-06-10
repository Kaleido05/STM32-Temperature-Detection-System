#include "ds18b20.h"
#include "gpio.h"
#include "delay.h" // 我们需要自己实现微秒级延时函数

// 复位DS18B20
static uint8_t DS18B20_Reset(void)
{
    uint8_t ack;
    
    // 拉低DQ
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
    delay_us(480); // 保持低电平至少480us
    
    // 释放DQ
    HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
    delay_us(60); // 等待DS18B20响应
    
    // 读取应答信号
    ack = HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN);
    delay_us(420); // 等待复位周期结束
    
    return ack; // 0表示应答成功，1表示失败
}

// 初始化DS18B20
uint8_t DS18B20_Init(void)
{
    return DS18B20_Reset();
}

// 向DS18B20写入一个字节
void DS18B20_Write_Byte(uint8_t dat)
{
    uint8_t i;
    
    for(i=0; i<8; i++)
    {
        // 拉低DQ开始写时序
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
        delay_us(1);
        
        // 写入当前位
        if(dat & 0x01)
            HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
        
        delay_us(60); // 保持至少60us
        
        // 释放DQ
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
        delay_us(1);
        
        dat >>= 1;
    }
}

// 从DS18B20读取一个字节
uint8_t DS18B20_Read_Byte(void)
{
    uint8_t i, dat=0;
    
    for(i=0; i<8; i++)
    {
        dat >>= 1;
        
        // 拉低DQ开始读时序
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_RESET);
        delay_us(1);
        
        // 释放DQ
        HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, GPIO_PIN_SET);
        delay_us(1);
        
        // 读取数据位
        if(HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN))
            dat |= 0x80;
        
        delay_us(60); // 等待读周期结束
    }
    
    return dat;
}

// 读取温度值
float DS18B20_Read_Temperature(void)
{
    uint8_t temp_l, temp_h;
    int16_t temp;
    float temperature;
    
    // 复位并发送跳过ROM命令
    DS18B20_Reset();
    DS18B20_Write_Byte(0xCC); // 跳过ROM
    DS18B20_Write_Byte(0x44); // 启动温度转换
    
    // 等待转换完成（最大750ms）
    HAL_Delay(800);
    
    // 复位并读取温度寄存器
    DS18B20_Reset();
    DS18B20_Write_Byte(0xCC); // 跳过ROM
    DS18B20_Write_Byte(0xBE); // 读取温度寄存器
    
    temp_l = DS18B20_Read_Byte(); // 低字节
    temp_h = DS18B20_Read_Byte(); // 高字节
    
    // 组合温度数据
    temp = (int16_t)((temp_h << 8) | temp_l);
    
    // 转换为实际温度值（DS18B20分辨率为12位时，0.0625℃/LSB）
    temperature = temp * 0.0625f;
    
    return temperature;
}