# STM32 Temperature Detection System

基于 STM32F103C8T6 的温度监测告警系统 — DS18B20 测温 + OLED 显示 + 蜂鸣器/LED 双重告警 + 蓝牙远程控制。

## 功能特性

| 功能 | 说明 |
|------|------|
| 🌡️ **DS18B20 测温** | 12-bit 分辨率（0.0625°C/LSB），每秒刷新 |
| 📺 **OLED 显示** | 0.96" I2C OLED (SSD1306, 128×64)，实时显示温度 |
| 🔊 **蜂鸣器告警** | TIM2 PWM 驱动，温度超阈值自动鸣响 |
| 💡 **LED 告警** | PA2 驱动，与蜂鸣器同步触发，声光双重提醒 |
| 📱 **蓝牙远程控制** | JDY-31 BLE 模块，手机 App 实时查看温度 + 远程修改报警阈值 |
| ⚙️ **动态阈值** | 手机端发送指令即可随时调整告警温度，无需重新烧录 |

## 硬件清单

| 元件 | 型号 | 数量 | 备注 |
|------|------|:--:|------|
| 主控 | STM32F103C8T6 (Blue Pill) | 1 | ARM Cortex-M3, 72MHz |
| 温度传感器 | DS18B20 (TO-92) | 1 | OneWire 协议 |
| 显示屏 | 0.96" I2C OLED (SSD1306) | 1 | 128×64, 地址 0x3C |
| 蜂鸣器 | 有源蜂鸣器模块 (低电平触发) | 1 | TIM2_CH1 PWM 驱动 |
| LED | 5mm LED (颜色任意) | 1 | 告警指示灯 |
| 电阻 1kΩ | 0805/直插 | 1 | LED 限流 (~1.3mA) |
| 电阻 4.7kΩ | 0805/直插 | 1 | DS18B20 DQ 上拉 |
| 蓝牙模块 | JDY-31 (BLE 4.2) | 1 | 手机远程通信 |
| 烧录器 | ST-Link V2 | 1 | SWD 接口 |

## 引脚接线

```
STM32F103C8T6
┌──────────────────────────────────┐
│                                  │
│  PA0  ─── 蜂鸣器 I/O (TIM2_CH1)   │
│  PA1  ─── DS18B20 DQ ──┬── 3.3V  │
│  PA2  ─── 1kΩ ──▶├── GND        │
│              (LED)               │
│  PA9  ─── JDY-31 RXD             │
│  PA10 ─── JDY-31 TXD             │
│                                  │
│  PB8  ─── OLED SCL (I2C1)        │
│  PB9  ─── OLED SDA (I2C1)        │
│                                  │
│  3.3V ─── OLED / DS18B20 / JDY-31│
│  GND  ─── 公共地                 │
│                                  │
│  PA13 ─── ST-Link SWDIO          │
│  PA14 ─── ST-Link SWCLK          │
└──────────────────────────────────┘
```

### JDY-31 蓝牙模块接线

| JDY-31 引脚 | 连接 | 说明 |
|-------------|------|------|
| VCC | STM32 **3.3V** | ⚠️ 绝不能接 5V |
| GND | STM32 GND | |
| TXD | STM32 PA10 (USART1_RX) | JDY 发送 → STM32 接收 |
| RXD | STM32 PA9 (USART1_TX) | STM32 发送 → JDY 接收 |

> 波特率 9600bps，BLE 名称 `JDY-31-SPP`。手机端推荐 App：Android 用 **BLE调试助手**，iOS 用 **LightBlue**。

### 蓝牙通信协议

```
STM32 → 手机 (实时数据，每秒1次):
  T:25.68 TH:27.0              当前温度 + 报警阈值
  T:28.15 TH:27.0 ALARM:ON     超温时附加告警标志

手机 → STM32 (修改阈值):
  SET:TH=30.5                  将报警温度设为 30.5℃
```

## 项目结构

