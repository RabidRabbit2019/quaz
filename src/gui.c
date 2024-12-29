#include "adc.h"
#include "calc.h"
#include "gen_dds.h"
#include "display.h"
#include "settings.h"
#include "buttons.h"
#include "fonts/font_25_30.h"
#include "stm32f103x6.h"

#include <stdio.h>


#define VDI_LINES_HEIGHT    90
#define VDI_SECTOR_DEGREES  8
#define VDI_LINES_WIDTH     (DISPLAY_WIDTH*VDI_SECTOR_DEGREES/360)
#define VDI_FFT_COLOR       0x07

#define GUI_MODE_MAIN       0
#define GUI_MODE_SETTINGS   1

#define MAIN_ITEM_MIN       0
#define MAIN_ITEM_BARRIER   0
#define MAIN_ITEM_VOLUME    1
#define MAIN_ITEM_FERRITE   2
#define MAIN_ITEM_GROUND    3
#define MAIN_ITEM_MAX       3

#define MAX_BARRIER_VALUE   999u
#define MIN_BARRIER_VALUE   1u

#define MAX_VOLTMETER_K_VALUE   131072u
#define MIN_VOLTMETER_K_VALUE   1u


static int g_gui_mode = GUI_MODE_MAIN;

static int g_x[ADC_SAMPLES_COUNT], g_y[ADC_SAMPLES_COUNT];
static char g_str[256];
static int g_last_column_fft = 0;

static int g_main_item = 0;

static int g_menu_level = 0; // 0 - корневое меню
static int g_menu_item = 0; // 0 - вернуться обратно

static int g_tmp = 0;
static settings_t * g_profiles_ptr[PROFILES_COUNT]; // адреса сохранённых профилей, NULL - профиль отсутствует

static void mi_tx_gen();
static void mi_rx_balance();
static void mi_mask();
static void mi_ferrite();
static void mi_power();
static void mi_save_profile();
static void mi_load_profile();

static void mh_tx_gen();
static void mh_rx_balance();
static void mh_mask();
static void mh_ferrite();
static void mh_power();
static void mh_save_profile();
static void mh_load_profile();

typedef void (*menu_item_init_t)(void);
typedef void (*menu_item_handler_t)(void);

typedef struct {
  const char * name;
  menu_item_init_t init;
  menu_item_handler_t handler;
} menu_item_t;

static const menu_item_t g_top_menu[] = {
  {"..", NULL, NULL}
, {"Раскачка TX", mi_tx_gen, mh_tx_gen}
, {"Баланс RX", mi_rx_balance, mh_rx_balance}
, {"Маска", mi_mask, mh_mask}
, {"Калибровка", mi_ferrite, mh_ferrite}
, {"Питание", mi_power, mh_power}
, {"Сохранить", mi_save_profile, mh_save_profile}
, {"Загрузить", mi_load_profile, mh_load_profile}
};

#define MENU_ITEM_BACK        0
#define MENU_ITEM_TX_POWER    1
#define MENU_ITEM_RX_BALANCE  2
#define MENU_ITEM_MASK        3
#define MENU_ITEM_FERRITE     4
#define MENU_ITEM_BATTERY     5
#define MENU_ITEM_SAVE        6
#define MENU_ITEM_LOAD        7


#define MENU_ITEM_MAX ((int)((sizeof(g_top_menu)/sizeof(g_top_menu[0]))-1))


void delay_ms( uint32_t a_ms );


static void gui_items() {
  settings_t * v_settings = settings_get_current_profile();
  // здесь отрисовка статических элементов экрана
  // строка "Порог"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT + font_25_30_font.m_row_height
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Порог"
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , 0 == g_main_item ? DISPLAY_COLOR_BLUE : DISPLAY_COLOR_DARKBLUE
      );
  sprintf( g_str, "%u", (unsigned int)v_settings->barrier_level );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, VDI_LINES_HEIGHT + font_25_30_font.m_row_height
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
  // строка "Громкость"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT + (font_25_30_font.m_row_height * 2)
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Громкость"
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , 1 == g_main_item ? DISPLAY_COLOR_BLUE : DISPLAY_COLOR_DARKBLUE
      );
  sprintf( g_str, "%u", (unsigned int)v_settings->level_sound );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, VDI_LINES_HEIGHT + (font_25_30_font.m_row_height * 2)
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
  // строка "Феррит"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT + (font_25_30_font.m_row_height * 3)
      , DISPLAY_WIDTH, font_25_30_font.m_row_height
      , "Феррит"
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , 2 == g_main_item ? DISPLAY_COLOR_MIDBLUE : DISPLAY_COLOR_DARKBLUE
      );
  // строка "Грунт"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT + (font_25_30_font.m_row_height * 4)
      , DISPLAY_WIDTH, font_25_30_font.m_row_height
      , "Грунт"
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , 3 == g_main_item ? DISPLAY_COLOR_MIDBLUE : DISPLAY_COLOR_DARKBLUE
      );
}


