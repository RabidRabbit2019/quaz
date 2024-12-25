#include "display.h"
#include "stm32g431xx.h"


#define ILI9341_RESET           0x01
#define ILI9341_SLEEP_OUT       0x11
#define ILI9341_INVOFF          0x20
#define ILI9341_GAMMA				    0x26
#define ILI9341_DISPLAY_OFF     0x28
#define ILI9341_DISPLAY_ON      0x29
#define ILI9341_COLUMN_ADDR     0x2A
#define ILI9341_PAGE_ADDR       0x2B
#define ILI9341_GRAM            0x2C
#define ILI9341_PTLAR           0x30
#define ILI9341_MAC             0x36
#define ILI9341_PIXEL_FORMAT    0x3A
#define ILI9341_WDB             0x51
#define ILI9341_WCD             0x53
#define ILI9341_RGB_INTERFACE   0xB0
#define ILI9341_FRC             0xB1
#define ILI9341_BPC             0xB5
#define ILI9341_DFC             0xB6
#define ILI9341_POWER1          0xC0
#define ILI9341_POWER2          0xC1
#define ILI9341_VCOM1           0xC5
#define ILI9341_VCOM2           0xC7
#define ILI9341_POWERA          0xCB
#define ILI9341_POWERB          0xCF
#define ILI9341_PGAMMA          0xE0
#define ILI9341_NGAMMA          0xE1
#define ILI9341_DTCA            0xE8
#define ILI9341_DTCB            0xEA
#define ILI9341_POWER_SEQ       0xED
#define ILI9341_EF              0xEF
#define ILI9341_3GAMMA_EN       0xF2
#define ILI9341_INTERFACE       0xF6
#define ILI9341_PRC             0xF7


#define ILI9341_MAC_MY  0x80
#define ILI9341_MAC_MX  0x40
#define ILI9341_MAC_MV  0x20
#define ILI9341_MAC_ML  0x10
#define ILI9341_MAC_RGB 0x00
#define ILI9341_MAC_BGR 0x08
#define ILI9341_MAC_MH  0x04


extern volatile uint32_t g_milliseconds;
void delay_ms( uint32_t a_ms );


static const uint8_t g_ili9341_init[] = {
  0x05, ILI9341_POWERA, 0x39, 0x2C, 0x00, 0x34, 0x02
, 0x03, ILI9341_POWERB, 0x00, 0xC1, 0x30
, 0x03, ILI9341_EF, 0x03, 0x80, 0x02 // ???
, 0x03, ILI9341_DTCA, 0x85, 0x00, 0x78
, 0x02, ILI9341_DTCB, 0x00, 0x00
, 0x04, ILI9341_POWER_SEQ, 0x64, 0x03, 0x12, 0x81
, 0x01, ILI9341_PRC, 0x20
, 0x01, ILI9341_POWER1, 0x23
, 0x01, ILI9341_POWER2, 0x10
, 0x02, ILI9341_VCOM1, 0x3E, 0x28
, 0x01, ILI9341_VCOM2, 0x86
, 0x01, ILI9341_MAC, ILI9341_MAC_MV | ILI9341_MAC_BGR | ILI9341_MAC_MX | ILI9341_MAC_MY
, 0x01, ILI9341_PIXEL_FORMAT, 0x55
, 0x02, ILI9341_FRC, 0x00, 0x1B
, 0x04, ILI9341_DFC, 0x08, 0x82, 0x27, 0x00
, 0x01, ILI9341_3GAMMA_EN, 0x02
, 0x01, ILI9341_GAMMA, 0x01
, 0x0F, 0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 // ILI9341_PGAMMA
, 0x0F, 0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F // ILI9341_NGAMMA
, 0x00 // end marker
};


static void display_select() {
  GPIOB->BSRR = GPIO_BSRR_BR8;
}


static void display_deselect() {
  GPIOB->BSRR = GPIO_BSRR_BS8;
}


