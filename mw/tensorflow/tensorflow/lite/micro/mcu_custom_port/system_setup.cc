/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/micro/system_setup.h"

// CMSIS device header for STM32U5 series registers
#include "stm32u5xx.h"
#include "core_cm33.h"  // CMSIS core definitions (SCB base registers)
#include "cachel1_armv7.h"  // helpers for enabling I/D cache

namespace tflite {

// To add an equivalent function for your own platform, create your own
// implementation file, and place it in a subfolder named after the target. See
// tensorflow/lite/micro/debug_log.cc for a similar example.
void InitializeTarget() {

    // Enable FPU
    SCB->CPACR |= (0xF << 20);
    __DSB();
    __ISB();

    // 1. Enable Power Interface clock
    RCC->AHB3ENR |= RCC_AHB3ENR_PWREN;

    // 2. Set voltage scaling to Range 1 (VOS = 0b11)
    PWR->VOSR = (PWR->VOSR & ~PWR_VOSR_VOS) | PWR_VOSR_VOS;
    // Wait for voltage ready
    while ((PWR->VOSR & PWR_VOSR_VOSRDY) == 0) {}

    // 3. Set regulator supply to SMPS
    PWR->CR3 |= PWR_CR3_REGSEL;

    // 4. Enable EPOD booster
   // PWR->VOSR |= PWR_VOSR_BOOSTEN;
    // Wait for booster ready
    // while ((PWR->VOSR & PWR_VOSR_BOOSTRDY) == 0) {}

    // 5. Enable Flash clock and configure flash latency
    RCC->AHB1ENR |= RCC_AHB1ENR_FLASHEN;
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_4WS;

    // 6. Enable MSI oscillator
    RCC->CR |= RCC_CR_MSISON;
    while ((RCC->CR & RCC_CR_MSISRDY) == 0) {}

    // 7. Configure MSI: range 4 (4 MHz)
    RCC->ICSCR1 = (RCC->ICSCR1 & ~RCC_ICSCR1_MSISRANGE) | (4U << RCC_ICSCR1_MSISRANGE_Pos);
    // 8. Enable manual range selection
    RCC->ICSCR1 |= RCC_ICSCR1_MSIRGSEL;
    // 9. Set MSI calibration trim for range 4 (value 0x10)
    RCC->ICSCR2 = (RCC->ICSCR2 & ~RCC_ICSCR2_MSITRIM1) | (0x10U << RCC_ICSCR2_MSITRIM1_Pos);

    // 10. Configure PLL1: source MSI, input range 4-8MHz, enable P/Q/R outputs
    RCC->PLL1CFGR = RCC_PLL1CFGR_PLL1SRC_0   // MSI as PLL1 source
                  | RCC_PLL1CFGR_PLL1RGE_1   // Input range 4-8 MHz
                  | RCC_PLL1CFGR_PLL1PEN     // Enable P output
                  | RCC_PLL1CFGR_PLL1QEN     // Enable Q output
                  | RCC_PLL1CFGR_PLL1REN;    // Enable R output

    // 11. Set PLL1 dividers: N=80, P=2, Q=2, R=2 (fields: N=80, P=1, Q=1, R=1)
    RCC->PLL1DIVR = (80U << RCC_PLL1DIVR_PLL1N_Pos)
                  | (1U << RCC_PLL1DIVR_PLL1P_Pos)
                  | (1U << RCC_PLL1DIVR_PLL1Q_Pos)
                  | (1U << RCC_PLL1DIVR_PLL1R_Pos);

    // 12. Enable PLL1
    RCC->CR |= RCC_CR_PLL1ON;
    // Wait for PLL1 lock
    while ((RCC->CR & RCC_CR_PLL1RDY) == 0) {}

    // 13. Set AHB and APB prescalers to 1 (no division)
    RCC->CFGR2 = 0;

    // 14. Switch system clock to PLL1
    RCC->CFGR1 = (RCC->CFGR1 & ~RCC_CFGR1_SW) | (RCC_CFGR1_SW_0 | RCC_CFGR1_SW_1);
    // Wait for switch to complete
    while ((RCC->CFGR1 & RCC_CFGR1_SWS) != (RCC_CFGR1_SWS_0 | RCC_CFGR1_SWS_1)) {}
    __ISB();

    // 15. Update SystemCoreClock
    SystemCoreClock = 160000000;

    // 16. Enable DCACHE1 clock
    RCC->AHB1ENR |= RCC_AHB1ENR_DCACHE1EN;
    // 17. Enable I-Cache controller
    ICACHE->CR |= ICACHE_CR_EN;
    // 18. Enable D-Cache controller
    DCACHE1->CR |= DCACHE_CR_EN;
    // 19. Enable I-Cache and D-Cache in the CPU core (also invalidates)
    SCB_EnableICache();
    SCB_EnableDCache();

}

}  // namespace tflite