static void settings_items() {
  for ( int i = 0; i < (int)(sizeof(g_top_menu)/sizeof(g_top_menu[0])); ++i ) {
    display_write_string_with_bg(
        0
      , i * font_25_30_font.m_row_height
      , DISPLAY_WIDTH
      , font_25_30_font.m_row_height
      , g_top_menu[i].name
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , g_menu_item == i ? DISPLAY_COLOR_MIDBLUE : DISPLAY_COLOR_DARKGRAY
      );
  }
}


void gui_init() {
  // чистим экран
  display_fill_rectangle_dma_fast( 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_BYTE_COLOR_BLACK );
  // здесь отрисовка статических элементов экрана
  gui_items();
}

// индекс в результатах ФФТ с частотой, наиболее близкой к частоте TX
static int get_fft_idx() {
  return (int)(((((uint64_t)ADC_SAMPLES_COUNT/2) * get_tx_freq()) + 0x80000000ul) >> 32);
}


static void gui_main() {
  settings_t * v_settings = settings_get_current_profile();
  // индекс
  int fft_idx = get_fft_idx();
  int v_column = 0;
  // ждём заполнения буфера
  bool v_last_flag = adc_buffer_flag();
  while ( adc_buffer_flag() == v_last_flag ) {}
  //uint32_t v_start_ts = g_milliseconds;
  // быстренько получим адрес буфера и фазу генератора на момент начала буфера
  uint16_t * v_from = adc_get_buffer();
  uint32_t v_tx_phase = adc_get_tx_phase();
  // копируем выборки в буфер
  for ( int i = 0; i < ADC_SAMPLES_COUNT; ++i ) {
    g_x[i] = ((int)v_from[i]) << 16;
    g_y[i] = 0;
  }
  // вычисляем фазу в градусах
  int v_tx_phase_deg = (int)(((360ull << 16) * v_tx_phase) / 0x100000000ull);
  // БПФ по данным от АЦП
  BPF( g_x, g_y );
  printf( "---- %3d.%03d ----\n", v_tx_phase_deg / 65536, ((v_tx_phase_deg & 0xFFFF) * 1000) / 65536 );
  printf( "---- %u, %X ----\n", (unsigned int)v_tx_phase, (unsigned int)v_from );
  int v_len = g_x[fft_idx];
  int v_d = full_atn( &v_len, g_y[fft_idx] ) - v_tx_phase_deg;
  if ( v_d < 0 ) {
    v_d += 360 * 65536;
  }
  v_len /= 65536;
  // показометр "VDI"
  // поставим частоту звука в зависимости от номера колонки
  set_sound_freq_by_angle( (uint32_t)(v_d / 65536) );
  // вычислим номер колонки в зависимости от угла
  v_column = (v_d / VDI_SECTOR_DEGREES) / 65536;
  // вычисли высоту колонки в зависимости от "силы отклика"
  int v_height = (log2_32( (uint32_t)v_len ) * 15) / 2;
  if ( v_height > VDI_LINES_HEIGHT ) {
    v_height = VDI_LINES_HEIGHT;
  }
  // предыдущий столбик стираем
  display_fill_rectangle_dma_fast(
      g_last_column_fft * VDI_LINES_WIDTH, 0
    , VDI_LINES_WIDTH, VDI_LINES_HEIGHT
    , 0 );
  // новый рисуем
  display_fill_rectangle_dma_fast(
      v_column * VDI_LINES_WIDTH, VDI_LINES_HEIGHT - v_height
    , VDI_LINES_WIDTH, v_height
    , VDI_FFT_COLOR );
  g_last_column_fft = v_column;
  // значение "сила отклика"
  sprintf( g_str, "%d", v_len );
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT
      , DISPLAY_WIDTH/2, font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_GREEN
      , DISPLAY_COLOR_DARKGREEN
      );
  // числовое значение угла
  sprintf( g_str, "%d", v_d / 65536 );
  display_write_string_with_bg(
        DISPLAY_WIDTH/2, VDI_LINES_HEIGHT
      , DISPLAY_WIDTH/2, font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_RED
      , DISPLAY_COLOR_DARKRED
      );
  printf(
      "[%2d] x=%4d|y=%4d|r=%4d|d=%3d.%03d\n"
    , fft_idx
    , g_x[fft_idx] / 65536
    , g_y[fft_idx] / 65536
    , v_len
    , v_d / 65536, ((v_d & 0xFFFF) * 1000) / 65536
    );
  /*
  uint32_t v_time = g_milliseconds - v_start_ts;
  sprintf( g_str, "%u", (unsigned int)v_time );
  display_write_string_with_bg( 0, 64, 64, 30, g_str, &font_25_30_font, DISPLAY_COLOR_YELLOW, DISPLAY_COLOR_BLACK );
  */
  // кнопки
  uint32_t v_changed = get_changed_buttons();
  uint32_t v_buttons = get_buttons_state();
  if ( 0 != v_changed ) {
    if ( 0 != (v_changed & BT_UP_mask) && 0 == (v_buttons & BT_UP_mask) ) {
      // нажата кнопка "вверх"
      if ( --g_main_item < MAIN_ITEM_MIN ) {
        g_main_item = MAIN_ITEM_MAX;
      }
      gui_items();
    }
    if ( 0 != (v_changed & BT_DOWN_mask) && 0 == (v_buttons & BT_DOWN_mask) ) {
      // нажата кнопка "вниз"
      if ( ++g_main_item > MAIN_ITEM_MAX ) {
        g_main_item = MAIN_ITEM_MIN;
      }
      gui_items();
    }
    if ( 0 != (v_changed & BT_INC_mask) && 0 == (v_buttons & BT_INC_mask) ) {
      // нажата кнопка +
      switch ( g_main_item ) {
        case MAIN_ITEM_BARRIER:
          if ( ++v_settings->barrier_level > MAX_BARRIER_VALUE ) {
            v_settings->barrier_level = MAX_BARRIER_VALUE;
          } else {
            gui_items();
          }
          break;
          
        case MAIN_ITEM_VOLUME:
          if ( ++v_settings->level_sound > MAX_SOUND_VOLUME ) {
            v_settings->level_sound = MAX_SOUND_VOLUME;
          } else {
            set_sound_volume( v_settings->level_sound );
            gui_items();
          }
          break;
          
        case MAIN_ITEM_FERRITE:
          break;
          
        case MAIN_ITEM_GROUND:
          break;
          
        default:
          break;
      }
    }
    if ( 0 != (v_changed & BT_DEC_mask) && 0 == (v_buttons & BT_DEC_mask) ) {
      // нажата кнопка -
      switch ( g_main_item ) {
        case MAIN_ITEM_BARRIER:
          if ( --v_settings->barrier_level < MIN_BARRIER_VALUE ) {
            v_settings->barrier_level = MIN_BARRIER_VALUE;
          } else {
            gui_items();
          }
          break;
          
        case MAIN_ITEM_VOLUME:
          if ( --v_settings->level_sound < MIN_SOUND_VOLUME ) {
            v_settings->level_sound = MIN_SOUND_VOLUME;
          } else {
            set_sound_volume( v_settings->level_sound );
            gui_items();
          }
          break;
          
        case MAIN_ITEM_FERRITE:
          break;
          
        case MAIN_ITEM_GROUND:
          break;
          
        default:
          break;
      }
    }
    if ( 0 != (v_changed & BT_OK_mask) && 0 == (v_buttons & BT_OK_mask) ) {
      // нажата кнопка "ОК/Меню"
      // переключаемся в экран настроек
      display_fill_rectangle_dma_fast( 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_BYTE_COLOR_BLACK );
      settings_items();
      g_gui_mode = GUI_MODE_SETTINGS;
      set_sound_volume( 0 );
    }
  }
}


