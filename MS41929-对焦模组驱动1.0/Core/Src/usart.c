/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
  else if(uartHandle->Instance==USART3)
  {
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
  if(uartHandle->Instance==USART1)
  {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  }
}

/* USER CODE BEGIN 1 */
#include "ms41929.h"
#include <string.h>

/* ─── 全局变量 ─── */
uint8_t uart_rx_buffer[sizeof(CommandFrame_t)];
uint8_t uart_rx_index = 0;
volatile uint8_t uart_single_byte_rx = 0;
uint8_t command_received = 0;
CommandFrame_t current_command;

/* ─── LD3320 语音模块 ─── */
#define LD3320_FRAME_LEN  5
#define LD3320_HDR1       0xAA
#define LD3320_HDR2       0x55
#define LD3320_TAIL1      0x55
#define LD3320_TAIL2      0xAA

volatile uint8_t ld3320_rx_byte = 0;
uint8_t ld3320_frame_buf[LD3320_FRAME_LEN];
uint8_t ld3320_frame_idx = 0;
uint8_t ld3320_frame_ready = 0;

/**
 * @brief  串口接收中断回调
 *   USART1: 单字节指令 (上位机)
 *   USART3: LD3320 帧 AA 55 [ID] 55 AA (语音模块)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t cmd = uart_single_byte_rx;
        uint8_t ack_ok = 0x55;
        uint8_t ack_err = 0xEE;

        /* 喂狗 */
        if (cmd <= 0x0C || (cmd >= 0x10 && cmd <= 0x73))
        {
            last_command_time = HAL_GetTick();
            system_is_timeout = 0;
        }

        /* 速度设置 */
        if (cmd >= 0x10 && cmd <= 0x73)
        {
            default_speed = cmd - 0x0F;
            HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
            HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_single_byte_rx, 1);
            return;
        }

        switch (cmd)
        {
            case 0x00: /* 心跳 */
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x01: motor_ab_state = 1; HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x02: motor_ab_state = 2; HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x03: motor_ab_state = 0; MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 0); HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x04: motor_ab_state = 0; MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1); HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x05: motor_cd_state = 1; HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x06: motor_cd_state = 2; HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x07: motor_cd_state = 0; MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 0); HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x08: motor_cd_state = 0; MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1); HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x09: HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_SET);  HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x0A: HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_RESET); HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            case 0x0B: HAL_UART_Transmit(&huart1, &ack_err, 1, 10); break;
            case 0x0C: motor_ab_state = 0; motor_cd_state = 0;
                       MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1);
                       MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);
                       HAL_UART_Transmit(&huart1, &ack_ok, 1, 10); break;
            default:   HAL_UART_Transmit(&huart1, &ack_err, 1, 10); break;
        }

        HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_single_byte_rx, 1);
    }
    else if (huart->Instance == USART3)
    {
        uint8_t byte_in = ld3320_rx_byte;

        /* 帧解析状态机: AA 55 [ID] 55 AA */
        switch (ld3320_frame_idx)
        {
            case 0: if (byte_in == LD3320_HDR1) ld3320_frame_idx = 1; break;
            case 1:
                if (byte_in == LD3320_HDR2) ld3320_frame_idx = 2;
                else ld3320_frame_idx = 0;
                break;
            case 2:
                ld3320_frame_buf[2] = byte_in;
                ld3320_frame_idx = 3;
                break;
            case 3:
                if (byte_in == LD3320_TAIL1) ld3320_frame_idx = 4;
                else ld3320_frame_idx = 0;
                break;
            case 4:
                if (byte_in == LD3320_TAIL2) ld3320_frame_ready = 1;
                ld3320_frame_idx = 0;
                break;
            default:
                ld3320_frame_idx = 0;
                break;
        }

        if (ld3320_frame_ready)
        {
            ld3320_frame_ready = 0;
            uint8_t keyword_id = ld3320_frame_buf[2];

            /* 喂狗 */
            last_command_time = HAL_GetTick();
            system_is_timeout = 0;

            switch (keyword_id)
            {
                case VOICE_AB_FORWARD:  motor_ab_state = 1;  break;  /* 1:对焦正转 */
                case VOICE_AB_REVERSE:  motor_ab_state = 2;  break;  /* 2:对焦反转 */
                case VOICE_CD_FORWARD:  motor_cd_state = 1;  break;  /* 3:变焦正转 */
                case VOICE_CD_REVERSE:  motor_cd_state = 2;  break;  /* 4:变焦反转 */
                case VOICE_ALL_STOP:
                    motor_ab_state = 0; motor_cd_state = 0;
                    MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1);
                    MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);
                    break;
                case VOICE_LED_ON:                                    /* 6:开灯 */
                    HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_SET);
                    break;
                case VOICE_LED_OFF:                                   /* 7:关灯 */
                    HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_RESET);
                    break;
                default: break;
            }
        }

        HAL_UART_Receive_IT(&huart3, (uint8_t *)&ld3320_rx_byte, 1);
    }
}

