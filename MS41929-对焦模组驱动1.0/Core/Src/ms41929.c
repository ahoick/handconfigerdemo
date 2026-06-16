/**
 * @file    ms41929.c
 * @brief   MS41929电机驱动实现
 * @details 实现MS41929芯片的所有功能，包括步进电机控制、直流电机控制和LED控制
 * @author  SPRING
 * @date    2026-01-08
 */

#include "ms41929.h"
#include "spi.h"
#include <stdio.h>
#include <string.h>
// 微步进模式值
#define MS41929_MICRO_MODE_VALUE MS41929_MICRO_64  // 默认微步进模式：64细分

/**
 * @brief  写MS41929寄存器
 * @param  reg_addr：寄存器地址
 * @param  data：写入数据
 * @retval 无
 */
 
 static void MS41929_Delay_us(uint32_t us) {
    // 针对 STM32F1/F4 等常见频率的粗略 μs 延时，根据你的单片机频率可适当微调
    uint32_t delay = us * 10; 
    while(delay--) {
        __NOP();
    }
}
 
void MS41929_WriteReg(uint8_t reg_addr, uint16_t data) {  
    
    // 24位数据：[C1 C0][A5~A0][D15~D0]
    uint8_t spi_buf[3];  
    uint32_t timeout = 100;
     
    spi_buf[0] = reg_addr & 0xBF;
	spi_buf[1] = data & 0xFF;  
    spi_buf[2] = (data >> 8) & 0xFF; 
     
    // 片选拉高
    HAL_GPIO_WritePin(MS41929_CS_PORT, MS41929_CS_PIN, GPIO_PIN_SET);
    
    // 满足CS建立时间t3≥60ns
    for(uint8_t i = 0; i < 30; i++) {
        __NOP();
    }
    
    // 发送24位数据
    HAL_SPI_Transmit(MS41929_SPI_HANDLE, spi_buf, 3, timeout);
    MS41929_Delay_us(10);  // 等待写入完成
    // 满足CS保持时间t4≥60ns
    for(uint8_t i = 0; i < 30; i++) {
        __NOP();
    }
    
    // 片选拉低
    HAL_GPIO_WritePin(MS41929_CS_PORT, MS41929_CS_PIN, GPIO_PIN_RESET);     
}

/**
 * @brief  读MS41929寄存器
 * @param  reg_addr：寄存器地址
 * @retval 16位读取数据
 */
uint16_t MS41929_ReadReg(uint8_t reg_addr) {
    uint8_t tx_buf[3] = {0};
    uint8_t rx_buf[3] = {0};
    uint32_t timeout = 100;
    
    // 组装24位读命令：  
    tx_buf[0] = reg_addr | 0x40;
    tx_buf[1] = 0xFF;
    tx_buf[2] = 0xFF;
    
    // 片选拉高
    HAL_GPIO_WritePin(MS41929_CS_PORT, MS41929_CS_PIN, GPIO_PIN_SET);
    
    // 满足CS建立时间t3≥60ns
    for(uint8_t i = 0; i < 10; i++) {
        __NOP();
    }
    
    // 发送读命令并接收数据
    HAL_SPI_TransmitReceive(MS41929_SPI_HANDLE, tx_buf, rx_buf, 3, timeout);
    
    // 满足CS保持时间t4≥60ns
    for(uint8_t i = 0; i < 10; i++) {
        __NOP();
    }
    
    // 片选拉低
    HAL_GPIO_WritePin(MS41929_CS_PORT, MS41929_CS_PIN, GPIO_PIN_RESET);
    uint16_t raw_rx = (rx_buf[2] << 8) | rx_buf[1];
    return raw_rx;
}

/**
 * @brief  MS41929硬件复位
 * @param  无
 * @retval 无
 */
void MS41929_Reset(void) {
    // RSTB拉低≥100μs
    HAL_GPIO_WritePin(MS41929_RSTB_PORT, MS41929_RSTB_PIN, GPIO_PIN_RESET);
    
    // 延时200μs
    for(uint32_t i = 0; i < 2000; i++) {
        __NOP();
    }
    
    // RSTB拉高，退出复位
    HAL_GPIO_WritePin(MS41929_RSTB_PORT, MS41929_RSTB_PIN, GPIO_PIN_SET);
    
    // 等待复位后稳定（1ms）
    HAL_Delay(2);
}

/**
 * @brief  步进电机初始化
 * @param  channel：电机通道
 * @param  micro_mode：细分模式
 * @retval HAL_StatusTypeDef：操作状态
 */
