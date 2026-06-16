/**
 * @file    ms41929.h
 * @brief   MS41929电机驱动头文件
 * @details 定义MS41929电机驱动的宏、枚举、结构体和函数原型
 * @author  SPRING
 * @date    2026-01-08
 */

#ifndef MS41929_H
#define MS41929_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

// 驱动参数宏定义
#define MS41929_VD_FREQ             50U     // VD_FZ同步频率（Hz）
#define MS41929_OSC_FREQ            27000000UL // OSCIN频率（27MHz）
#define MS41929_PPS                 800U    // 每步脉冲数
#define MS41929_CURRENT_RATIO       50U     // 电流比率（%）
#define MS41929_PWMMODE             8U      // PWM模式
#define MS41929_PPWX                (8U * MS41929_PWMMODE * MS41929_CURRENT_RATIO / 100U) // 峰值脉冲宽度

// 步进电机参数计算宏
#define MS41929_PSUMXX              ((4U * MS41929_PPS) / MS41929_VD_FREQ)
#define MS41929_INTCTXX             (MS41929_OSC_FREQ / (MS41929_VD_FREQ * MS41929_PSUMXX* 24U) )

// 寄存器地址定义
#define MS41929_REG_TESTEN1          0x0B    // TEST模式使能与VD_FZ极性选择
#define MS41929_REG_PWM_DT1          0x20    // PWM频率设置与起始点等待时间
#define MS41929_REG_TESTMODE         0x21    // 测试信号输出选择
#define MS41929_REG_DT2A_PHMODAB     0x22    // α电机起始点激励等待时间与相位矫正
#define MS41929_REG_PPWA_PPWB        0x23    // α电机A/B峰值脉冲宽度设置
#define MS41929_REG_PSUMAB_CTRL      0x24    // α电机步进数、方向、使能等控制
#define MS41929_REG_INTCTAB          0x25    // α电机微步进周期设置
#define MS41929_REG_DT2B_PHMODCD     0x27    // β电机起始点激励等待时间与相位矫正
#define MS41929_REG_PPWC_PPWD        0x28    // β电机C/D峰值脉冲宽度设置
#define MS41929_REG_PSUMCD_CTRL      0x29    // β电机步进数、方向、使能等控制
#define MS41929_REG_INTCTCD          0x2A    // β电机微步进周期设置
#define MS41929_REG_DC_MOTOR         0x2C    // 直流电机(IR-CUT)控制

// 寄存器位定义

// 0x0B寄存器位定义
#define MS41929_TESTEN1_BIT          15      // TEST模式使能1
#define MS41929_MODESEL_FZ_BIT       14      // VD_FZ极性选择位

// 0x20寄存器位定义
#define MS41929_DT1_MASK             0x00FF  // 起始点等待时间掩码
#define MS41929_PWMMODE_MASK         0x03E0  // PWM模式掩码
#define MS41929_PWMMODE_SHIFT        5       // PWM模式位移
#define MS41929_PWMRES_MASK          0x0C00  // PWM分辨率掩码
#define MS41929_PWMRES_SHIFT         10      // PWM分辨率位移

// 0x22寄存器位定义
#define MS41929_DT2A_MASK            0x00FF  // α电机起始点激励等待时间掩码
#define MS41929_PHMODAB_MASK         0x03F0  // α电机相位矫正掩码
#define MS41929_PHMODAB_SHIFT        4       // α电机相位矫正位移

// 0x23寄存器位定义
#define MS41929_PPWA_MASK            0x00FF  // α电机A峰值脉冲宽度掩码
#define MS41929_PPWB_MASK            0xFF00  // α电机B峰值脉冲宽度掩码
#define MS41929_PPWB_SHIFT           8       // α电机B峰值脉冲宽度位移

// 0x24寄存器位定义
#define MS41929_PSUMAB_MASK          0x00FF  // α电机步进数掩码
#define MS41929_CCWCWAB_BIT          8       // α电机转动方向位
#define MS41929_BRAKEAB_BIT          9       // α电机刹车状态位
#define MS41929_ENDISAB_BIT          10      // α电机使能位
#define MS41929_LEDB_BIT             11      // LED B控制位
#define MS41929_MICROAB_MASK         0x0C00  // α电机细分模式掩码
#define MS41929_MICROAB_SHIFT        12      // α电机细分模式位移

// 0x29寄存器位定义
#define MS41929_PSUMCD_MASK          0x00FF  // β电机步进数掩码
#define MS41929_CCWCWCD_BIT          8       // β电机转动方向位
#define MS41929_BRAKECD_BIT          9       // β电机刹车状态位
#define MS41929_ENDISCD_BIT          10      // β电机使能位
#define MS41929_LEDA_BIT             11      // LED A控制位
#define MS41929_MICROCD_MASK         0x0C00  // β电机细分模式掩码
#define MS41929_MICROCD_SHIFT        12      // β电机细分模式位移

// 0x2C寄存器位定义
#define MS41929_SWICH_BIT            2       // 直流电机控制模式选择位
#define MS41929_IN1_BIT              1       // 直流电机输入控制1位
#define MS41929_IN2_BIT              0       // 直流电机输入控制2位

