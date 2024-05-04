#ifndef __GEN_DDS_H__
#define __GEN_DDS_H__

#include <stdint.h>


#define BASE_PWM_FREQ       (72000000/512)

#define MAX_SOUND_VOLUME    2047u
#define MIN_SOUND_VOLUME    10u

#define MAX_TX_LEVEL        2047u
#define MIN_TX_LEVEL        10u


void gen_dds_init();
uint32_t get_tx_phase();
uint32_t get_tx_freq();
void set_sound_freq_by_angle( uint32_t a_angle );
void set_sound_volume( uint32_t a_volume );
uint32_t get_tx_level();
void set_tx_level( uint32_t a_level );

#endif // __GEN_DDS_H__