HAL_StatusTypeDef MS41929_Stepper_Init(MS41929_SubdivisionMode_e micro_mode) {   
    // 1. 复位芯片
    MS41929_Reset();
	MS41929_TriggerVD_FZ();
    // 0x0B
    MS41929_WriteReg(0x0B, 0x0200);  
    // 0x20
    MS41929_WriteReg(0x20, (1 << 13) | (MS41929_PWMMODE << 8) | 0x0F);

    // 0x22: PHMODAB[5:0]:D13~D8;DT2A[7:0]:D7~D0
    MS41929_WriteReg(MS41929_REG_DT2A_PHMODAB, 0x000F); 
    // 0x23: PPWB[7:0]:D15~D8;PPWA[7:0]:D7~D0
    MS41929_WriteReg(MS41929_REG_PPWA_PPWB, (MS41929_PPWX << 8) | MS41929_PPWX); 
    
    // 0x24: MICROAB[1:0]:D13~D12;LEDB:D11;ENDISAB:D10;BRAKEAB:D9;CCWCWAB:D8;PSUMAB[7:0]:D7~D0
    MS41929_WriteReg(MS41929_REG_PSUMAB_CTRL, (micro_mode << 12) | (0 << 11) | (1 << 10) | (0 << 9) | (0 << 8) | 0x0000);
    // 0x27: PHMODCD[5:0]:D13~D8;DT2B[7:0]:D7~D0
    MS41929_WriteReg(MS41929_REG_DT2B_PHMODCD, 0x000F); 
    // 0x28: PPWC[7:0]:D15~D8;PPWD[7:0]:D7~D0
    MS41929_WriteReg(MS41929_REG_PPWC_PPWD, (MS41929_PPWX << 8) | MS41929_PPWX); 
    
    // 0x29: MICROCD[1:0]:D13~D12;LEDA:D11;ENDISCD:D10;BRAKECD:D9;CCWCWCD:D8;PSUMCD[7:0]:D7~D0
    MS41929_WriteReg(MS41929_REG_PSUMCD_CTRL, (micro_mode << 12) | (0 << 11) | (1 << 10) | (0 << 9) | (0 << 8) | 0x0000);
        MS41929_TriggerVD_FZ();
    // 返回成功状态
    return HAL_OK;
}

/**
 * @brief  步进电机运行控制
 * @param  channel：电机通道
 * @param  direction：转动方向（1=正向，0=反向）
 * @param  speed：速度值（1-255，值越大速度越快）
 * @retval 无
 */
void MS41929_Stepper_Run(MS41929_MotorChannel_e channel, uint8_t direction, uint8_t speed) {
		MS41929_TriggerVD_FZ();
    
    // 1. 安全限制：输入速度严格限制在 0 ~ 100
    if (speed > 100) speed = 100;

    // 2. 丝滑调速线性映射（完全符合你：0最快，352最慢的底层逻辑）
    // speed = 100 时，计算结果为 0
    // speed = 0   时，计算结果为 352
    uint32_t actual_speed = (352U * (100U - speed)) / 100U;

    // 3. 规避硬件卡死硬限制：MS41929 寄存器写入 0 会导致芯片状态机死锁
    // 如果算出来是 0（最快），强行转为 1，确保总线安全畅通
    if (actual_speed == 0) actual_speed = 1;
    if (actual_speed > 352) actual_speed = 352;
    
    if(channel == MS41929_CHANNEL_AB) {
        MS41929_WriteReg(MS41929_REG_PSUMAB_CTRL, 
                    (MS41929_MICRO_MODE_VALUE << 12) |  // 微步进模式
                    (0 << 11) |                        // LED B状态
                    (1 << 10) |                        // 使能电机
                    (0 << 9) |                         // 刹车状态
                    (direction << 8) |                 // 转动方向
                    MS41929_PSUMXX);                   // 步数
        
        // 设置速度
        MS41929_WriteReg(MS41929_REG_INTCTAB, actual_speed);
    } else if(channel == MS41929_CHANNEL_CD) {
        MS41929_WriteReg(MS41929_REG_PSUMCD_CTRL, 
                    (MS41929_MICRO_MODE_VALUE << 12) |  // 微步进模式 MICROCD[1:0]
                    (0 << 11) |                        // LED A 状态（LEDA，bit11）
                    (1 << 10) |                        // 使能电机 ENDISCD
                    (0 << 9) |                         // 刹车状态 BRAKECD
                    (direction << 8) |                 // 转动方向 CCWCWCD
                    MS41929_PSUMXX);                   // 步数 PSUMCD
        MS41929_WriteReg(MS41929_REG_INTCTCD, actual_speed);
    }

}