static void display_cmd_mode() {
  GPIOB->BSRR = GPIO_BSRR_BR7;
}


static void display_data_mode() {
  GPIOB->BSRR = GPIO_BSRR_BS7;
}


uint8_t g_dummy = 0;

static void display_spi_write_start( const uint8_t * a_src, uint32_t a_size ) {
  // читаем регистр данных на всяк случай
  SPI1->DR;
  // чистим флаги каналов 2 и 3
  DMA1->IFCR = DMA_IFCR_CTCIF2
             | DMA_IFCR_CHTIF2
             | DMA_IFCR_CTEIF2
             | DMA_IFCR_CGIF2
             | DMA_IFCR_CTCIF3
             | DMA_IFCR_CHTIF3
             | DMA_IFCR_CTEIF3
             | DMA_IFCR_CGIF3
             ;
  // включаем DMA1 канал 2 на приём
  DMA1_Channel2->CMAR = (uint32_t)&g_dummy;
  DMA1_Channel2->CNDTR = a_size;
  DMA1_Channel2->CCR = DMA_CCR_EN;
  // включаем DMA1 канал 3 на передачу
  DMA1_Channel3->CMAR = (uint32_t)a_src;
  DMA1_Channel3->CNDTR = a_size;
  DMA1_Channel3->CCR = DMA_CCR_DIR
                     | DMA_CCR_MINC
                     | DMA_CCR_EN
                     ;
}


static void display_spi_write_end() {
  uint32_t v_from = g_milliseconds;
  // ждём завершения приёма по каналу 2 либо ошибки по каналам 2 и 3
  while ( ((uint32_t)(g_milliseconds - v_from)) < 50u ) {
    uint32_t v_flags = DMA1->ISR;
    if ( 0 != (v_flags & (DMA_ISR_TEIF2 | DMA_ISR_TEIF3 | DMA_ISR_TCIF2)) ) {
      break;
    }
  }
  // disable used channels
  DMA1_Channel2->CCR &= ~DMA_CCR_EN;
  DMA1_Channel3->CCR &= ~DMA_CCR_EN;
}


// синхронная передача данных, с ожиданием завершения передачи
static void display_spi_write( const uint8_t * a_src, uint32_t a_size ) {
  display_spi_write_start( a_src, a_size );
  display_spi_write_end();
}


static void display_write_cmd_dma( uint8_t a_cmd ) {
  display_cmd_mode();
  display_spi_write( &a_cmd, sizeof(a_cmd) );
}


static void display_write_data_dma( const uint8_t * a_buff, uint32_t a_buff_size ) {
  display_data_mode();
  display_spi_write( a_buff, a_buff_size );
}


static void display_reset_dma() {
  display_write_cmd_dma( ILI9341_RESET );
  delay_ms(150);
}


static void display_set_addr_window_dma( uint16_t x, uint16_t y, uint16_t w, uint16_t h ) {
  uint8_t v_cmd_data[4];
  // data for X
  v_cmd_data[0] = x >> 8;
  v_cmd_data[1] = x;
  x += w - 1;
  v_cmd_data[2] = x >> 8;
  v_cmd_data[3] = x;
  // write
  display_write_cmd_dma( ILI9341_COLUMN_ADDR );
  display_write_data_dma( v_cmd_data, sizeof(v_cmd_data) );
  // data for Y
  v_cmd_data[0] = y >> 8;
  v_cmd_data[1] = y;
  y += h - 1;
  v_cmd_data[2] = y >> 8;
  v_cmd_data[3] = y;
  // write
  display_write_cmd_dma( ILI9341_PAGE_ADDR );
  display_write_data_dma( v_cmd_data, sizeof(v_cmd_data) );
  //
  display_write_cmd_dma( ILI9341_GRAM );
  display_data_mode();
}


