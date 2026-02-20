/* Minimal stub of CMSIS header for STM32U5 series (U585AI) */

#ifndef STM32U5XX_H
#define STM32U5XX_H

#ifdef __cplusplus
extern "C" {
#endif

/* system initialization */
void SystemInit(void);

/* weak definitions for handlers for link */
void Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32U5XX_H */
