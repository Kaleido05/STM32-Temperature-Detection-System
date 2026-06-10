#include "delay.h"

// 微秒级延时函数（基于SysTick，72MHz系统时钟）
void delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt=0;
    uint32_t reload = SysTick->LOAD;
    
    ticks = us * 72; // 72MHz时钟，1us=72个时钟周期
    told = SysTick->VAL;
    
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
                tcnt += told - tnow;
            else
                tcnt += reload - tnow + told;
            
            told = tnow;
            if(tcnt >= ticks)
                break;
        }
    }
}