//
// Created by zhiwei.gong on 2026/5/18.
//

#include "error.h"
#include "cmsis_gcc.h"

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    DISABLE_IRQ;
    /* 短暂阻塞让调试器有机会捕获现场，然后执行系统复位 */
    for (volatile uint32_t i = 0; i < 1000000; i++) { __NOP(); }
    NVIC_SystemReset();
    /* 如果复位失败则死循环 */
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}