// рисуем прямоугольник, заполненный цветом
// подготавливается буфер размером w пикселов
// и далее через DMA отправляется h раз
void display_fill_rectangle_dma( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color ) {
  // clipping
  if((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT)) return;
  if((x + w - 1) >= DISPLAY_WIDTH) {
    w = DISPLAY_WIDTH - x;
  }
  if((y + h - 1) >= DISPLAY_HEIGHT) {
    h = DISPLAY_HEIGHT - y;
  }

  // Prepare whole line in a single buffer
  color = (color >> 8) | ((color & 0xFF) << 8);
  uint16_t line[DISPLAY_MAX_LINE_PIXELS];
  uint16_t i;
  for(i = 0; i < w; ++i) {
    line[i] = color;
  }

  display_select();
  display_set_addr_window_dma( x, y, w, h );
  // after call display_set_addr_window() display in data mode
  uint32_t line_bytes = w * sizeof(color);
  for( y = h; y > 0; y-- ) {
      display_spi_write( (uint8_t *)line, line_bytes );
  }
  display_deselect();
}


// рисуем прямоугольник, заполненный цветом, составленным из двух одинаковых байтов
// здесь буфер не требуется, через DMA сразу передаются все пиксели прямоугольника
// соответственно, адрес источника в памяти не меняется, т.е. передаётся один байт color
void display_fill_rectangle_dma_fast( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color ) {
  // clipping
  if((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT)) return;
  if((x + w - 1) >= DISPLAY_WIDTH) {
    w = DISPLAY_WIDTH - x;
  }
  if((y + h - 1) >= DISPLAY_HEIGHT) {
    h = DISPLAY_HEIGHT - y;
  }
  // количество передаваемых байтов
  uint32_t v_bytes = ((uint32_t)w) * h * sizeof(uint16_t);
  //
  display_select();
  display_set_addr_window_dma( x, y, w, h );
  //
  while ( 0 != v_bytes ) {
    // огрничение на одну передачу через DMA - 65535 элементов
    uint32_t v_bytes_to_write = (v_bytes <= 65535) ? v_bytes : 65535;
    // запуск передачи немного отличается от стандартного
    // читаем регистр данных на всяк случай
    SPI1->DR;
    // чистим флаги каналов 2 и 3
    DMA1->IFCR = DMA_IFCR_CTCIF2
               | DMA_IFCR_CHTIF2
               | DMA_IFCR_CTEIF2
               | DMA_IFCR_CGIF2
               | DMA_IFCR_CTCIF3
               | DMA_IFCR_CHTIF3
               | DMA_IFCR_CTEIF3
               | DMA_IFCR_CGIF3
               ;
    // включаем DMA1 канал 2 на приём
    DMA1_Channel2->CMAR = (uint32_t)&g_dummy;
    DMA1_Channel2->CNDTR = v_bytes_to_write;
    DMA1_Channel2->CCR = DMA_CCR_EN;
    // включаем DMA1 канал 3 на передачу
    DMA1_Channel3->CMAR = (uint32_t)&color;
    DMA1_Channel3->CNDTR = v_bytes_to_write;
    DMA1_Channel3->CCR = DMA_CCR_DIR
                       | DMA_CCR_EN
                       ;
    display_spi_write_end();
    v_bytes -= v_bytes_to_write;
  }
  display_deselect();
}


