/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/* USART3 init function (LD3320 语音模块) */
void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
#include "ms41929.h"
#include <string.h>

// 全局变量
uint8_t uart_rx_buffer[sizeof(CommandFrame_t)];
uint8_t uart_rx_index = 0;
uint8_t uart_single_byte_rx = 0;
uint8_t command_received = 0;
CommandFrame_t current_command;
volatile uint8_t buzzer_on = 0;
volatile uint32_t buzzer_on_time = 0;

// ─── ISR 内安全的串口发送（直接写寄存器，不经过 HAL 状态机） ───
static void UART_SendByte_ISR(UART_HandleTypeDef *huart, uint8_t byte)
{
    uint32_t timeout = 100000;
    while (!(huart->Instance->SR & USART_SR_TXE) && --timeout);
    huart->Instance->DR = byte;
}

// LD3320 语音模块帧解析状态机
// 帧格式: AA 55 [KEYWORD_ID] 55 AA
#define LD3320_FRAME_LEN  5
#define LD3320_HDR1       0xAA
#define LD3320_HDR2       0x55
#define LD3320_TAIL1      0x55
#define LD3320_TAIL2      0xAA

uint8_t ld3320_rx_byte = 0;
uint8_t ld3320_frame_buf[LD3320_FRAME_LEN];
uint8_t ld3320_frame_idx = 0;
uint8_t ld3320_frame_ready = 0;
/**
 * @brief  串口接收中断回调函数（可靠状态机）
 */
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART1)
//    {
//        uint8_t received_byte = uart_single_byte_rx;

//        if (uart_rx_index == 0)
//        {
//            if (received_byte == 0xAA) // 捕获帧头
//            {
//                uart_rx_buffer[0] = received_byte;
//                uart_rx_index = 1;
//            }
//        }
//        else
//        {
//            uart_rx_buffer[uart_rx_index] = received_byte;
//            uart_rx_index++;

//            if (uart_rx_index >= (uint8_t)sizeof(CommandFrame_t))
//            {
//                // 验证帧尾
//                if (uart_rx_buffer[sizeof(CommandFrame_t) - 1] == 0x55)
//                {
//                    // 计算 XOR 校验
//                    uint8_t checksum = 0;
//                    for (uint8_t i = 0; i < 6; i++)
//                    {
//                        checksum ^= uart_rx_buffer[i];
//                    }

//                    if (checksum == uart_rx_buffer[6]) // 校验通过
//                    {
//                        memcpy(&current_command, uart_rx_buffer, sizeof(CommandFrame_t));
//                        command_received = 1;
//                        last_command_time = HAL_GetTick();
//                        system_is_timeout = 0; // 喂狗，解除超时锁
//                    }
//                }
//                uart_rx_index = 0; // 重置状态机
//            }
//        }

