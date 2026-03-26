#include "stm32u5xx.h" 

#ifndef SYSTEM_AI_H_
#define SYSTEM_AI_H_

#ifdef __cplusplus

extern "C"{
#endif

void configure_model();

void benchmark_inference(uint32_t repeats);

void run_inference();
void run_sensor_inference(uint16_t sensor_value);

static inline void dwt_init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* enable DWT */
    DWT->CYCCNT = 0;                                   /* reset counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;              /* enable cycle counter */
}

static inline uint32_t dwt_get_cycles(void) {
    return DWT->CYCCNT;
}

#ifdef __cplusplus
}
#endif


#endif
