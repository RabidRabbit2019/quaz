#include "buttons.h"
#include "stm32f103x6.h"


#define BT_COUNT     5

#define BT_IGNORE_INTERVAL_MS   250u

static uint32_t g_buttons = 0;
static uint32_t g_buttons_change = 0;
static uint32_t g_buttons_wait_ms[BT_COUNT];
static const uint32_t g_buttons_masks[BT_COUNT] = {
  BT_OK_mask
, BT_UP_mask
, BT_DOWN_mask
, BT_INC_mask
, BT_DEC_mask
};

extern volatile uint32_t g_milliseconds;
void delay_ms( uint32_t a_ms );


uint32_t get_changed_buttons() {
  return g_buttons_change;
}


uint32_t get_buttons_state() {
  return g_buttons;
}


void buttons_init() {
  // PB12..PB15 входы с подтяжкой к питанию
  GPIOB->CRH = (GPIOC->CRH & ~( GPIO_CRH_MODE11 | GPIO_CRH_CNF11
                              | GPIO_CRH_MODE12 | GPIO_CRH_CNF12
                              | GPIO_CRH_MODE13 | GPIO_CRH_CNF13
                              | GPIO_CRH_MODE14 | GPIO_CRH_CNF14
                              | GPIO_CRH_MODE15 | GPIO_CRH_CNF15 ))
               | GPIO_CRH_CNF11_1
               | GPIO_CRH_CNF12_1
               | GPIO_CRH_CNF13_1
               | GPIO_CRH_CNF14_1
               | GPIO_CRH_CNF15_1
               ;
  GPIOB->ODR |= ( BT_OK_mask
                | BT_UP_mask
                | BT_DOWN_mask
                | BT_INC_mask
                | BT_DEC_mask
                );
  delay_ms( 5u );
  g_buttons = GPIOB->IDR;
  for ( int i = 0; i < BT_COUNT; ++i ) {
    g_buttons_wait_ms[i] = g_milliseconds;
  }
}


void buttons_scan() {
  uint32_t v_buttons = GPIOB->IDR;
  uint32_t v_now = g_milliseconds;
  uint32_t v_ignore_mask = 0;
  for ( int i = 0; i < BT_COUNT; ++i ) {
    // проверим интервал "игнора"
    if ( ((uint32_t)(v_now - g_buttons_wait_ms[i])) < BT_IGNORE_INTERVAL_MS ) {
      // пока что игнорируем изменения состояния кнопки
      v_ignore_mask |= g_buttons_masks[i];
    }
  }
  // обновим флаги изменения состояния
  g_buttons_change = g_buttons ^ v_buttons;
  // в битах изменения замаскируем "игнорируемые" кнопки
  g_buttons_change &= ~v_ignore_mask;
  // перепишем только состояние кнопок, которые не в игноре
  g_buttons = (g_buttons & v_ignore_mask) | (v_buttons & ~v_ignore_mask);
  // кнопки с изменившимся состоянием будем "игнорить"
  for ( int i = 0; i < BT_COUNT; ++i ) {
    if ( 0 != (g_buttons_change & g_buttons_masks[i]) ) {
      g_buttons_wait_ms[i] = v_now;
    } else {
      g_buttons_wait_ms[i] = v_now - BT_IGNORE_INTERVAL_MS;
    }
  }
}