void display_init() {
  // PB3 - SPI1/SCK, PB4 - сброс, PB5 - SPI1/MOSI, PB6 - подсветка, PB7 - команда/данные, PB8 - nCS, 
  // настройка PB4, PB6, PB7 и PB8: output push-pull normal speed
  GPIOB->OTYPER = (GPIOB->OTYPER & ~( GPIO_OTYPER_OT4
                                     | GPIO_OTYPER_OT6
                                     | GPIO_OTYPER_OT7
                                     | GPIO_OTYPER_OT8 
                                     ));
  GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~( GPIO_OSPEEDR_OSPEED4
                                       | GPIO_OSPEEDR_OSPEED6
                                       | GPIO_OSPEEDR_OSPEED7
                                       | GPIO_OSPEEDR_OSPEED8
                                       ))
                 | GPIO_OSPEEDR_OSPEED4_0
                 | GPIO_OSPEEDR_OSPEED6_0
                 | GPIO_OSPEEDR_OSPEED7_0
                 | GPIO_OSPEEDR_OSPEED8_0
                 ;
  GPIOB->MODER = (GPIOB->MODER & ~( GPIO_MODER_MODE4
                                   | GPIO_MODER_MODE6
                                   | GPIO_MODER_MODE7
                                   | GPIO_MODER_MODE8
                                   ))
               | GPIO_MODER_MODE4_0
               | GPIO_MODER_MODE6_0
               | GPIO_MODER_MODE7_0
               | GPIO_MODER_MODE8_0
               ;
  // включаем подсветку сразу, без неё всё равно нихрена не видно :)
  GPIOB->BSRR = GPIO_BSRR_BS6;
  // настройка PB3 и PB5, output push-pull AF5 high speed
  GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(GPIO_AFRL_AFSEL3 | GPIO_AFRL_AFSEL5))
                | (5 << GPIO_AFRL_AFSEL3_Pos)
                | (5 << GPIO_AFRL_AFSEL5_Pos)
                ;
  GPIOB->OTYPER = (GPIOB->OTYPER & ~(GPIO_OTYPER_OT3 | GPIO_OTYPER_OT5));
  GPIOB->OSPEEDR = (GPIOB->OSPEEDR & (GPIO_OSPEEDR_OSPEED3 | GPIO_OSPEEDR_OSPEED5))
                 | GPIO_OSPEEDR_OSPEED3_1
                 | GPIO_OSPEEDR_OSPEED5_1
                 ;
  GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODE3 | GPIO_MODER_MODE5))
               | GPIO_MODER_MODE3_1
               | GPIO_MODER_MODE5_1
               ;
  //
  display_deselect();
  GPIOB->BSRR = GPIO_BSRR_BS4;
  // тактирование
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  // SPI clock = APB2 clock / 4 (72 / 4 = 18 MHz), master mode, SPI mode 3
  // на счёт SPI mode в даташите на ILI9341 непонятки. в таймингах последовательного
  // интерфейса явно нарисована картина SPI mode 3, а в описании передачи данных - SPI mode 0
  // так что выбираем SPI mode 3
  SPI1->CR1 = SPI_CR1_MSTR
            | SPI_CR1_BR_0
            | SPI_CR1_SPE
            | SPI_CR1_SSI
            | SPI_CR1_SSM
            | SPI_CR1_CPOL
            | SPI_CR1_CPHA
            ;
  // настройки DMA для SPI1
  // тактирование DMA1 включено в adc.c
  // enable receive and transmit
  SPI1->CR2 = SPI_CR2_DS_0
            | SPI_CR2_DS_1
            | SPI_CR2_DS_2
            | SPI_CR2_FRXTH
            | SPI_CR2_RXDMAEN
            | SPI_CR2_TXDMAEN;
  DMA1_Channel2->CPAR = (uint32_t)&(SPI1->DR);
  DMA1_Channel3->CPAR = (uint32_t)&(SPI1->DR);
  // настраиваем DMAMUX1_Channel1, запрос от SPI1 приём (resource input #10)
  DMAMUX1_Channel1->CCR = (10 << DMAMUX_CxCR_DMAREQ_ID_Pos);
  // настраиваем DMAMUX1_Channel2, запрос от SPI1 передача (resource input #11)
  DMAMUX1_Channel2->CCR = (11 << DMAMUX_CxCR_DMAREQ_ID_Pos);
  // reset sequence
  delay_ms(150);
  display_select();
  display_reset_dma();
  // issue init commands set
  for ( const uint8_t * v_ptr = g_ili9341_init; *v_ptr; ) {
    // read data byte count, advance ptr
    uint32_t v_cnt = *v_ptr++;
    // read cmd byte, advance ptr
    uint8_t v_cmd = *v_ptr++;
    // write cmd
    display_write_cmd_dma( v_cmd );
    // write cmd dta
    display_write_data_dma( v_ptr, v_cnt );
    // advance ptr
    v_ptr += v_cnt;
  }
  display_write_cmd_dma( ILI9341_SLEEP_OUT );
  delay_ms(6);
  display_write_cmd_dma( ILI9341_DISPLAY_ON );
  display_deselect();
  display_fill_rectangle_dma_fast( 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_BYTE_COLOR_BLACK );
}


