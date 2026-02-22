#include <stdint.h>
#include <stdio.h>
#include "stm32u5xx.h" 

/* redirect printf to ITM via CMSIS helper */
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ITM_SendChar((uint32_t)*ptr++);
    }
    return len;
}

int main(void)
{
	char counter = 0;
	int delay;

	/* Loop forever */
	while(1)
	{
	     printf("counter = %d\n", counter++);
	     delay = 160000;
	     while(delay){ delay--; }
	}
}