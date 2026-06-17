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

/* ─── 扬声器变量 ─── */
static volatile uint8_t  speaker_active = 0;
static volatile uint16_t speaker_half_period = 0;   // 半周期(ms)
static volatile uint32_t speaker_stop_tick = 0;
static volatile uint32_t speaker_last_toggle = 0;

/**
 * @brief  扬声器发声（非阻塞，主循环驱动）
 * @param  freq_hz: 频率(Hz) 如 500=500Hz
 * @param  duration_ms: 持续时长(ms)
 */
void Speaker_Beep(uint16_t freq_hz, uint16_t duration_ms)
{
    if (freq_hz == 0 || duration_ms == 0) return;
    speaker_half_period = 500 / freq_hz;   // 半周期 ≈ (1000/freq)/2
    if (speaker_half_period < 1) speaker_half_period = 1;
    speaker_stop_tick = HAL_GetTick() + duration_ms;
    speaker_last_toggle = HAL_GetTick();
    speaker_active = 1;
    HAL_GPIO_WritePin(SPEAKER_GPIO_Port, SPEAKER_Pin, GPIO_PIN_SET);
}

/**
 * @brief  扬声器状态机（主循环每轮调用）
 */
void Speaker_Process(void)
{
    if (!speaker_active) return;
    uint32_t now = HAL_GetTick();
    if (now >= speaker_stop_tick) {
        HAL_GPIO_WritePin(SPEAKER_GPIO_Port, SPEAKER_Pin, GPIO_PIN_RESET);
        speaker_active = 0;
        return;
    }
    if ((now - speaker_last_toggle) >= speaker_half_period) {
        HAL_GPIO_TogglePin(SPEAKER_GPIO_Port, SPEAKER_Pin);
        speaker_last_toggle = now;
    }
}

/**
 * @brief  串口接收中断回调 — 单字节协议
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
            Speaker_Beep(800, 50);   /* 速度设置 → 短促高音 */
            HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
            HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_single_byte_rx, 1);
            return;
        }

        switch (cmd)
        {
            case 0x00: /* 心跳 */
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;

            case 0x01: /* AB 正转 */
                motor_ab_state = 1;
                Speaker_Beep(500, 150);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x02: /* AB 反转 */
                motor_ab_state = 2;
                Speaker_Beep(400, 150);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x03: /* AB 停止 */
                motor_ab_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 0);
                Speaker_Beep(300, 80);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x04: /* AB 刹车 */
                motor_ab_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1);
                Speaker_Beep(200, 100);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;

            case 0x05: /* CD 正转 */
                motor_cd_state = 1;
                Speaker_Beep(600, 150);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x06: /* CD 反转 */
                motor_cd_state = 2;
                Speaker_Beep(550, 150);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x07: /* CD 停止 */
                motor_cd_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 0);
                Speaker_Beep(350, 80);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x08: /* CD 刹车 */
                motor_cd_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);
                Speaker_Beep(250, 100);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;

            case 0x09: /* 七彩LED 开 */
                HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_SET);
                Speaker_Beep(1000, 60);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x0A: /* 七彩LED 关 */
                HAL_GPIO_WritePin(GPIOB, RGB_LED_Pin, GPIO_PIN_RESET);
                Speaker_Beep(900, 40);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;
            case 0x0B: /* 蜂鸣器(保留) */
                HAL_UART_Transmit(&huart1, &ack_err, 1, 10);
                break;
            case 0x0C: /* AB+CD 急停 */
                motor_ab_state = 0; motor_cd_state = 0;
                MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1);
                MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);
                Speaker_Beep(200, 300);
                HAL_UART_Transmit(&huart1, &ack_ok, 1, 10);
                break;

            default:
                HAL_UART_Transmit(&huart1, &ack_err, 1, 10);
                break;
        }

        HAL_UART_Receive_IT(&huart1, (uint8_t *)&uart_single_byte_rx, 1);
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
}

/* ─── 旧版帧协议兼容代码，保留但不使用 ─── */
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
/* USER CODE END 1 */
