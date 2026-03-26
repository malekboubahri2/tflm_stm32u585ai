#include <stdint.h>
#include <stdio.h>
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
	dwt_init();

	configure_model();

	benchmark_inference(1000);

	while (1) {
		run_inference();
	}
}