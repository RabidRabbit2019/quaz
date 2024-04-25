#ifndef __GEN_DDS_H__
#define __GEN_DDS_H__

#include <stdint.h>


#define MAX_SOUND_VOLUME    2047u
#define MIN_SOUND_VOLUME    10u


void gen_dds_init();
uint32_t get_tx_phase();
uint32_t get_tx_freq();
void set_sound_volume( uint32_t a_volume );

#endif // __GEN_DDS_H__
