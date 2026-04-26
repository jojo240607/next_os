#ifndef LOG_CONFIG_H
#define LOG_CONFIG_H

#define LOG_RING_BUF_SIZE     64        // 环形缓冲区容量，必须是2的幂
#define LOG_MSG_MAX_LEN       80        // 单条日志文本最大长度
#define LOG_TASK_STACK_SIZE   512       // 日志任务栈大小（单位：字）
#define LOG_TASK_PRIORITY     5         // 日志任务优先级，通常较低

#endif