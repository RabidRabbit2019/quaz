#include "adc.h"
#include "calc.h"
#include "gen_dds.h"
#include "display.h"
#include "settings.h"
#include "buttons.h"
#include "fonts/font_25_30.h"
#include "stm32f103x6.h"

#include <stdio.h>


#define VDI_LINES_HEIGHT    45
#define VDI_SECTOR_DEGREES  8
#define VDI_LINES_WIDTH     (DISPLAY_WIDTH*VDI_SECTOR_DEGREES/360)
#define VDI_FFT_COLOR       0x07
#define VDI_SD_COLOR        0xE0

#define GUI_MODE_MAIN       0
#define GUI_MODE_SETTINGS   1

static int g_gui_mode = GUI_MODE_MAIN;

static int g_x[ADC_SAMPLES_COUNT], g_y[ADC_SAMPLES_COUNT];
static uint16_t g_adc_copy[ADC_SAMPLES_COUNT];
static char g_str[256];
static int g_last_column_fft = 0;
static int g_last_column_sd = 0;

static int g_main_item = 0;


void delay_ms( uint32_t a_ms );


void gui_init() {
  settings_t * v_settings = settings_get_current_profile();
  // здесь отрисовка статических элементов экрана
  // чистим экран
  display_fill_rectangle_dma_fast( 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_BYTE_COLOR_BLACK );
  // строка "Порог"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT*2 + font_25_30_font.m_row_height
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Порог"
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKBLUE
      );
  sprintf( g_str, "%u", (unsigned int)v_settings->barrier_level );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, VDI_LINES_HEIGHT*2 + font_25_30_font.m_row_height
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
  // строка "Громкость"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT*2 + (font_25_30_font.m_row_height * 2)
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Громкость"
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKBLUE
      );
  sprintf( g_str, "%u", (unsigned int)v_settings->level_sound );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, VDI_LINES_HEIGHT*2 + (font_25_30_font.m_row_height * 2)
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
  // строка "Феррит"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT*2 + (font_25_30_font.m_row_height * 3)
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Феррит"
      , &font_25_30_font
      , DISPLAY_COLOR_GREEN
      , DISPLAY_COLOR_DARKBLUE
      );
  sprintf( g_str, "%u", ((unsigned int)v_settings->ferrite_angle_fft) >> 16 );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, VDI_LINES_HEIGHT*2 + (font_25_30_font.m_row_height * 3)
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
  // строка "Феррит"
  display_write_string_with_bg(
        0, VDI_LINES_HEIGHT*2 + (font_25_30_font.m_row_height * 4)
      , DISPLAY_WIDTH*2/3, font_25_30_font.m_row_height
      , "Феррит"
      , &font_25_30_font
      , DISPLAY_COLOR_RED
      , DISPLAY_COLOR_DARKBLUE
      );
  sprintf( g_str, "%u", ((unsigned int)v_settings->ferrite_angle_sd) >> 16 );
  display_write_string_with_bg(
        DISPLAY_WIDTH*2/3, VDI_LINES_HEIGHT*2 + (font_25_30_font.m_row_height * 4)
      , DISPLAY_WIDTH - (DISPLAY_WIDTH*2/3), font_25_30_font.m_row_height
      , g_str
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , DISPLAY_COLOR_DARKGRAY
      );
}