//        // 重新开启中断接收
//        if (HAL_UART_Receive_IT(&huart1, &uart_single_byte_rx, 1) != HAL_OK)
//        {
//            huart1.RxState = HAL_UART_STATE_READY;
//            __HAL_UNLOCK(&huart1);
//            HAL_UART_Receive_IT(&huart1, &uart_single_byte_rx, 1);
//        }
//    }
//}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t cmd = uart_single_byte_rx;
        uint8_t ack_ok = 0x55;  
        uint8_t ack_err = 0xEE; 
        
        // ====== 1. 任何合法指令（动作、调速、心跳）进来，都执行”喂狗”刷新时间 ======
        // 去掉了 cmd >= 0x00 的无用警告，cmd <= 0x08 自动包含了 0x00
        if (cmd <= 0x0C || (cmd >= 0x10 && cmd <= 0x73))
        {
            // 👈 删掉了之前报错的内联 extern 声明，直接使用 usart.h 里的全局变量
            last_command_time = HAL_GetTick(); // 刷新滴答时间（喂狗）
            system_is_timeout = 0;             // 自动解除超时锁
        }

        // ====== 2. 数值偏移调速 (上位机发 16~115 / 0x10~0x73，对应速度 1~100) ======
        if (cmd >= 0x10 && cmd <= 0x73)
        {
            default_speed = cmd - 0x0F; // 自动还原为 1 ~ 100
            UART_SendByte_ISR(&huart1, ack_ok);             
            HAL_UART_Receive_IT(&huart1, &uart_single_byte_rx, 1);
            return;
        }

        // ====== 3. 核心指令集解析 switch ======
        switch (cmd)
        {
            /* ==================== 心跳包 (0x00) ==================== */
            case 0x00: 
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;

            /* ==================== AB 电机控制 (0x01 ~ 0x04) ==================== */
            case 0x01: // AB 电机正转
                motor_ab_state = 1; // 只改标志，让 main.c 的 while(1) 去疯狂驱动
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;
                
            case 0x02: // AB 电机反转
                motor_ab_state = 2; 
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;
                
            case 0x03: // AB 电机自由停止（滑行）
                motor_ab_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 0); 
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;
                
            case 0x04: // AB 电机刹车抱死
                motor_ab_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1); 
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;

            /* ==================== CD 电机控制 (0x05 ~ 0x08) ==================== */
            case 0x05: // CD 电机正转
                motor_cd_state = 1;
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;
                
            case 0x06: // CD 电机反转
                motor_cd_state = 2;
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;
                
            case 0x07: // CD 电机自由停止（滑行）
                motor_cd_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 0); 
                UART_SendByte_ISR(&huart1, ack_ok); 
                break;
                
            case 0x08: // CD 电机刹车抱死
                motor_cd_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);
                UART_SendByte_ISR(&huart1, ack_ok);
                break;

            /* ==================== 七彩LED (0x09 ~ 0x0A) ==================== */
            case 0x09: // 七彩LED 开 (PB1 高电平 → 模块自动闪烁)
                HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_SET);
                UART_SendByte_ISR(&huart1, ack_ok);
                break;

            case 0x0A: // 七彩LED 关 (PB1 低电平)
                HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_RESET);
                UART_SendByte_ISR(&huart1, ack_ok);
                break;

            case 0x0B: // 蜂鸣器短鸣（主循环自动关断）
                HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
                buzzer_on = 1;
                buzzer_on_time = HAL_GetTick();
                UART_SendByte_ISR(&huart1, ack_ok);
                break;

            case 0x0C: // 急停：AB + CD 同时刹车
                motor_ab_state = 0;
                motor_cd_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1);
                MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);
                UART_SendByte_ISR(&huart1, ack_ok);
                break;

            /* ==================== 无法识别的无效命令 ==================== */
            default:
                UART_SendByte_ISR(&huart1, ack_err); 
                break;
        }

        // 重新开启中断接收，等待下一个控制单字节
        HAL_UART_Receive_IT(&huart1, &uart_single_byte_rx, 1);
    }
    else if (huart->Instance == USART3)
    {
        // LD3320 帧解析状态机: AA 55 [ID] 55 AA
        uint8_t byte_in = ld3320_rx_byte;

        switch (ld3320_frame_idx)
        {
            case 0:  // 等待帧头 AA
                if (byte_in == LD3320_HDR1)
                    ld3320_frame_idx = 1;
                break;
            case 1:  // 等待帧头 55
                if (byte_in == LD3320_HDR2)
                    ld3320_frame_idx = 2;
                else
                    ld3320_frame_idx = 0;
                break;
            case 2:  // 接收关键词 ID
                ld3320_frame_buf[2] = byte_in;
                ld3320_frame_idx = 3;
                break;
            case 3:  // 等待尾 55
                if (byte_in == LD3320_TAIL1)
                    ld3320_frame_idx = 4;
                else
                    ld3320_frame_idx = 0;
                break;
            case 4:  // 等待尾 AA → 帧完整
                if (byte_in == LD3320_TAIL2)
                {
                    ld3320_frame_ready = 1;
                }
                ld3320_frame_idx = 0;
                break;
            default:
                ld3320_frame_idx = 0;
                break;
        }

        // 帧接收完成 → 执行指令
        if (ld3320_frame_ready)
        {
            ld3320_frame_ready = 0;
            uint8_t keyword_id = ld3320_frame_buf[2];

            // 喂狗
            last_command_time = HAL_GetTick();
            system_is_timeout = 0;

            switch (keyword_id)
            {
                case VOICE_AB_FORWARD:
                    motor_ab_state = 1;
                    break;
                case VOICE_AB_REVERSE:
                    motor_ab_state = 2;
                    break;
                case VOICE_CD_FORWARD:
                    motor_cd_state = 1;
                    break;
                case VOICE_CD_REVERSE:
                    motor_cd_state = 2;
                    break;
                case VOICE_ALL_STOP:
                    motor_ab_state = 0;
                    motor_cd_state = 0;
                    MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1);
                    MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);
                    break;
                case VOICE_LED_ON:
                    HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_SET);
                    break;
                case VOICE_LED_OFF:
                    HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_RESET);
                    break;
                default:
                    break;
            }
        }

        // 重新开启中断接收
        HAL_UART_Receive_IT(&huart3, &ld3320_rx_byte, 1);
    }
}
/**
 * @brief  发送应答帧
 * @param  status: 0x01-成功, 0x02-错误指令, 0x04-参数错误
 */