// рисуем символ с двойной буферизацией
// 1. подготовить строку пикселов N
// 2. подождать завершения передачи строки N-1
// 2. запустить передачу строки N
// 3. подготовить строку пикселов N+1
// 4. подождать завершения передачи строки N
// 5. запустить передачу строки N+1
// 6. N+=2, перейти к шагу №1
void display_write_char(uint16_t x, uint16_t y, display_char_s * a_data) {
  bool v_last_row = false;
  uint32_t v_bytes_to_write = a_data->m_cols_count * sizeof(uint16_t);
  uint16_t line_buf1[MAX_FONT_WIDTH]; // max font width
  uint16_t line_buf2[MAX_FONT_WIDTH]; // max font width
  
  display_set_addr_window_dma(x, y, a_data->m_cols_count, a_data->m_font->m_row_height);
  
  // write line 1
  a_data->m_pixbuf = line_buf1;
  v_last_row = display_char_row( a_data );
  display_spi_write_start((uint8_t *)line_buf1, v_bytes_to_write);
  //
  while ( !v_last_row ) {
    // write line 2
    a_data->m_pixbuf = line_buf2;
    v_last_row = display_char_row( a_data );
    display_spi_write_end();
    display_spi_write_start((uint8_t *)line_buf2, v_bytes_to_write);
    if ( v_last_row ) {
      break;
    }
    // write line 1
    a_data->m_pixbuf = line_buf1;
    v_last_row = display_char_row( a_data );
    display_spi_write_end();
    display_spi_write_start((uint8_t *)line_buf1, v_bytes_to_write);
  }
  // end wait
  display_spi_write_end();
}


void display_write_string(uint16_t x, uint16_t y, const char* str, const packed_font_desc_s * fnt, uint16_t color, uint16_t bgcolor) {
    bool v_used = false;
    display_char_s v_ds;
    uint16_t v_colors[8];

    display_select();

    for ( uint32_t c = get_next_utf8_code( &str ); 0 != c; c = get_next_utf8_code( &str ) ) {
        if ( '\n' == c ) {
          x = 0;
          y += v_ds.m_font->m_row_height;
          
          if ( (y + v_ds.m_font->m_row_height) >= DISPLAY_HEIGHT ) {
              break;
          }
          continue;
        }
        if ( !v_used ) {
          v_used = true;
          display_char_init( &v_ds, c, fnt, 0, bgcolor, color, v_colors );
        } else {
          display_char_init2( &v_ds, c );
        }
        
        if ( (x + v_ds.m_symbol->m_x_advance) >= DISPLAY_WIDTH ) {
            x = 0;
            y += v_ds.m_font->m_row_height;
            
            if ( (y + v_ds.m_font->m_row_height) >= DISPLAY_HEIGHT ) {
                break;
            }
        }

        display_write_char(x, y, &v_ds);
        x += v_ds.m_symbol->m_x_advance;
    }

    display_deselect();
}


bool prepare_char_line( uint16_t * a_buf, display_char_s * a_symbols, int a_symbols_count ) {
  bool v_result = false;

  // with all symbols  
  for ( int i = 0; i < a_symbols_count; ++i ) {
    // set dst buffer for output symbol's row
    a_symbols->m_pixbuf = a_buf;
    // put symbol's row into buffer
    v_result = display_char_row( a_symbols ) || v_result;
    // advance dst ptr for next symbol
    a_buf += a_symbols->m_symbol->m_x_advance;
    // to next stmbol
    ++a_symbols;
  }

  return v_result;
}