/**
 * @brief  步进电机停止
 * @param  channel：电机通道
 * @param  is_brake：0=正常停止，1=紧急刹车
 * @retval 无
 */
void MS41929_Stepper_Stop(MS41929_MotorChannel_e channel, uint8_t is_brake) {
    uint16_t reg_val;
    
  	  // 触发VD_FZ信号使配置生效
    MS41929_TriggerVD_FZ();
    
    if(channel == MS41929_CHANNEL_AB) {
        // 读取当前寄存器值
        reg_val = MS41929_ReadReg(MS41929_REG_PSUMAB_CTRL);
        
        if(is_brake) {
            // 紧急刹车（BRAKEAB=1）
            reg_val |= (1 << MS41929_BRAKEAB_BIT);
        } else {
            // 正常停止（PSUMAB=0，清除刹车位）
            reg_val &= ~(MS41929_PSUMAB_MASK | (1 << MS41929_BRAKEAB_BIT));
        }
        
        // 写入更新后的值
        MS41929_WriteReg(MS41929_REG_PSUMAB_CTRL, reg_val);
    } else if(channel == MS41929_CHANNEL_CD) {
        // 读取当前寄存器值
        reg_val = MS41929_ReadReg(MS41929_REG_PSUMCD_CTRL);
        
        if(is_brake) {
            // 紧急刹车（BRAKECD=1）
            reg_val |= (1 << MS41929_BRAKECD_BIT);
        } else {
            // 正常停止（PSUMCD=0，清除刹车位）
            reg_val &= ~(MS41929_PSUMCD_MASK | (1 << MS41929_BRAKECD_BIT));
        }
        
        // 写入更新后的值
        MS41929_WriteReg(MS41929_REG_PSUMCD_CTRL, reg_val);
    }

}

/**
 * @brief  步进电机使能控制
 * @param  channel：电机通道
 * @param  enable：0=禁用，1=使能
 * @retval 无
 */
void MS41929_Stepper_Enable(MS41929_MotorChannel_e channel, uint8_t enable) {
    uint16_t reg_val;
        // 修改使能状态前先触发VDFZ同步信号
    MS41929_TriggerVD_FZ();
    
    if(channel == MS41929_CHANNEL_AB) {
        reg_val = MS41929_ReadReg(MS41929_REG_PSUMAB_CTRL);
        if(enable) {
            reg_val |= (1 << MS41929_ENDISAB_BIT);
        } else {
            reg_val &= ~(1 << MS41929_ENDISAB_BIT);
        }
        MS41929_WriteReg(MS41929_REG_PSUMAB_CTRL, reg_val);
    } else if(channel == MS41929_CHANNEL_CD) {
        reg_val = MS41929_ReadReg(MS41929_REG_PSUMCD_CTRL);
        if(enable) {
            reg_val |= (1 << MS41929_ENDISCD_BIT);
        } else {
            reg_val &= ~(1 << MS41929_ENDISCD_BIT);
        }
        MS41929_WriteReg(MS41929_REG_PSUMCD_CTRL, reg_val);
    }


}

/**
 * @brief  触发VD_FZ同步信号
 * @param  无
 * @retval 无
 */
void MS41929_TriggerVD_FZ(void) {
    // 输出高电平
    HAL_GPIO_WritePin(MS41929_VDFZ_PORT, MS41929_VDFZ_PIN, GPIO_PIN_SET);
    
    // 增加延时时间，确保上升沿被检测到
    // 使用HAL_Delay确保足够的延时时间（约1ms）
    MS41929_Delay_us(50);
    
    // 输出低电平
    HAL_GPIO_WritePin(MS41929_VDFZ_PORT, MS41929_VDFZ_PIN, GPIO_PIN_RESET);
    
    // 再次延时，确保信号完整
    MS41929_Delay_us(50);
}

/**
 * @brief  直流电机直接模式控制
 * @param  state：0=滑行，1=反转，2=正转，3=刹车
 * @retval 无
 */
