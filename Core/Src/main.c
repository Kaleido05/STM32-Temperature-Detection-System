/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 温度监测系统 — DS18B20 + OLED + 蜂鸣器
  * @description    : 每秒读取 DS18B20 温度，OLED 实时显示，超过 27℃ 蜂鸣器告警
  * @hardware       : STM32F103C8T6, 0.96" I2C OLED, DS18B20, 有源蜂鸣器(低电平触发)
  * @pinout         : PA1=DS18B20_DQ, PB8=I2C1_SCL, PB9=I2C1_SDA, PA0=TIM2_CH1(Buzzer PWM)
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ds18b20.h"   // DS18B20 温度传感器驱动
#include "oled.h"      // OLED 128x64 显示屏驱动 (I2C)
#include "delay.h"     // 微秒级延时 (OneWire 时序用)
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
float temperature;     // 当前温度值 (℃)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void Buzzer_PWM_Init(void);

/**
  * @brief  主函数 — 系统初始化 → 外设自检 → 循环测温显示
  * @retval int
  */
int main(void)
{
  /* MCU 基础初始化 ----------------------------------------------------------*/
  HAL_Init();                       // HAL 库初始化，配置 SysTick
  SystemClock_Config();             // 系统时钟: HSE→PLL→72MHz

  /* 外设初始化 --------------------------------------------------------------*/
  MX_GPIO_Init();                   // GPIO 初始化 (DS18B20_DQ/PA1, Buzzer/PA0→AF)
  MX_I2C1_Init();                   // I2C1 初始化 (OLED: PB8=SCL, PB9=SDA)
  Buzzer_PWM_Init();                // TIM2_CH1 PWM 初始化 (PA0, 2.5kHz, 默认关闭)

  /* USER CODE BEGIN 2 */
  /* OLED 与传感器自检 ------------------------------------------------------*/
  OLED_Init();                      // OLED 初始化 (SSD1306, I2C地址 0x3C)

  // 第1行: 固定标题
  OLED_ShowString(1, 1, "Temperature:");

  // DS18B20 自检 — 复位应答失败则报错停机
  if(DS18B20_Init() == 0)
  {
    OLED_ShowString(2, 1, "DS18B20 OK!");   // 传感器在线
  }
  else
  {
    OLED_ShowString(2, 1, "DS18B20 Err!");   // 传感器异常
    while(1);                                 // 停机, 需复位重试
  }

  HAL_Delay(1000);                  // 停留 1 秒让用户看到自检结果
  OLED_Clear();                     // 清屏，进入主显示界面

  // 恢复标题行
  OLED_ShowString(1, 1, "Temperature:");
  /* USER CODE END 2 */

  /* 主循环 — 每秒刷新一次 --------------------------------------------------*/
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // [1] 读取 DS18B20 温度 (12-bit, 0.0625℃/LSB, 耗时约 800ms)
    temperature = DS18B20_Read_Temperature();

    // [2] OLED 第2行显示当前温度 (格式: "Temp:XX.XXC")
    OLED_ShowString(2, 1, "Temp:");
    OLED_ShowFloat(2, 7, temperature, 2, 2);   // 2位整数 + 2位小数
    OLED_ShowString(2, 13, "C");

    // [3] 蜂鸣器告警 + LED 指示灯 — 温度 > 27℃ 时同时触发
    //     PWM模式1+低电平有效: CCR1=200→50%方波→鸣响, CCR1=0→持续高电平→关闭
    if(temperature > 27.0f)
    {
      TIM2->CCR1 = 20;                             // 占空比5%, 2.5kHz方波 → 蜂鸣器鸣响
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   // LED 点亮
    }
    else
    {
      TIM2->CCR1 = 0;                              // 占空比0%, 输出持续高电平 → 蜂鸣器关闭
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // LED 熄灭
    }

    HAL_Delay(1000);                // 每秒更新一次
    /* USER CODE END WHILE */
  }
}

/**
  * @brief  蜂鸣器 PWM 初始化 — TIM2_CH1 (PA0) 输出 2.5kHz 方波
  * @note   使用直接寄存器操作, 不依赖 HAL TIM 驱动
  *         TIM2 时钟 = APB1×2 = 72MHz, PSC=71 → 1MHz, ARR=399 → 2.5kHz
  *         PWM模式1 + 低电平有效:
  *           CCR1=0   → CNT 始终 >= CCR1 → 输出恒高 → 蜂鸣器关闭
  *           CCR1=200 → CNT < CCR1 时输出低, >= CCR1 时输出高 → 50%方波 → 鸣响
  * @retval None
  */
static void Buzzer_PWM_Init(void)
{
  /* 开启 TIM2 时钟 (APB1) */
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  /* 时基配置: PSC=71, ARR=399 → 72MHz/(71+1)/(399+1) = 2.5kHz */
  TIM2->PSC = 71;
  TIM2->ARR = 399;
  TIM2->CCR1 = 0;                   // 初始占空比 0% → 蜂鸣器不响

  /* PWM模式1: CNT < CCR1 时输出有效电平 */
  TIM2->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;  // OC1M = 110 (PWM mode 1)
  TIM2->CCMR1 |= TIM_CCMR1_OC1PE;   // 输出比较1预装载使能

  /* 输出极性: 低电平有效, 使能 CH1 输出 */
  TIM2->CCER |= TIM_CCER_CC1P;      // CC1P = 1 (active low)
  TIM2->CCER |= TIM_CCER_CC1E;      // CC1E = 1 (enable output)

  /* 自动重装载预装载使能 + 启动计数器 */
  TIM2->CR1 |= TIM_CR1_ARPE;
  TIM2->CR1 |= TIM_CR1_CEN;         // 启动 TIM2
}

/**
  * @brief  系统时钟配置: HSE (8MHz) → PLL ×9 → SYSCLK = 72MHz
  * @note   APB1 = 36MHz, APB2 = 72MHz, HCLK = 72MHz, Flash Latency = 2
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** HSE + PLL 配置 */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;            // 开启外部 8MHz 晶振
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;        // 8MHz × 9 = 72MHz
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** 总线时钟分配 */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;  // PLL 作为系统时钟
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;          // HCLK = 72MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;           // APB1 = 36MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;           // APB2 = 72MHz

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  严重错误处理 — 关全局中断，死循环等待看门狗复位
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