/**
 * @brief  UART 错误回调
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
        volatile uint8_t dummy = (uint8_t)(huart->Instance->DR);
        (void)dummy;
        __HAL_UART_CLEAR_OREFLAG(huart);
    }
    if (huart->ErrorCode & HAL_UART_ERROR_FE) __HAL_UART_CLEAR_FEFLAG(huart);
    if (huart->ErrorCode & HAL_UART_ERROR_NE) __HAL_UART_CLEAR_NEFLAG(huart);

    huart->RxState = HAL_UART_STATE_READY;
    if (huart->Instance == USART1)
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_single_byte_rx, 1);
    else if (huart->Instance == USART3)
        HAL_UART_Receive_IT(&huart3, (uint8_t *)&ld3320_rx_byte, 1);
}

/* ─── 以下为旧版帧协议兼容代码，保留但不使用 ─── */
void SendAck(uint8_t status)
{
    CommandFrame_t ack_frame;
    memset(&ack_frame, 0, sizeof(CommandFrame_t));
    ack_frame.start_byte = 0xAA;
    ack_frame.command_type = CMD_ACK;
    ack_frame.sub_command = status;
    uint8_t checksum = 0;
    uint8_t *p = (uint8_t*)&ack_frame;
    for (uint8_t i = 0; i < 6; i++) checksum ^= p[i];
    ack_frame.checksum = checksum;
    ack_frame.end_byte = 0x55;
    HAL_UART_Transmit(&huart1, (uint8_t*)&ack_frame, sizeof(CommandFrame_t), 100);
}

void ProcessCommand(void)
{
    if (!command_received) return;
    MS41929_MotorChannel_e channel = (MS41929_MotorChannel_e)current_command.channel;
    if (channel != MS41929_CHANNEL_AB && channel != MS41929_CHANNEL_CD) {
        SendAck(0x04); command_received = 0; return;
    }
    switch (current_command.command_type) {
        case CMD_MOTOR_CONTROL: {
            uint8_t speed = current_command.speed;
            if (speed < 1) speed = 1;
            switch (current_command.sub_command) {
                case MOTOR_FORWARD: MS41929_Stepper_Run(channel, 1, speed); SendAck(0x01); break;
                case MOTOR_REVERSE: MS41929_Stepper_Run(channel, 0, speed); SendAck(0x01); break;
                case MOTOR_STOP:    MS41929_Stepper_Stop(channel, 0);       SendAck(0x01); break;
                case MOTOR_BRAKE:   MS41929_Stepper_Stop(channel, 1);       SendAck(0x01); break;
                default: SendAck(0x04); break;
            }
            break;
        }
        case CMD_SPEED_SET: {
            uint8_t speed = current_command.speed;
            uint16_t status = MS41929_GetMotorStatus(channel);
            uint8_t bit_shift = (channel == MS41929_CHANNEL_AB) ? MS41929_CCWCWAB_BIT : MS41929_CCWCWCD_BIT;
            uint8_t current_dir = (status >> bit_shift) & 0x01;
            MS41929_Stepper_Run(channel, current_dir, speed);
            SendAck(0x01);
            break;
        }
        default: SendAck(0x02); break;
    }
    command_received = 0;
}

void UART_Receive_Init(void)
{
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_single_byte_rx, 1);
}

void LD3320_Receive_Init(void)
{
    HAL_UART_Receive_IT(&huart3, (uint8_t *)&ld3320_rx_byte, 1);
}
/* USER CODE END 1 */