static void gui_settings() {
  //
  if ( g_menu_level > MENU_ITEM_BACK ) {
    g_top_menu[g_menu_level].handler();
  } else {
    // кнопки
    uint32_t v_changed = get_changed_buttons();
    uint32_t v_buttons = get_buttons_state();
    if ( 0 != v_changed ) {
      if ( 0 != (v_changed & BT_UP_mask) && 0 == (v_buttons & BT_UP_mask) ) {
        // нажата кнопка "вверх"
        if ( --g_menu_item < 0 ) {
          g_menu_item = MENU_ITEM_MAX;
        }
        settings_items();
      }
      if ( 0 != (v_changed & BT_DOWN_mask) && 0 == (v_buttons & BT_DOWN_mask) ) {
        // нажата кнопка "вниз"
        if ( ++g_menu_item > MENU_ITEM_MAX ) {
          g_menu_item = 0;
        }
        settings_items();
      }
      if ( 0 != (v_changed & BT_OK_mask) && 0 == (v_buttons & BT_OK_mask) ) {
        // нажата кнопка "ОК/Меню"
        display_fill_rectangle_dma_fast( 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_BYTE_COLOR_BLACK );
        if ( MENU_ITEM_BACK == g_menu_item ) {
          // переключаемся на главный экран
          // если настройки были изменены, сохраняем текущий профиль
          
          // чтобы определить наличие изменений, проверим CRC32 профиля
          
          // подключаем канал IN3 АЦП
          adc_select_channel( ADC_IN_RX );
          gui_items();
          g_gui_mode = GUI_MODE_MAIN;
          set_sound_volume( settings_get_current_profile()->level_sound );
        } else {
          // переходим в выбранный пункт меню
          g_menu_level = g_menu_item;
          g_menu_item = 0;
          g_top_menu[g_menu_level].init();
        }
      }
    }
  }
}


