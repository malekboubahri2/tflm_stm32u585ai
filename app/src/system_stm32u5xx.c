/* Minimal system implementation for STM32U5 series */

#include "stm32u5xx.h"
#include <stdint.h>

uint32_t SystemCoreClock = 16000000; // Initial value after reset (HSI16)

void SystemInit(void)
{
    /* no initialization needed for minimal project */
}
