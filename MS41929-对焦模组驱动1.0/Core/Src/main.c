/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ms41929.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// COMMAND_TIMEOUT 已在 usart.h 中定义为 1000ms
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

//* USER CODE BEGIN PV */
// 电机控制相关配置（保留初始化所需的通道和细分默认值）
MS41929_MotorChannel_e motor_channel = MS41929_CHANNEL_AB;  // 默认电机通道
MS41929_SubdivisionMode_e subdiv_mode = MS41929_SUBDIV_64;  // 默认细分模式：64细分

// 显式声明外部串口句柄，防止 fputc 报 undefined 错误
extern UART_HandleTypeDef huart1; 
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE BEGIN 0 */
/**
  * @brief  重定向c库函数printf到USART
  * @param  ch: 要输出的字符
  * @param  f: 文件指针
  * @retval 输出的字符
  */
int fputc(int ch, FILE *f)
{
    uint8_t data = (uint8_t)ch;
    
    // 这里的 huart1 已在 PV 区通过 extern 成功引用
    if (HAL_UART_Transmit(&huart1, &data, 1, 100) != HAL_OK)
    {
        return -1;
    }
    
    return ch;
}

// 串口精简业务逻辑相关的外部函数与变量声明
extern void ProcessCommand(void);
extern void UART_Receive_Init(void);
extern void LD3320_Receive_Init(void);
void MX_USART3_UART_Init(void);
volatile uint32_t last_command_time = 0;
volatile uint8_t system_is_timeout = 0;
volatile uint8_t motor_ab_state = 0;
volatile uint8_t motor_cd_state = 0;

/* 声明之前在中断里用的速度变量，并让它变成全局可用 */
volatile uint8_t default_speed = 1;
extern volatile uint8_t buzzer_on;
extern volatile uint32_t buzzer_on_time;
/* USER CODE END 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("STM32 Init.....");
  HAL_Delay(10);

  // 初始化步进电机
  MS41929_Stepper_Init(subdiv_mode);
  MS41929_SPI_Test();
  // 初始化串口
  UART_Receive_Init();
  // 初始化 LD3320 语音模块 (USART3, 9600bps)
  MX_USART3_UART_Init();
  LD3320_Receive_Init();

  printf("STM32 Init完成");
  
  MS41929_Stepper_Enable(MS41929_CHANNEL_AB, 1); 
MS41929_Stepper_Enable(MS41929_CHANNEL_CD, 1);
HAL_Delay(10); // 给硬件一点稳定时间
  /* USER CODE END 2 */

 /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // ====== 1. 检查指令超时（看门狗） ======
      if (HAL_GetTick() - last_command_time > COMMAND_TIMEOUT)
      {
          if (system_is_timeout == 0)
          {
              system_is_timeout = 1; // 标记进入安全超时状态
              
              // 强行把全局状态清零
              motor_ab_state = 0;
              motor_cd_state = 0;
              
              // 紧急抱死电机
              MS41929_Stepper_Stop(MS41929_CHANNEL_AB, 1);  
              MS41929_Stepper_Stop(MS41929_CHANNEL_CD, 1);  
              
              // 可以选择通过串口发一个 0x99 报警帧告知上位机
              uint8_t timeout_ack = 0x99;
              HAL_UART_Transmit(&huart1, &timeout_ack, 1, 10);
          }
      }

      // ====== 1.5 蜂鸣器自动关断（80ms） ======
      if (buzzer_on && (HAL_GetTick() - buzzer_on_time > 80))
      {
          HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
          buzzer_on = 0;
      }

      // ====== 2. 如果系统正常（未超时），根据状态持续驱动电机 ======
      if (system_is_timeout == 0)
      {
          /* ---------- 持续驱动 AB 电机 ---------- */
          if (motor_ab_state == 1) 
          {
              MS41929_Stepper_Run(MS41929_CHANNEL_AB, 1, default_speed); // 持续正转
          }
          else if (motor_ab_state == 2) 
          {
              MS41929_Stepper_Run(MS41929_CHANNEL_AB, 0, default_speed); // 持续反转
          }

          /* ---------- 持续驱动 CD 电机 ---------- */
          if (motor_cd_state == 1) 
          {
              MS41929_Stepper_Run(MS41929_CHANNEL_CD, 1, default_speed); // 持续正转
          }
          else if (motor_cd_state == 2) 
          {
              MS41929_Stepper_Run(MS41929_CHANNEL_CD, 0, default_speed); // 持续反转
          }
      }
      
      /* USER CODE END WHILE */
      /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * @brief  测试MS41929 SPI通信
 * @param  无
 * @retval 无
 */
void MS41929_SPI_Test(void)
{
    uint16_t write_value, read_value;
    uint8_t test_reg = 0x24;  // 使用TESTEN1寄存器进行测试
    MS41929_TriggerVD_FZ();
    printf("\r\n=== MS41929 SPI test start ===\r\n");
    
    // Test 1: Write 0x0000 and read back
    write_value = 0x0000;
    MS41929_WriteReg(test_reg, write_value);
    HAL_Delay(10);  // Wait for write to complete
    read_value = MS41929_ReadReg(test_reg);
    printf("测试1: 写入0x%04X, 读取0x%04X %s\r\n", 
           write_value, read_value, (read_value == write_value) ? "成功" : "失败");
    // Test 2: Write 0x1234 and read back
    write_value = 0x1034;
    MS41929_WriteReg(test_reg, write_value);
    HAL_Delay(10);  // Wait for write to complete
    read_value = MS41929_ReadReg(test_reg);
	printf("测试2: 写入0x%04X, 读取0x%04X %s\r\n", 
           write_value, read_value, (read_value == write_value) ? "成功" : "失败");
    
    // Test 3: Write 0xABCD and read back
    write_value = 0xA8CD;
    MS41929_WriteReg(test_reg, write_value);
    HAL_Delay(10);  // Wait for write to complete
    read_value = MS41929_ReadReg(test_reg);
    printf("Test 3: Write 0x%04X, Read 0x%04X %s\r\n", 
           write_value, read_value, (read_value == write_value) ? "Success" : "Fail");
    
    // Test 4: Write 0xFFFF and read back
    write_value = 0xFCFF;
    MS41929_WriteReg(test_reg, write_value);
    HAL_Delay(10);  // Wait for write to complete
    read_value = MS41929_ReadReg(test_reg);
    printf("Test 4: Write 0x%04X, Read 0x%04X %s\r\n", 
           write_value, read_value, (read_value == write_value) ? "Success" : "Fail");
    
    // Restore register default value
    MS41929_WriteReg(test_reg, 0x0000);
    
    printf("=== MS41929 SPI test end ===\r\n\r\n");
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
