#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <stdbool.h>
#include <stdint.h>


#define BT_OK_mask   (1UL<<14)
#define BT_UP_mask   (1UL<<12)
#define BT_DOWN_mask (1UL<<11)
#define BT_INC_mask  (1UL<<15)
#define BT_DEC_mask  (1UL<<13)


void buttons_init();
void buttons_scan();
uint32_t get_changed_buttons();
uint32_t get_buttons_state();
// вернёт true, если для кнопки включен автоповтор
bool is_repeated( unsigned int a_button_mask );


#endif // __BUTTONS_H__
