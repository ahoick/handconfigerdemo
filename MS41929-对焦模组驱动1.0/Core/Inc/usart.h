#include "main.h"
#include <stdint.h>  // 显式引入标准整数类型，防止 uint8_t 报错
#include "stm32f1xx_hal.h"
/* USER CODE BEGIN Private defines */
#define UART_BUFFER_SIZE 128
#define COMMAND_TIMEOUT 1000 // 1秒超时

// 精简后的指令类型
typedef enum {
    CMD_MOTOR_CONTROL = 0x01, // 电机动作控制
    CMD_SPEED_SET     = 0x02, // 电机速度设置
    CMD_ACK           = 0xFF  // 确认响应
} CommandType_e;

// 电机动作子指令
typedef enum {
    MOTOR_FORWARD = 0x01,  // 正转
    MOTOR_REVERSE = 0x02,  // 反转
    MOTOR_STOP    = 0x03,  // 停止（自由滑行）
    MOTOR_BRAKE   = 0x04   // 刹车（锁死）
} MotorControlSubCmd_e;

// 统一的 8 字节指令结构体
typedef struct {
    uint8_t start_byte;     // 固定 0xAA
    uint8_t command_type;   // 指令类型 (0x01 或 0x02)
    uint8_t sub_command;    // 子指令 (动作类型)
    uint8_t channel;        // 电机通道 (0: AB, 1: CD)
    uint8_t speed;          // 速度值 (1~255)
    uint8_t reserved;       // 保留字节 (固定 0x00)
    uint8_t checksum;       // 校验和 (字节 0~5 的 XOR 结果)
    uint8_t end_byte;       // 固定 0x55
} CommandFrame_t;

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void SendAck(uint8_t status);
void ProcessCommand(void);
void UART_Receive_Init(void);
void MX_USART3_UART_Init(void);
void LD3320_Receive_Init(void);

// LD3320 语音关键词ID → 指令映射表
// 根据 LD3320 模块实际编程的关键词调整以下 ID
// 帧格式: AA 55 [ID] 55 AA
#define VOICE_AB_FORWARD    1   // "对焦正转" → AB正转
#define VOICE_AB_REVERSE    2   // "对焦反转" → AB反转
#define VOICE_CD_FORWARD    3   // "变焦正转" → CD正转
#define VOICE_CD_REVERSE    4   // "变焦反转" → CD反转
#define VOICE_ALL_STOP      5   // "停止" → AB+CD急停
#define VOICE_LED_ON        6   // "开灯" → 七彩LED亮
#define VOICE_LED_OFF       7   // "关灯" → 七彩LED灭
// ID 8: "播放音乐" — LD3320 本地播放，不通过串口输出

extern volatile uint32_t last_command_time;
extern volatile uint8_t system_is_timeout;
extern volatile uint8_t motor_ab_state;
extern volatile uint8_t motor_cd_state;
/* USER CODE END Prototypes */
