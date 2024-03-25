#include "font_bmp.h"

#ifdef __cplusplus
extern "C" {
#endif

// find symbol desc by it's code
// returns symbol desc index within a_font->m_symbols
static int find_symbol_index( const packed_font_desc_s * a_font, uint32_t a_code ) {
  int l, m, u;
  l = 0;
  u = a_font->m_symbols_count - 1;
  do {
    m = (l + u) / 2;
    if ( a_font->m_symbols[m].m_code == a_code ) {
      // found
      return m;
    }
    if ( a_font->m_symbols[m].m_code > a_code ) {
      // lower part
      u = m - 1;
    } else {
      // upper part
      l = m + 1;
    }
  } while ( l <= u );
  return a_font->m_def_code_idx;
}


// unpack from R5G6B5 to R8, G8, B8
static void unpack_color( rgb_unpacked_s * a_dst, uint16_t a_color ) {
  a_dst->r = (a_color >> 8) & 0xF8;
  a_dst->g = (a_color >> 3) & 0xFC;
  a_dst->b = (a_color << 3) & 0xF8;
}

// pack from R8, G8, B8 to R5G6B5
static uint16_t pack_color( rgb_unpacked_s * a_src ) {
  return (uint16_t)(
      ((a_src->r & 0xF8) << 8)
    | ((a_src->g & 0xFC) << 3)
    | ((a_src->b & 0xF8) >> 3)
    );
}


// prepare colors table
void build_colors_table( uint16_t a_bgcolor, uint16_t a_fgcolor, uint16_t * a_colors_tbl ) {
  rgb_unpacked_s v_rgb_bg;
  rgb_unpacked_s v_rgb_fg;
  rgb_unpacked_s v_rgb;
  unpack_color( &(v_rgb_bg), a_bgcolor );
  unpack_color( &(v_rgb_fg), a_fgcolor );
  for ( int i = 0; i < 8; ++i ) {
    v_rgb.r = ((v_rgb_bg.r * (7 - i)) / 7)
            + ((v_rgb_fg.r * i) / 7)
            ;
    v_rgb.g = ((v_rgb_bg.g * (7 - i)) / 7)
            + ((v_rgb_fg.g * i) / 7)
            ;
    v_rgb.b = ((v_rgb_bg.b * (7 - i)) / 7)
            + ((v_rgb_fg.b * i) / 7)
            ;
    uint16_t v_c = pack_color( &v_rgb );
    a_colors_tbl[i] = (v_c >> 8) | (v_c << 8);
  }
}


// prepare to display symbol, init a_data structure
void display_char_init(
        display_char_s * a_data
      , uint32_t a_code
      , const packed_font_desc_s * a_font
      , uint16_t * a_dst_row
      , uint16_t a_bgcolor
      , uint16_t a_fgcolor
      , uint16_t * a_colors_tbl
       ) {
  a_data->m_font = a_font;
  a_data->m_symbol = &(a_font->m_symbols[find_symbol_index(a_font, a_code)]);
  a_data->m_bmp_ptr = a_font->m_bmp + a_data->m_symbol->m_offset;
  a_data->m_curr_nibble = a_data->m_symbol->m_nibble;
  a_data->m_row = 0;
  a_data->m_pixbuf = a_dst_row;
  a_data->m_cols_count = a_data->m_symbol->m_x_advance;
  a_data->m_counter = 0;
  a_data->m_curr_color = 0;
  a_data->m_curr_byte = *a_data->m_bmp_ptr++;
  a_data->m_last_row = a_data->m_symbol->m_y_offset + a_data->m_symbol->m_height;
  a_data->m_last_col = a_data->m_symbol->m_x_offset + a_data->m_symbol->m_width;
  // gen colors table
  build_colors_table( a_bgcolor, a_fgcolor, a_colors_tbl );
  a_data->m_colors = a_colors_tbl;
}


// prepare to display symbol, using existing font, colors and buffer
void display_char_init2( display_char_s * a_data, uint32_t a_code ) {
  a_data->m_symbol = &(a_data->m_font->m_symbols[find_symbol_index(a_data->m_font, a_code)]);
  a_data->m_bmp_ptr = a_data->m_font->m_bmp + a_data->m_symbol->m_offset;
  a_data->m_curr_nibble = a_data->m_symbol->m_nibble;
  a_data->m_row = 0;
  a_data->m_cols_count = a_data->m_symbol->m_x_advance;
  a_data->m_counter = 0;
  a_data->m_curr_color = 0;
  a_data->m_curr_byte = *a_data->m_bmp_ptr++;
  a_data->m_last_row = a_data->m_symbol->m_y_offset + a_data->m_symbol->m_height;
  a_data->m_last_col = a_data->m_symbol->m_x_offset + a_data->m_symbol->m_width;
}


// prepare to display symbol, init a_data structure using font and colors from other
void display_char_init3( display_char_s * a_data, uint32_t a_code, uint16_t * a_dst_row, display_char_s * a_from ) {
  // copy font ptr
  a_data->m_font = a_from->m_font;
  // prepare fields
  display_char_init2( a_data, a_code );
  // set buffer
  a_data->m_pixbuf = a_dst_row;
  // copy colors ptr
  a_data->m_colors = a_from->m_colors;
}


// prepare one row pixels buffer, returns 0 (zero), if it was last row
bool display_char_row( display_char_s * a_data ) {
  uint16_t * a_dst = a_data->m_pixbuf;
  // first fill background for y offset
  if ( a_data->m_row < a_data->m_symbol->m_y_offset ) {
    for ( int i = 0; i < a_data->m_symbol->m_x_advance; ++i ) {
      *a_dst++ = a_data->m_colors[0];
    }
  } else {
    if ( a_data->m_row < a_data->m_last_row ) {
      int v_col = 0;
      // first fill background for x offset
      for ( ; v_col < a_data->m_symbol->m_x_offset; ++v_col ) {
        *a_dst++ = a_data->m_colors[0];
      }
      // next check counter
      if ( a_data->m_counter > 0 ) {
        // fill repeated color
        for ( ; a_data->m_counter > 0 && v_col < a_data->m_last_col; --a_data->m_counter, ++v_col ) {
          *a_dst++ = a_data->m_colors[a_data->m_curr_color];
        }
      }
      // next pixels
      for ( ; v_col < a_data->m_last_col; ++v_col ) {
        uint8_t v_packed_color;
        if ( a_data->m_curr_nibble ) {
          // low nibble
          v_packed_color = a_data->m_curr_byte & 0x0F;
          a_data->m_curr_nibble = false;
          a_data->m_curr_byte = *a_data->m_bmp_ptr++;
        } else {
          // high nibble
          v_packed_color = (a_data->m_curr_byte >> 4) & 0x0F;
          a_data->m_curr_nibble = true;
        }
        // it is color or repeat?
        if ( 0 == (v_packed_color & 0x08) ) {
          a_data->m_curr_color = v_packed_color;
          *a_dst++ = a_data->m_colors[v_packed_color];
        } else {
          a_data->m_counter = (v_packed_color & 0x07) + 1;
          for ( ; a_data->m_counter > 0 && v_col < a_data->m_last_col; --a_data->m_counter, ++v_col ) {
            *a_dst++ = a_data->m_colors[a_data->m_curr_color];
          }
          --v_col;
        }
      }
      // background color up to x_advance
      for ( ; v_col < a_data->m_symbol->m_x_advance; ++v_col ) {
        *a_dst++ = a_data->m_colors[0];
      }
    } else {
      // bottom space
      for ( int i = 0; i < a_data->m_symbol->m_x_advance; ++i ) {
        *a_dst++ = a_data->m_colors[0];
      }
    }
  }
  //
  return ++a_data->m_row >= a_data->m_font->m_row_height;
}


//
uint32_t get_next_utf8_code( const char ** a_ptr ) {
  uint8_t c0 = (uint8_t)*(*a_ptr)++;
  // one byte
  if ( 0 == (c0 & 0x80) ) {
    return c0;
  }
  // two bytes
  if ( 0xC0 == (c0 & 0xE0) ) {
    uint8_t c1 = (uint8_t)*(*a_ptr)++;
    if ( 0x80 == (c1 & 0xC0) ) {
      return ((c0 & 0x1F) << 6) | (c1 & 0x3F);
    } else {
      return 0;
    }
  }
  // three bytes
  if ( 0xE0 == (c0 & 0xF0) ) {
    uint8_t c1 = (uint8_t)*(*a_ptr)++;
    if ( 0x80 == (c1 & 0xC0) ) {
      uint8_t c2 = (uint8_t)*(*a_ptr)++;
      if ( 0x80 == (c2 & 0xC0) ) {
        return ((c0 & 0x1F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  // four bytes
  if ( 0xF0 == (c0 & 0xF8) ) {
    uint8_t c1 = (uint8_t)*(*a_ptr)++;
    if ( 0x80 == (c1 & 0xC0) ) {
      uint8_t c2 = (uint8_t)*(*a_ptr)++;
      if ( 0x80 == (c2 & 0xC0) ) {
        uint8_t c3 = (uint8_t)*(*a_ptr)++;
        if ( 0x80 == (c3 & 0xC0) ) {
          return ((c0 & 0x1F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}


//
void get_text_extent( const packed_font_desc_s * a_font, const char * a_str, int * a_width, int * a_height ) {
  int v_width = 0;
  int v_height = 0;
  int v_max_width = 0;
  int v_height_add = a_font->m_row_height;
  int v_idx;
  // scan all characters
  for ( uint32_t c = get_next_utf8_code( &a_str ); 0 != c; c = get_next_utf8_code( &a_str ) ) {
    if ( '\r' == c ) {
      // CR
      v_width = 0;
      continue;
    }
    if ( '\n' == c ) {
      // LF unix style
      v_height_add += a_font->m_row_height;
      v_width = 0;
      continue;
    }
    // increase height at first symbol
    if ( 0 == v_width ) {
      v_height += v_height_add;
      v_height_add = 0;
    }
    //
    v_idx = find_symbol_index( a_font, c );
    v_width += a_font->m_symbols[v_idx].m_x_advance;
    //
    if ( v_width > v_max_width ) {
      v_max_width = v_width;
    }
  }
  // result
  *a_width = v_max_width;
  *a_height = v_height;
}


#ifdef __cplusplus
}
#endif
