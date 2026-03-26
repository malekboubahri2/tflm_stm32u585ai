

//STEP: 1
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
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
	static tflite::MicroMutableOpResolver<3> micro_op_resolver;
	TfLiteStatus status = micro_op_resolver.AddQuantize();
	status = micro_op_resolver.AddDequantize();
	status = micro_op_resolver.AddFullyConnected();
	if (status != kTfLiteOk)
	{
		MicroPrintf("Failed to add ops\r\n");
	}
	

	 /*6. Configure interpreter*/
	 static tflite::MicroInterpreter static_interpreter(model,micro_op_resolver,tensor_arena,kTensorArenaSize);
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

void benchmark_inference(uint32_t repeats = 1000) {
  if (interpreter == nullptr || input == nullptr) {
    MicroPrintf("Benchmark aborted: interpreter not configured\n");
    return;
  }

  // Warm up the kernel to stabilize caches and branch predictions.
  for (uint32_t i = 0; i < 16; ++i) {
    float position = static_cast<float>(i) / static_cast<float>(kInferencesPerCycle);
    float x = position * kXrange;
    int8_t x_quantized = x / input->params.scale + input->params.zero_point;
    input->data.int8[0] = x_quantized;
    interpreter->Invoke();
  }

  uint32_t start_cycles = dwt_get_cycles();
  for (uint32_t i = 0; i < repeats; ++i) {
    float position = static_cast<float>(i) / static_cast<float>(kInferencesPerCycle);
    float x = position * kXrange;
    int8_t x_quantized = x / input->params.scale + input->params.zero_point;
    input->data.int8[0] = x_quantized;
    interpreter->Invoke();
  }
  uint32_t total_cycles = dwt_get_cycles() - start_cycles;

  float avg_cycles = (float)total_cycles / (float)repeats;
  MicroPrintf("Benchmark: %u runs, total %u cycles, avg %.1f cycles/run\n",
              repeats, total_cycles, avg_cycles);
}

void run_inference() {

    float position = static_cast<float>(inference_count) /
                     static_cast<float>(kInferencesPerCycle);
    float x = position * kXrange;

    int8_t x_quantized = x / input->params.scale + input->params.zero_point;
    input->data.int8[0] = x_quantized;

    /* --- start cycle count --- */
    uint32_t cycles_start = dwt_get_cycles();

    TfLiteStatus status = interpreter->Invoke();

    uint32_t cycles_elapsed = dwt_get_cycles() - cycles_start;
    /* --- end cycle count --- */

    if (status != kTfLiteOk) {
        MicroPrintf("Unable to invoke interpreter\r\n");
    }

    int8_t y_quantized = output->data.int8[0];
    float y = (y_quantized - output->params.zero_point) * output->params.scale;

    //handle_output(x, y);

    /* log cycles every inference */
    /* convert floats to integers for printing */
	float elapsed_ms = (float)cycles_elapsed / (float)SystemCoreClock * 1000.0f;
	int ms_int  = (int)elapsed_ms;
	int ms_frac = (int)((elapsed_ms - ms_int) * 1000);

	MicroPrintf("x: %f  y: %f  time: %d.%d ms\r\n",
        x, y, ms_int, ms_frac);

    inference_count += 1;
    if (inference_count >= kInferencesPerCycle) {
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


