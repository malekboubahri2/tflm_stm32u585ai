#ifndef SYSTEM_AI_H_
#define SYSTEM_AI_H_


#ifdef __cplusplus

extern "C"{
#endif

void configure_model();

void run_inference();
void run_sensor_inference(uint16_t sensor_value);

#ifdef __cplusplus
}
#endif


#endif