void gui_process() {
  switch ( g_gui_mode ) {
    case GUI_MODE_MAIN:
      gui_main();
      break;
    
    case GUI_MODE_SETTINGS:
      gui_settings();
      break;
      
    default:
      gui_main();
      break;
  }
}


static void mi_tx_gen() {
  // для получения первого значения
  g_tmp = -1;
  // подключаем канал IN1 АЦП
  adc_select_channel( ADC_IN_ACC );
  // здесь отрисовка статических элементов экрана
  // строка заголовка
  display_write_string_with_bg(
        0, 0
      , DISPLAY_WIDTH, font_25_30_font.m_row_height
      , g_top_menu[MENU_ITEM_TX_POWER].name
      , &font_25_30_font
      , DISPLAY_COLOR_YELLOW
      , DISPLAY_COLOR_MIDBLUE
      );
  // ток в контуре
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Ток, мА"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
  // уровень сигнала "накачки"
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height*2
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Уровень"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
}


static void rx_balance_init_screen() {
  // здесь отрисовка статических элементов экрана
  // строка заголовка
  display_write_string_with_bg(
        0, 0
      , DISPLAY_WIDTH, font_25_30_font.m_row_height
      , g_top_menu[MENU_ITEM_RX_BALANCE].name
      , &font_25_30_font
      , DISPLAY_COLOR_YELLOW
      , DISPLAY_COLOR_MIDBLUE
      );
  // сигнал RX - длина вектора
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "RX длина"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
  // сигнал RX - угол вектора
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height*2
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "RX угол"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
  // компенсирующий сигнал - уровень
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height*3
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "CM уровень"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , 0 == g_menu_item ? DISPLAY_COLOR_BLUE : DISPLAY_COLOR_DARKBLUE
      );
  // компенсирующий сигнал - начальная фаза
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height*4
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "CM фаза"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , 1 == g_menu_item ? DISPLAY_COLOR_BLUE : DISPLAY_COLOR_DARKBLUE
      );
}


static void mi_rx_balance() {
  rx_balance_init_screen();
  // для получения первого значения
  g_tmp = -1;
}


static void mi_mask() {
}


static void mi_ferrite() {
}


static void mi_power() {
  // для получения первого значения
  g_tmp = -1;
  // подключаем канал IN0 АЦП
  ADC1->SQR3 = 0;
  // строка заголовка
  display_write_string_with_bg(
        0, 0
      , DISPLAY_WIDTH, font_25_30_font.m_row_height
      , g_top_menu[MENU_ITEM_BATTERY].name
      , &font_25_30_font
      , DISPLAY_COLOR_YELLOW
      , DISPLAY_COLOR_MIDBLUE
      );
  // напряжение питания
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Напряжение"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
}


static void mi_save_profile() {
  settings_t * v_current_profile = settings_get_current_profile();
  // строка заголовка
  display_write_string_with_bg(
        0, 0
      , DISPLAY_WIDTH, font_25_30_font.m_row_height
      , g_top_menu[MENU_ITEM_SAVE].name
      , &font_25_30_font
      , DISPLAY_COLOR_YELLOW
      , DISPLAY_COLOR_MIDBLUE
      );
  // активный (загруженный) профиль
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Активный"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
  // его номер
  g_tmp = (int)v_current_profile->profile_id.id;
  sprintf( g_str, "%d", g_tmp );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
  // в какой профиль записать
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height * 2
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Сохранить в"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
}