void SendAck(uint8_t status)
{
    CommandFrame_t ack_frame;
    memset(&ack_frame, 0, sizeof(CommandFrame_t));
    
    ack_frame.start_byte = 0xAA;
    ack_frame.command_type = CMD_ACK;
    ack_frame.sub_command = status;
    
    uint8_t checksum = 0;
    uint8_t *p = (uint8_t*)&ack_frame;
    for (uint8_t i = 0; i < 6; i++) {
        checksum ^= p[i];
    }
    ack_frame.checksum = checksum;
    ack_frame.end_byte = 0x55;
    
    HAL_UART_Transmit(&huart1, (uint8_t*)&ack_frame, sizeof(CommandFrame_t), 100);
}

/**
 * @brief  核心业务逻辑处理
 */
void ProcessCommand(void)
{
    if (!command_received) return;

    MS41929_MotorChannel_e channel = (MS41929_MotorChannel_e)current_command.channel;
    
    // 基础防错：检查通道有效性
    if (channel != MS41929_CHANNEL_AB && channel != MS41929_CHANNEL_CD) {
        SendAck(0x04); // 参数错误
        command_received = 0;
        return;
    }

    switch (current_command.command_type)
    {
        case CMD_MOTOR_CONTROL: // 1. 电机动作+速度一体化控制
        {
            uint8_t speed = current_command.speed;
            if (speed < 1) speed = 1; // 确保有基础速度

            switch (current_command.sub_command)
            {
                case MOTOR_FORWARD:
                    MS41929_Stepper_Run(channel, 1, speed);
                    SendAck(0x01);
                    break;
                case MOTOR_REVERSE:
                    MS41929_Stepper_Run(channel, 0, speed);
                    SendAck(0x01);
                    break;
                case MOTOR_STOP:
                    MS41929_Stepper_Stop(channel, 0); // 滑行停止
                    SendAck(0x01);
                    break;
                case MOTOR_BRAKE:
                    MS41929_Stepper_Stop(channel, 1); // 刹车锁死
                    SendAck(0x01);
                    break;
                default:
                    SendAck(0x04); // 动作子指令错误
                    break;
            }
            break;
        }

        case CMD_SPEED_SET: // 2. 纯改变速度指令（保持当前方向运行）
        {
            uint8_t speed = current_command.speed;
            
            // 获取芯片当前在该通道的运行方向
            uint16_t status = MS41929_GetMotorStatus(channel);
            uint8_t bit_shift = (channel == MS41929_CHANNEL_AB) ? MS41929_CCWCWAB_BIT : MS41929_CCWCWCD_BIT;
            uint8_t current_dir = (status >> bit_shift) & 0x01;
            
            // 应用新速度
            MS41929_Stepper_Run(channel, current_dir, speed);
            SendAck(0x01);
            break;
        }

        default:
            SendAck(0x02); // 未知主指令
            break;
    }
    
    command_received = 0; // 标记处理完毕
}

/**
 * @brief  初始化串口接收中断
 */
void UART_Receive_Init(void)
{
    HAL_UART_Receive_IT(&huart1, &uart_single_byte_rx, 1);
}

void LD3320_Receive_Init(void)
{
    HAL_UART_Receive_IT(&huart3, &ld3320_rx_byte, 1);
}
/* USER CODE END 1 */