static void gui_main() {
  // индекс
  int fft_idx = (int)((64ull * get_tx_freq()) >> 32);
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
    g_adc_copy[i] = v_from[i];
    g_x[i] = ((int)v_from[i]) << 16;
    g_y[i] = 0;
  }
  // вычисляем фазу в градусах
  int v_tx_phase_deg = (int)(((360ull << 16) * v_tx_phase) / 0x100000000ull);
  // БПФ по данным от АЦП
  BPF( g_x, g_y );
  printf( "---- %3d.%03d ----\n", v_tx_phase_deg / 65536, ((v_tx_phase_deg & 0xFFFF) * 1000) / 65536 );
  for ( int i = 0; i < 13; ++i ) {
    int v_d = full_atn( g_x[i], g_y[i] ) - v_tx_phase_deg;
    if ( v_d < 0 ) {
      v_d += 360 << 16;
    }
    unsigned int v_len = (unsigned int)(columnSqrt((uint64_t)( ((((int64_t)g_x[i])*g_x[i]) >> 16) + ((((int64_t)g_y[i])*g_y[i]) >> 16) )) / 256);
    if ( i == fft_idx ) {
      // показометр "VDI"
      v_column = (v_d / VDI_SECTOR_DEGREES) / 65536;
      display_fill_rectangle_dma_fast( g_last_column_fft * VDI_LINES_WIDTH, 0, VDI_LINES_WIDTH, VDI_LINES_HEIGHT, 0 );
      display_fill_rectangle_dma_fast( v_column * VDI_LINES_WIDTH, 0, VDI_LINES_WIDTH, VDI_LINES_HEIGHT, VDI_FFT_COLOR );
      g_last_column_fft = v_column;
      // значение "сила отклика"
      sprintf( g_str, "%u", v_len );
      display_write_string_with_bg( 0, VDI_LINES_HEIGHT*2, DISPLAY_WIDTH/2, 30, g_str, &font_25_30_font, DISPLAY_COLOR_GREEN, DISPLAY_COLOR_DARKGREEN );
    }
    printf(
        "[%2d] x = %4d | y = %4d | r = %4d | d = %3d.%03d\n"
      , i
      , g_x[i] / 65536
      , g_y[i] / 65536
      , v_len
      , v_d / 65536, ((v_d & 0xFFFF) * 1000) / 65536
      );
  }
  // теперь попробуем изобразить синхронный детектор
  // 1-й и 4-й квадранты X+, 2-й и 3-й - X-
  // 1-й квадрант Y+, 2-й квадрант Y-
  // 360 градусов - это 2^32
  // шаг сэмплирования по фазе = g_gen_freq / 2
  // начальная фаза известна.
  int sumX = 0;
  int sumY = 0;
  int cnt = 0;
  // начальная фаза сигнала в окне выборки
  uint32_t samplePhaseStep = get_tx_freq() / 2;
  uint64_t samplePhase = v_tx_phase + (samplePhaseStep / 2);
  // ограничение по целому числу периодов сигнала, помещающихся в окне выборки АЦП
  // т.к. нам надо рассматривать только целое количество периодов
  // отсюда граничение по минимальной частоте, в окно выборки АЦП должен целиком
  // уместиться хотя бы один период сигнала, т.е. g_gen_freq >= 67108864 (2197.265625 Гц)
  uint64_t samplePhaseEdge = (((64ull * get_tx_freq()) >> 32) << 32) + samplePhase;
  //
  for ( int i = 0; i < ADC_SAMPLES_COUNT && samplePhase < samplePhaseEdge; ++i ) {
    //
    uint32_t v_sp = samplePhase & 0xFFFFFFFFul;
    ++cnt;
    if ( v_sp < 0x40000000ul ) {
      // 1-й квадрант
      sumX += g_adc_copy[i];
      sumY += g_adc_copy[i];
    } else {
      if ( v_sp < 0x80000000ul ) {
        // 2-й квадрант
        sumX -= g_adc_copy[i];
        sumY += g_adc_copy[i];
      } else {
        if ( v_sp < 0xC0000000ul ) {
          // 3-й квадрант
          sumX -= g_adc_copy[i];
          sumY -= g_adc_copy[i];
        } else {
          // 4-й квадрант
          sumX += g_adc_copy[i];
          sumY -= g_adc_copy[i];
        }
      }
    }
    //
    samplePhase += samplePhaseStep;
  }
  printf( "sumX: %8d, sumY: %8d, cnt: %4d\n", sumX, sumY, cnt );
  // всем значениям добавляем фиксированную точку, 10 двоичных разрядов
  // считаем X и Y
  int v_X = ((sumX * 1024) / cnt) * 64;
  int v_Y = ((sumY * 1024) / cnt) * 64;
  int v_d = full_atn( v_X, v_Y ) - v_tx_phase_deg;
  if ( v_d < 0 ) {
    v_d += 360 << 16;
  }
  //
  unsigned int v_a = (unsigned int)(columnSqrt( (uint64_t)( ((((int64_t)v_X)*v_X) >> 16) + ((((int64_t)v_Y)*v_Y) >> 16) ) ) / 256);
  printf(
      "X/Y: [%5d.%03d/%5d.%03d], r = %4u, d = %3d.%03d\n"
    , v_X / 65536, ((v_X & 0xFFFF) * 1000) / 65536
    , v_Y / 65536, ((v_Y & 0xFFFF) * 1000) / 65536
    , v_a
    , v_d / 65536, ((v_d & 0xFFFF) * 1000) / 65536
    );
  printf( "------------------------------\n" );
  // показометр "VDI"
  v_column = (v_d / VDI_SECTOR_DEGREES) / 65536;
  display_fill_rectangle_dma_fast( g_last_column_sd * VDI_LINES_WIDTH, VDI_LINES_HEIGHT, VDI_LINES_WIDTH, VDI_LINES_HEIGHT, 0 );
  display_fill_rectangle_dma_fast( v_column * VDI_LINES_WIDTH, VDI_LINES_HEIGHT, VDI_LINES_WIDTH, VDI_LINES_HEIGHT, VDI_SD_COLOR );
  g_last_column_sd = v_column;
  // значение "сила отклика"
  sprintf( g_str, "%u", v_a );
  display_write_string_with_bg( DISPLAY_WIDTH/2, VDI_LINES_HEIGHT*2, DISPLAY_WIDTH/2, 30, g_str, &font_25_30_font, DISPLAY_COLOR_RED, DISPLAY_COLOR_DARKRED );
  /*
  uint32_t v_time = g_milliseconds - v_start_ts;
  sprintf( g_str, "%u", (unsigned int)v_time );
  display_write_string_with_bg( 0, 64, 64, 30, g_str, &font_25_30_font, DISPLAY_COLOR_YELLOW, DISPLAY_COLOR_BLACK );
  */
}