void MS41929_DC_Motor_Control(uint8_t state) {
    switch (state) {
        case MS41929_DC_FLOAT:  // 滑行：IN1=0，IN2=0
            HAL_GPIO_WritePin(MS41929_IN1_PORT, MS41929_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MS41929_IN2_PORT, MS41929_IN2_PIN, GPIO_PIN_RESET);
            break;
        case MS41929_DC_REVERSE:  // 反转：IN1=0，IN2=1
            HAL_GPIO_WritePin(MS41929_IN1_PORT, MS41929_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MS41929_IN2_PORT, MS41929_IN2_PIN, GPIO_PIN_SET);
            break;
        case MS41929_DC_FORWARD:  // 正转：IN1=1，IN2=0
            HAL_GPIO_WritePin(MS41929_IN1_PORT, MS41929_IN1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MS41929_IN2_PORT, MS41929_IN2_PIN, GPIO_PIN_RESET);
            break;
        case MS41929_DC_BRAKE:  // 刹车：IN1=1，IN2=1
            HAL_GPIO_WritePin(MS41929_IN1_PORT, MS41929_IN1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MS41929_IN2_PORT, MS41929_IN2_PIN, GPIO_PIN_SET);
            break;
        default:
            // 默认为滑行状态
            HAL_GPIO_WritePin(MS41929_IN1_PORT, MS41929_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MS41929_IN2_PORT, MS41929_IN2_PIN, GPIO_PIN_RESET);
            break;
    }
}

/**
 * @brief  直流电机SPI模式控制
 * @param  state：0=滑行，1=反转，2=正转，3=刹车
 * @retval 无
 */
void MS41929_DC_Motor_SPI_Control(uint8_t state) {
    uint16_t reg_val = 0;
    
    // 1. 切换至SPI模式（SWICH=1）
    reg_val = MS41929_ReadReg(MS41929_REG_DC_MOTOR) | (1 << MS41929_SWICH_BIT);
    
    // 2. 清除IN1/IN2位
    reg_val &= ~((1 << MS41929_IN1_BIT) | (1 << MS41929_IN2_BIT));
    
    // 3. 配置IN1/IN2
    switch (state) {
        case MS41929_DC_FLOAT:  // 滑行：IN1=0，IN2=0
            break;
        case MS41929_DC_REVERSE:  // 反转：IN1=0，IN2=1
            reg_val |= (1 << MS41929_IN2_BIT);
            break;
        case MS41929_DC_FORWARD:  // 正转：IN1=1，IN2=0
            reg_val |= (1 << MS41929_IN1_BIT);
            break;
        case MS41929_DC_BRAKE:  // 刹车：IN1=1，IN2=1
            reg_val |= (1 << MS41929_IN1_BIT) | (1 << MS41929_IN2_BIT);
            break;
        default:
            break;
    }
    
    // 4. 写入寄存器
    MS41929_WriteReg(MS41929_REG_DC_MOTOR, reg_val);
}

/**
 * @brief  LED控制
 * @param  led_a：LED A状态（0=关闭，1=开启）
 * @param  led_b：LED B状态（0=关闭，1=开启）
 * @retval 无
 */
void MS41929_LED_Control(uint8_t led_a, uint8_t led_b) {
    uint16_t reg_val;
    
    // 控制LED A（位于PSUMCD_CTRL寄存器）
    reg_val = MS41929_ReadReg(MS41929_REG_PSUMCD_CTRL);
    if(led_a) {
        reg_val |= (1 << MS41929_LEDA_BIT);
    } else {
        reg_val &= ~(1 << MS41929_LEDA_BIT);
    }
    MS41929_WriteReg(MS41929_REG_PSUMCD_CTRL, reg_val);
    
    // 控制LED B（位于PSUMAB_CTRL寄存器）
    reg_val = MS41929_ReadReg(MS41929_REG_PSUMAB_CTRL);
    if(led_b) {
        reg_val |= (1 << MS41929_LEDB_BIT);
    } else {
        reg_val &= ~(1 << MS41929_LEDB_BIT);
    }
    MS41929_WriteReg(MS41929_REG_PSUMAB_CTRL, reg_val);
}

/**
 * @brief  获取电机状态
 * @param  channel：电机通道
 * @retval uint16_t：电机状态寄存器值
 */
uint16_t MS41929_GetMotorStatus(MS41929_MotorChannel_e channel) {
    if(channel == MS41929_CHANNEL_AB) {
        // 读取AB通道（α电机）状态寄存器
        return MS41929_ReadReg(MS41929_REG_PSUMAB_CTRL);
    } else if(channel == MS41929_CHANNEL_CD) {
        // 读取CD通道（β电机）状态寄存器
        return MS41929_ReadReg(MS41929_REG_PSUMCD_CTRL);
    }
    return 0;
}