static void mi_load_profile() {
  settings_t * v_current_profile = settings_get_current_profile();
  // строка заголовка
  display_write_string_with_bg(
        0, 0
      , DISPLAY_WIDTH, font_25_30_font.m_row_height
      , g_top_menu[MENU_ITEM_LOAD].name
      , &font_25_30_font
      , DISPLAY_COLOR_YELLOW
      , DISPLAY_COLOR_MIDBLUE
      );
  // активный (загруженный) профиль
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Активный"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
  // его номер
  sprintf( g_str, "%d", (int)v_current_profile->profile_id.id );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
  // какой загрузить
  display_write_string_with_bg(
        0, font_25_30_font.m_row_height * 2
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Загрузить"
      , &font_25_30_font
      , DISPLAY_COLOR_CYAN
      , DISPLAY_COLOR_DARKBLUE
      );
  // надо прочитать идентификаторы сохранённых профилей
  if ( 0 == load_profiles_pointers( g_profiles_ptr ) ) {
    // сигнал о том, что нет доступных к загрузке профилей
    g_tmp = -1;
  } else {
    // поищем среди доступных к загрузке профиль с идентификатором, таким же, как у текущего
    // если таковой найдётся, проставим его идентификатор в g_tmp
    // иначе проставим туда идентификатор первого по порядку доступного профиля
    if ( g_profiles_ptr[v_current_profile->profile_id.id] ) {
      g_tmp = v_current_profile->profile_id.id;
    } else {
      // предохранитель чисто на всякий случай
      g_tmp = -1;
      // ищем первый попавшийся доступный профиль (из сохранённых во flash)
      for ( int i = 0; i < PROFILES_COUNT; ++i ) {
        if ( g_profiles_ptr[i] ) {
          g_tmp = g_profiles_ptr[i]->profile_id.id;
          break;
        }
      }
    }
  }
}


static void back_to_settings( int a_from_item ) {
  g_menu_item = a_from_item;
  g_menu_level = MENU_ITEM_BACK;
  display_fill_rectangle_dma_fast( 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_BYTE_COLOR_BLACK );
  settings_items();
}


static void mh_tx_gen() {
  settings_t * v_current_profile = settings_get_current_profile();
  // индекс
  int fft_idx = get_fft_idx();
  // ждём заполнения буфера
  bool v_last_flag = adc_buffer_flag();
  while ( adc_buffer_flag() == v_last_flag ) {}
  // быстренько получим адрес буфера
  uint16_t * v_from = adc_get_buffer();
  // копируем выборки в буфер
  for ( int i = 0; i < ADC_SAMPLES_COUNT; ++i ) {
    g_x[i] = ((int)v_from[i]) << 16;
    g_y[i] = 0;
  }
  // БПФ по данным от АЦП
  BPF( g_x, g_y );
  int v_len;
  for ( int i = 0; i < 13; ++i ) {
    v_len = g_x[i];
    int v_d = full_atn( &v_len, g_y[i] );
    v_len /= 65536;
    printf(
        "[%2d]%d x=%4d|y=%4d|r=%4d|d=%3d.%03d\n"
      , i
      , i == fft_idx ? 1 : 0
      , g_x[i] / 65536
      , g_y[i] / 65536
      , v_len
      , v_d / 65536, ((v_d & 0xFFFF) * 1000) / 65536
      );
  }
  // значение "силы тока"
  v_len = g_x[fft_idx];
  full_atn( &v_len, g_y[fft_idx] );
  if ( -1 == g_tmp ) {
    g_tmp = v_len;
  } else {
    g_tmp = (v_len / 16) + ((g_tmp * 15) / 16);
  }
  // с учётом номиналом элементов схемы
  sprintf( g_str, "%u", (unsigned int)((v_current_profile->ampermeter * ((uint64_t)g_tmp)) >> 32) );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // значение "уровня накачки"
  sprintf( g_str, "%u", (unsigned int)get_tx_level() );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height*2
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // кнопки
  uint32_t v_changed = get_changed_buttons();
  uint32_t v_buttons = get_buttons_state();
  if ( 0 != v_changed ) {
    // кнопки "+" и "-" - регулировка уровня сигнала TX
    if ( 0 != (v_changed & BT_INC_mask) && 0 == (v_buttons & BT_INC_mask) ) {
      // нажата кнопка "+"
      set_tx_level( get_tx_level() + 1u );
    }
    if ( 0 != (v_changed & BT_DEC_mask) && 0 == (v_buttons & BT_DEC_mask) ) {
      // нажата кнопка "-"
      set_tx_level( get_tx_level() - 1u );
    }
    // кнопки "вверх" и "вниз" - коррекция показаний измерителя тока в контуре TX
    if ( 0 != (v_changed & BT_UP_mask) && 0 == (v_buttons & BT_UP_mask) ) {
      // нажата кнопка "вверх"
      ++v_current_profile->ampermeter;
    }
    if ( 0 != (v_changed & BT_DOWN_mask) && 0 == (v_buttons & BT_DOWN_mask) ) {
      // нажата кнопка "вниз"
      --v_current_profile->ampermeter;
    }
    // кнопка "ОК" - выход из настройки
    if ( 0 != (v_changed & BT_OK_mask) && 0 == (v_buttons & BT_OK_mask) ) {
      // нажата кнопка "OK"
      back_to_settings( MENU_ITEM_TX_POWER );
    }
  }
}