// draw string a_str within rectangle(a_width, a_height) at a_x, a_y
// string for MAX_ONE_STR_SYMBOLS or less symbols, one line
// flicker-free display - line by line for entire rectangle with double-buffer
void display_write_string_with_bg(
            int a_x
          , int a_y
          , int a_width
          , int a_height
          , const char * a_str
          , const packed_font_desc_s * a_fnt
          , uint16_t a_color
          , uint16_t a_bgcolor
          ) {
  // buffer for all display symbols
  display_char_s v_ds[MAX_ONE_STR_SYMBOLS];
  // display colors
  uint16_t v_colors[8];
  // display line buffers
  uint16_t line_buf1[DISPLAY_WIDTH];
  uint16_t line_buf2[DISPLAY_WIDTH];
  // get text bounds
  int v_str_width = 0;
  int v_str_height = 0;
  get_text_extent( a_fnt, a_str, &v_str_width, &v_str_height );
  // start column for first symbol
  int v_start_str_column = (a_width - v_str_width) / 2;
  if ( v_start_str_column < 0 ) {
    v_start_str_column = 0;
  }
  // current column number
  int v_symbol_column = v_start_str_column;

  // prepare symbols
  int v_symbols_count = 0;
  // for each symbol in a_str
  for ( uint32_t c = get_next_utf8_code( &a_str ); 0 != c && v_symbols_count < MAX_ONE_STR_SYMBOLS; c = get_next_utf8_code( &a_str ) ) {
    // prepare display structure
    // buffer ptr set to 0 (zero), as it use double buffering by line
    if ( 0 == v_symbols_count ) {
      display_char_init( &v_ds[v_symbols_count], c, a_fnt, 0, a_bgcolor, a_color, v_colors );
    } else {
      // reuse font, and colors from prev symbol
      display_char_init3( &v_ds[v_symbols_count], c, 0, &v_ds[v_symbols_count - 1] );
    }
    // check str width
    if ( (v_symbol_column + v_ds[v_symbols_count].m_symbol->m_x_advance) >= a_width ) {
      // string up to current symbol wider than display rectangle, skip it and other symbols
      break;
    }
    // advance display offset
    v_symbol_column += v_ds[v_symbols_count].m_symbol->m_x_advance;
    // advance symbols counter
    ++v_symbols_count;
  }
  // prepare paddings for line buffers
  for ( int i = 0; i < v_start_str_column; ++i ) {
    line_buf1[i] = v_colors[0];
    line_buf2[i] = v_colors[0];
  }
  for ( int i = v_symbol_column; i < a_width; ++i ) {
    line_buf1[i] = v_colors[0];
    line_buf2[i] = v_colors[0];
  }
  
  bool v_last_row = false;
  uint32_t bytes_to_write = a_width * sizeof(uint16_t);
  
  display_select();

  // display region as entire rectangle
  display_set_addr_window_dma( a_x, a_y, a_width, a_height );

  // line by line double buffered display
  uint16_t * start_line_buf1 = line_buf1 + v_start_str_column;
  uint16_t * start_line_buf2 = line_buf2 + v_start_str_column;
  
  // write line 1
  v_last_row = prepare_char_line( start_line_buf1, v_ds, v_symbols_count );
  display_spi_write_start( (uint8_t *)line_buf1, bytes_to_write );
  //
  while ( !v_last_row ) {
    // write line 2
    v_last_row = prepare_char_line( start_line_buf2, v_ds, v_symbols_count );
    display_spi_write_end();
    display_spi_write_start((uint8_t *)line_buf2, bytes_to_write);
    if ( v_last_row ) {
      break;
    }
    // write line 1
    v_last_row = prepare_char_line( start_line_buf1, v_ds, v_symbols_count );
    display_spi_write_end();
    display_spi_write_start((uint8_t *)line_buf1, bytes_to_write);
  }
  // end wait
  display_spi_write_end();

  display_deselect();
}
