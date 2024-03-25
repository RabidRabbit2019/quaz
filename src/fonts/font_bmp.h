#ifndef __FONT_BMP_H__
#define __FONT_BMP_H__

#include <stdint.h>
#include <stdbool.h>


#define MAX_FONT_WIDTH    128


#ifdef __cplusplus
extern "C" {
#endif

// packed symbol description
typedef struct {
  uint32_t m_code;       // code
  uint32_t m_offset: 31; // index of first byte symbol's packed data
  uint32_t m_nibble: 1;  // start nibble, 0 - high, 1 - low
  uint8_t m_width;       // width in pixels of symbol bitmap
  uint8_t m_height;      // height in pixels of symbol bitmap
  uint8_t m_x_offset;    // x offset for display symbol
  uint8_t m_y_offset;    // y offset for display symbol
  uint8_t m_x_advance;   // displayed width of symbol
} packed_symbol_desc_s;


// packed font description
typedef struct {
  const uint8_t * m_bmp;            // font packed data ptr
  int m_symbols_count;              // total symbols
  int m_row_height;                 // text row height
  uint32_t m_def_code_idx;          // default symbol index, if symbol code not found
  const packed_symbol_desc_s * m_symbols; // descriptions of symbols ptr
} packed_font_desc_s;


typedef struct {
  int r;
  int g;
  int b;
} rgb_unpacked_s;

// display char structure
typedef struct {
  const packed_font_desc_s * m_font;      // font desc ptr
  const packed_symbol_desc_s * m_symbol;  // symbol desc ptr
  const uint8_t * m_bmp_ptr;              // symbol packed data ptr
  int m_row;                              // current row to display
  uint16_t * m_pixbuf;                    // dst pixels row
  int m_cols_count;                       // width of pixels for symbol place
  uint16_t * m_colors;                    // ptr to 8 colors, scale from background to foreground
  int m_counter;                          // repeated colors
  int m_curr_color;                       // current color
  int m_last_row;                         // last symbol row within it place
  int m_last_col;                         // last symbol col within it place
  uint8_t m_curr_byte;                    // current packed byte
  bool m_curr_nibble;                     // current nibble
} display_char_s;


// prepare to display symbol, init a_data structure
void display_char_init( display_char_s * a_data, uint32_t a_code, const packed_font_desc_s * a_font, uint16_t * a_dst_row, uint16_t a_bgcolor, uint16_t a_fgcolor, uint16_t * a_colors_tbl );
// prepare to display symbol, init a_data structure using existing font, colors and buffer
void display_char_init2( display_char_s * a_data, uint32_t a_code );
// prepare to display symbol, init a_data structure using font and colors from other
void display_char_init3( display_char_s * a_data, uint32_t a_code, uint16_t * a_dst_row, display_char_s * a_from );
// build colors table
void build_colors_table( uint16_t a_bgcolor, uint16_t a_fgcolor, uint16_t * a_colors_tbl );

// prepare one row pixels buffer, returns true, if it was last row
bool display_char_row( display_char_s * a_data );
// get rectangle size for text
void get_text_extent( const packed_font_desc_s * a_font, const char * a_str, int * a_width, int * a_height );
// get next symbol (uni)code
uint32_t get_next_utf8_code( const char ** a_ptr );


#ifdef __cplusplus
}
#endif

#endif // __FONT_BMP_H__