static void mh_rx_balance() {
  settings_t * v_settings = settings_get_current_profile();
  // индекс
  int fft_idx = get_fft_idx();
  // ждём заполнения буфера
  bool v_last_flag = adc_buffer_flag();
  while ( adc_buffer_flag() == v_last_flag ) {}
  // быстренько получим адрес буфера
  uint16_t * v_from = adc_get_buffer();
  uint32_t v_tx_phase = adc_get_tx_phase();
  // копируем выборки в буфер
  for ( int i = 0; i < ADC_SAMPLES_COUNT; ++i ) {
    g_x[i] = ((int)v_from[i]) << 16;
    g_y[i] = 0;
  }
  // вычисляем фазу в градусах
  int v_tx_phase_deg = (int)(((360ull << 16) * v_tx_phase) / 0x100000000ull);
  // БПФ по данным от АЦП
  BPF( g_x, g_y );
  int v_len = g_x[fft_idx];
  int v_d = full_atn( &v_len, g_y[fft_idx] ) - v_tx_phase_deg;
  if ( v_d < 0 ) {
    v_d += 360 * 65536;
  }
  // значение "длины вектора"
  if ( -1 == g_tmp ) {
    g_tmp = v_len;
  } else {
    g_tmp = (v_len / 16) + (g_tmp * 15) / 16;
  }
  sprintf( g_str, "%d.%d", g_tmp / 65536, ((g_tmp & 0xFFFF) * 10) / 65536 );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // значение "угла вектора"
  sprintf( g_str, "%d.%d", v_d / 65536, ((v_d & 0xFFFF) * 10) / 65536 );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height * 2
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // значение "уровня сигнала компенсации"
  sprintf( g_str, "%u", (unsigned int)get_cm_level() );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height * 3
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // значение "фазы сигнала компенсации"
  unsigned int v_cmp_phase_deg = (unsigned int)(((360ull << 16) * v_settings->phase_comp_start) / 0x100000000ull);
  sprintf( g_str, "%u.%u", v_cmp_phase_deg >> 16, ((v_cmp_phase_deg & 0xFFFF) * 10u) >> 16 );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height * 4
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // кнопки
  uint32_t v_changed = get_changed_buttons();
  uint32_t v_buttons = get_buttons_state();
  if ( 0 != v_changed ) {
    if ( 0 != (v_changed & BT_INC_mask) && 0 == (v_buttons & BT_INC_mask) ) {
      // нажата кнопка "+"
      if ( 0 == g_menu_item ) {
        // увеличиваем уровень сигнала компенсации
        set_cm_level( get_cm_level() + 1u );
      } else {
        uint64_t v_cm_phase_start = v_settings->phase_comp_start;
        // увеличиваем начальную фазу сигнала компенсации
        if ( v_cm_phase_start < 0x100000000ull ) {
          // прибавляем одну десятую градуса на одиночное нажатие и по одному градусу на автоповтор
          v_cm_phase_start += is_repeated(BT_INC_mask) ? 11930464u : 1193046u; // 0x100000000 / 360(0)
          // за 360 градусов не вылезаем
          if ( v_cm_phase_start >= 0x100000000ull ) {
            v_cm_phase_start = 0xFFFFFFFFul;
          }
        }
        // меняем фазу генератора
        if ( ((uint32_t)v_cm_phase_start) != v_settings->phase_comp_start ) {
          set_cm_phase( ((uint32_t)v_cm_phase_start) - v_settings->phase_comp_start );
          v_settings->phase_comp_start = ((uint32_t)v_cm_phase_start);
        }
      }
    }
    if ( 0 != (v_changed & BT_DEC_mask) && 0 == (v_buttons & BT_DEC_mask) ) {
      // нажата кнопка "-"
      if ( 0 == g_menu_item ) {
        // уменьшаем уровень сигнала компенсации
        set_cm_level( get_cm_level() - 1u );
      } else {
        int64_t v_cm_phase_start = v_settings->phase_comp_start;
        // уменьшаем начальную фазу сигнала компенсации
        // убавляем одну десятую градуса на одиночное нажатие и по одному градусу на автоповтор
        v_cm_phase_start -= is_repeated(BT_INC_mask) ? 11930464 : 1193046; // 0x100000000 / 360(0)
        if ( v_cm_phase_start < 0 ) {
          // меньше нуля не делаем
          v_cm_phase_start = 0;
        }
        // меняем фазу генератора
        if ( ((uint32_t)v_cm_phase_start) != v_settings->phase_comp_start ) {
          set_cm_phase( ((uint32_t)v_cm_phase_start) - v_settings->phase_comp_start );
          v_settings->phase_comp_start = (uint32_t)v_cm_phase_start;
        }
      }
    }
    if ( 0 != (v_changed & BT_UP_mask) && 0 == (v_buttons & BT_UP_mask) ) {
      // нажата кнопка "вверх"
      g_menu_item ^= 1;
      // перерисуем вид экрана
      rx_balance_init_screen();
    }
    if ( 0 != (v_changed & BT_DOWN_mask) && 0 == (v_buttons & BT_DOWN_mask) ) {
      // нажата кнопка "вниз"
      g_menu_item ^= 1;
      // перерисуем вид экрана
      rx_balance_init_screen();
    }
    if ( 0 != (v_changed & BT_OK_mask) && 0 == (v_buttons & BT_OK_mask) ) {
      // нажата кнопка "OK"
      back_to_settings( MENU_ITEM_RX_BALANCE );
    }
  }
}