// 电机通道枚举
typedef enum {
    MS41929_CHANNEL_AB = 0,  // AB通道（α电机）
    MS41929_CHANNEL_CD = 1   // CD通道（β电机）
} MS41929_MotorChannel_e;

// 电机细分模式枚举
typedef enum {
    MS41929_SUBDIV_64 = 0,   // 64细分
    MS41929_SUBDIV_128 = 1,  // 128细分
    MS41929_SUBDIV_256 = 2   // 256细分
} MS41929_SubdivisionMode_e;

// 电机细分模式定义
#define MS41929_MICRO_64             0x00    // 64细分
#define MS41929_MICRO_128            0x01    // 128细分
#define MS41929_MICRO_256            0x02    // 256细分

// 直流电机状态定义
#define MS41929_DC_FLOAT             0       // 滑行
#define MS41929_DC_REVERSE           1       // 反转
#define MS41929_DC_FORWARD           2       // 正转
#define MS41929_DC_BRAKE             3       // 刹车

// 电机通道定义
#define MS41929_CH_AB                0       // AB通道（α电机）
#define MS41929_CH_CD                1       // CD通道（β电机）

// 用户可配置的硬件相关宏定义
#define MS41929_SPI_HANDLE           (&hspi1)        // SPI句柄
#define MS41929_CS_PORT              GPIOA           // CS引脚端口
#define MS41929_CS_PIN               GPIO_PIN_4      // CS引脚号
#define MS41929_RSTB_PORT            GPIOA           // RSTB引脚端口
#define MS41929_RSTB_PIN             GPIO_PIN_3      // RSTB引脚号
#define MS41929_VDFZ_PORT            GPIOA           // VDFZ引脚端口
#define MS41929_VDFZ_PIN             GPIO_PIN_2      // VDFZ引脚号
#define MS41929_IN1_PORT             GPIOB           // IN1引脚端口
#define MS41929_IN1_PIN              GPIO_PIN_5      // IN1引脚号
#define MS41929_IN2_PORT             GPIOB           // IN2引脚端口
#define MS41929_IN2_PIN              GPIO_PIN_6      // IN2引脚号

// 函数声明

/**
 * @brief  写MS41929寄存器
 * @param  reg_addr：寄存器地址
 * @param  data：16位写入数据
 * @retval 无
 */
void MS41929_WriteReg(uint8_t reg_addr, uint16_t data);

/**
 * @brief  读MS41929寄存器
 * @param  reg_addr：寄存器地址
 * @retval 16位读取数据
 */
uint16_t MS41929_ReadReg(uint8_t reg_addr);

/**
 * @brief  MS41929硬件复位
 * @param  无
 * @retval 无
 */
void MS41929_Reset(void);

/**
 * @brief  步进电机初始化
 * @param  micro_mode：细分模式（MS41929_SUBDIV_64/128/256）
 * @retval HAL_StatusTypeDef：操作状态
 */
HAL_StatusTypeDef MS41929_Stepper_Init(MS41929_SubdivisionMode_e micro_mode);

/**
 * @brief  步进电机运行控制
 * @param  channel：电机通道（MS41929_CHANNEL_AB或MS41929_CHANNEL_CD）
 * @param  direction：转动方向（1=正向，0=反向）
 * @param  speed：速度值（1-255，值越大速度越快）
 * @retval 无
 */
void MS41929_Stepper_Run(MS41929_MotorChannel_e channel, uint8_t direction, uint8_t speed);

/**
 * @brief  步进电机停止
 * @param  channel：电机通道
 * @param  is_brake：0=正常停止，1=紧急刹车
 * @retval 无
 */
void MS41929_Stepper_Stop(MS41929_MotorChannel_e channel, uint8_t is_brake);

/**
 * @brief  步进电机使能控制
 * @param  channel：电机通道（MS41929_CHANNEL_AB或MS41929_CHANNEL_CD）
 * @param  enable：0=禁用，1=使能
 * @retval 无
 */
void MS41929_Stepper_Enable(MS41929_MotorChannel_e channel, uint8_t enable);

/**
 * @brief  触发VD_FZ同步信号
 * @param  无
 * @retval 无
 */
void MS41929_TriggerVD_FZ(void);

/**
 * @brief  直流电机直接模式控制
 * @param  state：0=滑行，1=反转，2=正转，3=刹车
 * @retval 无
 */
void MS41929_DC_Motor_Control(uint8_t state);

/**
 * @brief  直流电机SPI模式控制
 * @param  state：0=滑行，1=反转，2=正转，3=刹车
 * @retval 无
 */
void MS41929_DC_Motor_SPI_Control(uint8_t state);

/**
 * @brief  LED控制
 * @param  led_a：LED A状态（0=关闭，1=开启）
 * @param  led_b：LED B状态（0=关闭，1=开启）
 * @retval 无
 */
void MS41929_LED_Control(uint8_t led_a, uint8_t led_b);

/**
 * @brief  获取电机状态
 * @param  channel：电机通道
 * @retval uint16_t：电机状态寄存器值
 */
uint16_t MS41929_GetMotorStatus(MS41929_MotorChannel_e channel);

#endif /* MS41929_H */
