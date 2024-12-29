#ifndef __ADC_H__
#define __ADC_H__

#include <stdbool.h>
#include <stdint.h>

// #define ADC_SAMPLES_COUNT   128
#define ADC_SAMPLES_COUNT   256

#define ADC_IN_RX   3
#define ADC_IN_ACC  0
#define ADC_IN_TX   1

void adc_init();
void adc_shutdown();
void adc_startup( int );
void adc_select_channel( int );

bool adc_buffer_flag();
bool adc_error_flag();
uint16_t * adc_get_buffer();
int adc_get_tx_phase();


#endif // __ADC_H__