static void mh_mask() {
}


static void mh_ferrite() {
}


static void mh_power() {
  settings_t * v_settings = settings_get_current_profile();
  // ждём заполнения буфера
  bool v_last_flag = adc_buffer_flag();
  while ( adc_buffer_flag() == v_last_flag ) {}
  // быстренько получим адрес буфера
  uint16_t * v_from = adc_get_buffer();
  // собираем среднее значение
  int v_avg = 0;
  for ( int i = 0; i < ADC_SAMPLES_COUNT; ++i ) {
    v_avg += v_from[i];
  }
  //
  v_avg /= (ADC_SAMPLES_COUNT/2);
  //
  if ( -1 == g_tmp ) {
    g_tmp = v_avg;
  } else {
    g_tmp = (v_avg / 8) + ((v_avg * 7) / 8);
  }
  int v_volts = (g_tmp * (int)(v_settings->voltmeter)) / MAX_VOLTMETER_K_VALUE;
  // значение напряжения питания
  sprintf( g_str, "%d.%02d", v_volts / 100, v_volts % 100 );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // кнопки
  uint32_t v_changed = get_changed_buttons();
  uint32_t v_buttons = get_buttons_state();
  if ( 0 != v_changed ) {
    // кнопки "вверх" и "вниз" - коррекция показаний измерителя напряжения питания
    if ( 0 != (v_changed & BT_UP_mask) && 0 == (v_buttons & BT_UP_mask) ) {
      // нажата кнопка "вверх", увеличиваем коэффициент
      if ( v_settings->voltmeter < MAX_VOLTMETER_K_VALUE ) {
        ++v_settings->voltmeter;
      }
    }
    if ( 0 != (v_changed & BT_DOWN_mask) && 0 == (v_buttons & BT_DOWN_mask) ) {
      // нажата кнопка "вниз"
      if ( v_settings->voltmeter > MIN_VOLTMETER_K_VALUE ) {
        --v_settings->voltmeter;
      }
    }
    // кнопка "ОК" - выход из настройки
    if ( 0 != (v_changed & BT_OK_mask) && 0 == (v_buttons & BT_OK_mask) ) {
      // нажата кнопка "OK"
      back_to_settings( MENU_ITEM_TX_POWER );
    }
  }
}


