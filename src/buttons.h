#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <stdint.h>


#define BT_OK_mask   (1UL<<12)
#define BT_ESC_mask  (1UL<<13)
#define BT_INC_mask  (1UL<<14)
#define BT_DEC_mask  (1UL<<15)


void buttons_init();
void buttons_scan();
uint32_t get_changed_buttons();
uint32_t get_buttons_state();


#endif // __BUTTONS_H__
