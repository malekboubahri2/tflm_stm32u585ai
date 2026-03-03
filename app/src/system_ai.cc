

//STEP: 1
#include "tensorflow/lite/micro/mcu_custom_port/debug_log_callback.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "stdio.h"
#include "system_ai.h"
#include "sine_model_data.h"
#include "output_handler.h"
#include "constants.h"
#include "math.h"


//STEP: 2


namespace{

	const tflite::Model * model =  nullptr;
	tflite::MicroInterpreter * interpreter = nullptr;
	TfLiteTensor * input =  nullptr;
	TfLiteTensor * output =  nullptr;
	int inference_count = 0;

	constexpr int kTensorArenaSize =  2 * 1024;  //In bytes

	alignas(16) uint8_t tensor_arena[kTensorArenaSize];

}

//STEP: 3

void debug_log_printf(const char* s)
{
	printf(s);
}



void configure_model(){
	 /*1. Register debug log callback*/
	 RegisterDebugLogCallback(debug_log_printf);

	MicroPrintf("Start Model!\r\n");

	 /*2. Initialize target*/
	 tflite::InitializeTarget();

	 /*3. Get model*/
	 model = tflite::GetModel(sine_model);

	 /*4. Check version */
	 if(model->version() !=  TFLITE_SCHEMA_VERSION)
	 {
		 MicroPrintf("Model version: %d and not equal schema version : %d\n\r",model->version(),TFLITE_SCHEMA_VERSION);
	   //Option2
	   // return;
	 }
	 /*5. Get operations needed*/
	 static tflite::AllOpsResolver resolver;

	 /*6. Configure interpreter*/
	 static tflite::MicroInterpreter static_interpreter(model,resolver,tensor_arena,kTensorArenaSize);
	 interpreter =  &static_interpreter;

	 /*7. Allocate memory from the tensor arena*/
	 TfLiteStatus allocate_status =  interpreter->AllocateTensors();
	 if(allocate_status != kTfLiteOk)
	 {
		 MicroPrintf("Unable to allocate tensors\r\n");
	 }

	 /*8.Get pointers to the  input and output tensors*/
	 input = interpreter->input(0);
	 output = interpreter->output(0);

}

float y;

void run_inference(){

	/*1.Compute x value*/
	float position  =  static_cast<float>(inference_count)/ static_cast<float>(kInferencesPerCycle);
	float x =  position * kXrange;

	/*2.Quantize x value*/
	int8_t x_quantized = x / input->params.scale  + input->params.zero_point;

	/*3.Place quantized x value in the input tensor*/
	input->data.int8[0] = x_quantized;

	/*4.Invoke the interpreter to run the inference*/
	TfLiteStatus  status =  interpreter->Invoke();

	if(status != kTfLiteOk)
	{
		 MicroPrintf("Unable to invoke interpreter\r\n");

	}
	/*5.Obtain quantized output*/
	int8_t y_quantized = output->data.int8[0];

	/*6.Dequantize the output*/
	 y =  (y_quantized - output->params.zero_point) * output->params.scale;

	/*9.Handle output*/
	 handle_output(x, y);

	/*10. Increment inference count and reset if it crosses a threshold*/
	 inference_count += 1;

	 if(inference_count >= kInferencesPerCycle)
	 {
		 inference_count = 0;
	 }

}


void run_sensor_inference(uint16_t sensor_value){

	/*1.ADC values range from 0 to 4095.
	 * Map(normalize) the adc value (from 0 4095)  to 0 to 2*pi*/

	float normalized_input =  (static_cast<float>(sensor_value)/ 4095.0f)*2.0f * M_PI;

	/*2.Quantize input value*/
	int8_t input_quantized = normalized_input / input->params.scale  + input->params.zero_point;

	/*3.Place quantized input value in the input tensor*/
	input->data.int8[0] = input_quantized;

	/*4.Invoke the interpreter to run the inference*/
	TfLiteStatus  status =  interpreter->Invoke();

	if(status != kTfLiteOk)
	{
		 MicroPrintf("Unable to invoke interpreter\r\n");

	}
	/*5.Obtain quantized output*/
	int8_t output_quantized = output->data.int8[0];

	/*6.Dequantize the output*/
	float output_float =  (output_quantized - output->params.zero_point) * output->params.scale;

	/*9.Handle output*/
	 handle_output(normalized_input, output_float);

	/*10. Increment inference count and reset if it crosses a threshold*/
	 inference_count += 1;

	 if(inference_count >= kInferencesPerCycle)
	 {
		 inference_count = 0;
	 }

}