static int g_menu_level = 0; // 0 - корневое меню
static int g_menu_item = 0; // 0 - вернуться обратно

static void mh_tx_gen();
static void mh_rx_balance();
static void mh_mask();
static void mh_ferrite();
static void mh_power();
static void mh_save_profile();

typedef void (*menu_item_handler_t)(void);

typedef struct {
  const char * name;
  menu_item_handler_t handler;
} menu_item_t;

static const menu_item_t g_top_menu[] = {
  {"..", NULL}
, {"Раскачка TX", mh_tx_gen}
, {"Баланс RX", mh_rx_balance}
, {"Маска", mh_mask}
, {"Калибровка", mh_ferrite}
, {"Питание", mh_power}
, {"Сохранить", mh_save_profile}
};


static void gui_settings() {
  for ( int i = 0; i < (int)(sizeof(g_top_menu)/sizeof(g_top_menu[0])); ++i ) {
    display_write_string_with_bg(
        0
      , i * font_25_30_font.m_row_height
      , DISPLAY_WIDTH
      , font_25_30_font.m_row_height
      , g_top_menu[i].name
      , &font_25_30_font
      , DISPLAY_COLOR_WHITE
      , g_menu_item == i ? DISPLAY_COLOR_BLUE : DISPLAY_COLOR_DARKGRAY
      );
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


static void mh_tx_gen() {
}


static void mh_rx_balance() {
}


static void mh_mask() {
}


static void mh_ferrite() {
}


static void mh_power() {
}


static void mh_save_profile() {
}
