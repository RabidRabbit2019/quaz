#ifndef __ADC_H__
#define __ADC_H__

#include <stdbool.h>
#include <stdint.h>

// #define ADC_SAMPLES_COUNT   128
#define ADC_SAMPLES_COUNT   256

void adc_init();
void adc_shutdown();

bool adc_buffer_flag();
bool adc_error_flag();
uint16_t * adc_get_buffer();
int adc_get_tx_phase();


#endif // __ADC_H__