```
STM32-Temperature-Detection-System/
├── Core/
│   ├── Inc/                   # 头文件
│   │   ├── main.h             #   主程序 + 引脚宏定义
│   │   ├── ds18b20.h          #   DS18B20 驱动声明
│   │   ├── oled.h             #   SSD1306 OLED 驱动声明
│   │   ├── delay.h            #   微秒延时声明
│   │   ├── i2c.h              #   I2C 句柄声明
│   │   ├── gpio.h             #   GPIO 句柄声明
│   │   ├── stm32f1xx_hal_conf.h
│   │   └── stm32f1xx_it.h
│   └── Src/                   # 源文件
│       ├── main.c             #   主程序 (初始化+自检+主循环)
│       ├── ds18b20.c          #   DS18B20 OneWire 协议实现
│       ├── oled.c             #   SSD1306 驱动 + 8×16 ASCII 字库
│       ├── delay.c            #   SysTick 微秒延时
│       ├── i2c.c              #   I2C1 初始化 (100kHz)
│       ├── gpio.c             #   GPIO 初始化
│       ├── stm32f1xx_hal_msp.c
│       ├── stm32f1xx_it.c
│       ├── system_stm32f1xx.c
│       ├── syscalls.c
│       └── sysmem.c
├── Drivers/                   # STM32 HAL 库 + CMSIS
├── cmake/                     # CMake 工具链 + CubeMX 子模块
│   ├── gcc-arm-none-eabi.cmake
│   ├── starm-clang.cmake
│   └── stm32cubemx/
├── CMakeLists.txt             # 顶层 CMake (项目名: TempCheck)
├── CMakePresets.json
├── README.md
└── .gitignore
```

## 编译与烧录

### 前置条件

- [STM32CubeCLT](https://www.st.com/en/development-tools/stm32cubeclt.html) — ARM GCC + CMake + Ninja + STM32CubeProgrammer
- [OpenOCD](https://github.com/openocd-org/openocd) — 烧录工具（对 ST-Link 克隆版兼容性更好）

### 编译

```bash
cd build/Release
ninja
```

编译产物：
- `TempCheck.elf` — ELF 固件（含调试信息）
- `TempCheck.hex` — Intel Hex 格式
- `TempCheck.bin` — 原始二进制

### 烧录

**OpenOCD（推荐，兼容性好）：**

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Release/TempCheck.elf verify reset exit"
```

**STM32CubeProgrammer CLI：**

```bash
STM32_Programmer_CLI -c port=SWD freq=4000 \
  -w build/Release/TempCheck.elf -v -s
```

## 使用说明

### 启动流程

1. 按引脚接线表连好所有硬件
2. 烧录固件后上电
3. OLED 显示自检画面：
   - `Temperature:` — 标题行
   - `DS18B20 OK!` — 传感器正常，或 `DS18B20 Err!` — 传感器异常（蜂鸣器长鸣，LED 闪烁）
4. 1 秒后进入主界面，每秒刷新：
   - 第 1 行：`Temperature:`
   - 第 2 行：`Temp:XX.XX C`

### 告警行为

| 条件 | 蜂鸣器 | LED |
|------|:---:|:---:|
| 温度 ≤ 阈值 (默认 27°C) | 🔇 关闭 | ⚫ 熄灭 |
| 温度 > 阈值 | 🔊 鸣响 | 💡 点亮 |

### 蓝牙远程操作

1. 手机打开 BLE 调试 App
2. 扫描并连接 `JDY-31-SPP`
3. 进入 UART 服务通道
4. 自动收到实时数据 `T:25.68 TH:27.0`
5. 发送 `SET:TH=30.5` 即可将阈值改为 30.5°C

### 调整蜂鸣器音量

修改 `Core/Src/main.c` 中 `TIM2->CCR1` 的值：

| 音量 | CCR1 | 占空比 |
|------|------|:---:|
| 很响 | 200  | 50% |
| 适中 | 80   | 20% |
| 轻声 | 40   | 10% |
| 微弱 | 20   | 5% |
| 静音 | 0    | 0%  |

## 中国大陆网络提示

GitHub HTTPS 直连可能不稳定，推荐使用 SSH over 443 端口：

```bash
# ~/.ssh/config
Host github.com
    Hostname ssh.github.com
    Port 443
    User git
    IdentityFile ~/.ssh/id_ed25519
```

或使用镜像加速：
```bash
git remote set-url origin https://ghproxy.com/https://github.com/<user>/<repo>.git
```

---

*Built with STM32CubeMX + CMake + Ninja + ARM GCC*

*MCU: STM32F103C8T6 · 72MHz · 64KB Flash · 20KB RAM*
