#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "fonts/font_bmp.h"
#include "stm32f103x6.h"

#include <stdint.h>


#define DISPLAY_WIDTH             320
#define DISPLAY_HEIGHT            240
#define DISPLAY_MAX_LINE_PIXELS   DISPLAY_WIDTH
// максимальная длина выводимой на экран строки в символах
#define MAX_ONE_STR_SYMBOLS       24

#define RGB565(r,g,b) (((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3))
#define RGB565FINAL(r,g,b) ((r&0xF8)|((g&0xE0)>>5)|((g&0x1C)<<11)|((b&0xF8)<<5))

// Color definitions                       R    G    B
#define DISPLAY_COLOR_BLACK       RGB565(  0,   0,   0)
#define DISPLAY_COLOR_RED         RGB565(255,   0,   0)
#define DISPLAY_COLOR_GREEN       RGB565(  0, 255,   0)
#define DISPLAY_COLOR_BLUE        RGB565(  0,   0, 255)
#define DISPLAY_COLOR_CYAN        RGB565(  0, 255, 255)
#define DISPLAY_COLOR_MAGENTA     RGB565(255,   0, 255)
#define DISPLAY_COLOR_YELLOW      RGB565(255, 255,   0)
#define DISPLAY_COLOR_WHITE       RGB565(255, 255, 255)
#define DISPLAY_COLOR_GRAY        RGB565(127, 127, 127)
#define DISPLAY_COLOR_LIGHTGREY   RGB565(191, 191, 191)
#define DISPLAY_COLOR_PINK        RGB565(255, 191, 191)
#define DISPLAY_COLOR_DARKRED     RGB565( 63,   0,   0)
#define DISPLAY_COLOR_DARKGREEN   RGB565(  0,  63,   0)
#define DISPLAY_COLOR_DARKBLUE    RGB565(  0,   0,  63)
#define DISPLAY_COLOR_DARKCYAN    RGB565(  0,  63,  63)
#define DISPLAY_COLOR_DARKMAGENTA RGB565( 63,   0,  63)
#define DISPLAY_COLOR_DARKYELLOW  RGB565( 63,  63,   0)
#define DISPLAY_COLOR_DARKGRAY    RGB565( 63,  63,  63)


void display_init();
void display_write_char(uint16_t x, uint16_t y, display_char_s * a_data);
void display_write_string(uint16_t x, uint16_t y, const char* str, const packed_font_desc_s * fnt, uint16_t color, uint16_t bgcolor);
void display_fill_rectangle_dma( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_fill_rectangle_dma_fast( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color );
void display_write_string_with_bg(
            int a_x
          , int a_y
          , int a_width
          , int a_height
          , const char * a_str
          , const packed_font_desc_s * a_fnt
          , uint16_t a_color
          , uint16_t a_bgcolor
          );

#endif // __DISPLAY_H__
