#include <stdint.h>
#include <stdio.h>
#include "stm32u5xx.h" 
#include "system_ai.h"

extern "C"{
	/* redirect printf to ITM via CMSIS helper */
	int _write(int file, char *ptr, int len) {
		for (int i = 0; i < len; i++) {
			ITM_SendChar((uint32_t)*ptr++);
		}
		return len;
}
}

int main(void)
{
	configure_model();

	while(1)
	{
		run_inference();
	}
}