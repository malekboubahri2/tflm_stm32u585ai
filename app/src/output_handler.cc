#include "output_handler.h"
#include "tensorflow/lite/micro/micro_log.h"


void handle_output(float x_value, float y_value)
{
	/*Print x and y value*/
	MicroPrintf("x_value:  %f, y_value: %f\n\r ",x_value,y_value);
}