static void mh_save_profile() {
  // отображаем номер выбранного
  sprintf( g_str, "%d", g_tmp );
  display_write_string_with_bg(
          DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height * 2
        , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
        , g_str
        , &font_25_30_font
        , DISPLAY_COLOR_WHITE
        , DISPLAY_COLOR_DARKGRAY
        );
  // кнопки
  uint32_t v_changed = get_changed_buttons();
  uint32_t v_buttons = get_buttons_state();
  if ( 0 != v_changed ) {
    if ( 0 != (v_changed & BT_OK_mask) && 0 == (v_buttons & BT_OK_mask) ) {
      // нажата кнопка "OK"
      back_to_settings( MENU_ITEM_LOAD );
    }
    if ( 0 != (v_changed & BT_DEC_mask) && 0 == (v_buttons & BT_DEC_mask) ) {
      // нажата кнопка "-", выбираем профиль с меньшим идентификатором, если возможно
      if ( g_tmp > 0 ) {
        --g_tmp;
      }
    }
    if ( 0 != (v_changed & BT_INC_mask) && 0 == (v_buttons & BT_INC_mask) ) {
      // нажата кнопка "+", выбираем профиль с большим идентификатором, если возможно
      if ( g_tmp < (PROFILES_COUNT - 1) ) {
        ++g_tmp;
      }
    }
    // для сохранения выбранного профиля нужно одновременно нажать "вверх" и "вниз"
    if ( 0 != (v_changed & (BT_UP_mask | BT_DOWN_mask)) && 0 == (v_buttons & (BT_UP_mask | BT_DOWN_mask)) ) {
      // нажаты кнопки "вверх" и "вниз", сохраняем текущий профиль с выбранным идентификатором
      store_profile( g_tmp );
      sprintf( g_str, "Сохранён профиль %d", settings_get_current_profile()->profile_id.id );
      display_write_string_with_bg(
              0, font_25_30_font.m_row_height * 3
            , DISPLAY_WIDTH, font_25_30_font.m_row_height
            , g_str
            , &font_25_30_font
            , DISPLAY_COLOR_WHITE
            , DISPLAY_COLOR_MIDGREEN
            );
      delay_ms( 2000u );
      back_to_settings( MENU_ITEM_SAVE );
    }
  }
  // сохраняем текущий профиль
}


static void mh_load_profile() {
  if ( -1 == g_tmp ) {
    // нет доступных к загрузке профилей
    display_write_string_with_bg(
            0, font_25_30_font.m_row_height * 3
          , DISPLAY_WIDTH, font_25_30_font.m_row_height
          , "Профилей нет"
          , &font_25_30_font
          , DISPLAY_COLOR_WHITE
          , DISPLAY_COLOR_MIDRED
          );
    delay_ms( 2000u );
    back_to_settings( MENU_ITEM_SAVE );
  } else {
    // есть доступные профили
    // отображаем номер выбранного
    sprintf( g_str, "%d", g_tmp );
    display_write_string_with_bg(
            DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height * 2
          , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
          , g_str
          , &font_25_30_font
          , DISPLAY_COLOR_WHITE
          , DISPLAY_COLOR_DARKGRAY
          );
    // кнопки
    uint32_t v_changed = get_changed_buttons();
    uint32_t v_buttons = get_buttons_state();
    if ( 0 != v_changed ) {
      if ( 0 != (v_changed & BT_OK_mask) && 0 == (v_buttons & BT_OK_mask) ) {
        // нажата кнопка "OK"
        back_to_settings( MENU_ITEM_LOAD );
      }
      if ( 0 != (v_changed & BT_DEC_mask) && 0 == (v_buttons & BT_DEC_mask) ) {
        // нажата кнопка "-", выбираем профиль с меньшим идентификатором, если есть
        for ( int i = g_tmp - 1; i >= 0; --i ) {
          if ( g_profiles_ptr[i] ) {
            g_tmp = i;
            break;
          }
        }
      }
      if ( 0 != (v_changed & BT_INC_mask) && 0 == (v_buttons & BT_INC_mask) ) {
        // нажата кнопка "+", выбираем профиль с большим идентификатором, если есть
        for ( int i = g_tmp + 1; i < PROFILES_COUNT; ++i ) {
          if ( g_profiles_ptr[i] ) {
            g_tmp = i;
            break;
          }
        }
      }
      // для загрузки выбранного профиля нужно одновременно нажать "вверх" и "вниз"
      if ( 0 != (v_changed & (BT_UP_mask | BT_DOWN_mask)) && 0 == (v_buttons & (BT_UP_mask | BT_DOWN_mask)) ) {
        // нажаты кнопки "вверх" и "вниз", загружаем профиль
        if ( load_profile( g_profiles_ptr[g_tmp] ) ) {
          sprintf( g_str, "Загружен профиль %d", settings_get_current_profile()->profile_id.id );
          display_write_string_with_bg(
                  0, font_25_30_font.m_row_height * 3
                , DISPLAY_WIDTH, font_25_30_font.m_row_height
                , g_str
                , &font_25_30_font
                , DISPLAY_COLOR_WHITE
                , DISPLAY_COLOR_MIDGREEN
                );
        } else {
          display_write_string_with_bg(
                  0, font_25_30_font.m_row_height * 3
                , DISPLAY_WIDTH, font_25_30_font.m_row_height
                , "Ошибка загрузки"
                , &font_25_30_font
                , DISPLAY_COLOR_WHITE
                , DISPLAY_COLOR_MIDRED
                );
        }
        delay_ms( 2000u );
        back_to_settings( MENU_ITEM_LOAD );
      }
    }
  }
}
