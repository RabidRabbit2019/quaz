#include "buttons.h"
#include "stm32g431xx.h"


#define BT_COUNT     5

// после изменения состояния кнопки другие изменения состояния игнорируются на это время, мс
#define BT_IGNORE_INTERVAL_MS   250u
// интервал запуска автоповтора нажатия при длительном нажатии на кнопку, мс
#define BT_AUTOREPEAT_WAIT      1000u
// интервал между "автоматическими" нажатиями на кнопку
#define BT_AUTOREPEAT_INTERVAL  100u


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
// временные метки от нажатия на кнопку для перехода в состояние повторного нажатия
static uint32_t g_repeat_wait[BT_COUNT];
// флаги автоматического генерирования повторного нажатия
static uint32_t g_repeat_flag = 0;

extern volatile uint32_t g_milliseconds;
void delay_ms( uint32_t a_ms );


// получить маску кнопок с изменившимся состоянием (1 - состояние изменилось)
uint32_t get_changed_buttons() {
  return g_buttons_change;
}


// получить маску состояния кнопок (0 - нажата)
uint32_t get_buttons_state() {
  return g_buttons;
}


// начальные настройки
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


// сканирование состояния кнопок
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
  // обработка всех кнопок
  for ( int i = 0; i < BT_COUNT; ++i ) {
    if ( 0 != (v_ignore_mask & g_buttons_masks[i]) ) {
      // заигноренные кнопки пропускаем
      continue;
    }
    if ( 0 != (g_buttons_change & g_buttons_masks[i]) ) {
      // кнопка изменила состояние, зарядим интервал игнора
      g_buttons_wait_ms[i] = v_now;
      if ( 0 != (g_buttons & g_buttons_masks[i]) ) {
        // если кнопка была отпущена, снимаем флаг автоповтора
        g_repeat_flag &= ~g_buttons_masks[i];
      } else {
        // если кнопка была нажата, зарядим ожидание генерации автоповтора
        g_repeat_wait[i] = v_now;
      }
    } else {
      // кнопка не меняла состояние, поддерживаем интервал игнора, чтоб в следующий раз не игнорировать
      g_buttons_wait_ms[i] = v_now - BT_IGNORE_INTERVAL_MS;
      // состояние кнопки не изменилось, если прошло достаточно времени, запустим автоповтор
      if ( 0 == (g_buttons & g_buttons_masks[i]) ) {
        // кнопка нажата, проверим автоповтор
        if ( 0 != (g_repeat_flag & g_buttons_masks[i]) ) {
          // у кнопки включен автоповтор, проверим время последнего изменения состояния
          if ( ((uint32_t)(v_now - g_repeat_wait[i])) >= BT_AUTOREPEAT_INTERVAL ) {
            // генерируем автонажатие
            g_buttons_change |= g_buttons_masks[i];
            // задержка автоповтора
            g_repeat_wait[i] = v_now;
          }
        } else {
          // автоповтор выключен, проверим интервал запуска
          if ( ((uint32_t)(v_now - g_repeat_wait[i])) >= BT_AUTOREPEAT_WAIT ) {
            // можно запускать автоповтор
            g_repeat_flag |= g_buttons_masks[i];
            // генерируем автонажатие
            g_buttons_change |= g_buttons_masks[i];
            // задержка автоповтора
            g_repeat_wait[i] = v_now;
          }
        }
      }
    }
  }
}


bool is_repeated( unsigned int a_button_mask ) {
  return 0 != (g_repeat_flag & a_button_mask);
}
