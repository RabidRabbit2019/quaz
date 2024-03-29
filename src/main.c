#include "adc.h"
#include "calc.h"
#include "gen_dds.h"
#include "display.h"
#include "fonts/font_25_30.h"
#include "stm32f103x6.h"

#include <stdio.h>


extern volatile uint32_t g_milliseconds;


void delay_ms( uint32_t a_ms ) {
  uint32_t a_from = g_milliseconds;
  while ( ((uint32_t)(g_milliseconds - a_from)) < a_ms ) {}
}


extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;


int g_x[ADC_SAMPLES_COUNT], g_y[ADC_SAMPLES_COUNT];
uint16_t g_adc_copy[ADC_SAMPLES_COUNT];
char g_str[256];
int g_last_column = 0;

void run() {
  // copy initialized data
  uint32_t * v_ptr = (uint32_t *)&_sidata;
  for ( uint32_t * i = (uint32_t *)&_sdata; i < (uint32_t *)&_edata; ++i ) {
    *i = *v_ptr++;
  }
  // zero initialized data
  for ( uint32_t * i = (uint32_t *)&_sbss; i < (uint32_t *)&_ebss; ++i ) {
    *i = 0;
  }
  // at start clock source = HSI
  // configure RCC: 8 MHz HSE + PLL x9 = 72 MHz
  // enable HSE
  RCC->CR |= RCC_CR_HSEON;
  // wait for HSE starts
  while ( 0 == (RCC->CR & RCC_CR_HSERDY) ) {}
  // FLASH latency 2
  FLASH->ACR = FLASH_ACR_PRFTBE
             | FLASH_ACR_LATENCY_2
             ;
  // clock params: PLL = (HSE/1)*9, AHB = PLL/1, APB1 = PLL/2, APB2 = PLL/1
  RCC->CFGR = RCC_CFGR_SW_HSI
            | RCC_CFGR_HPRE_DIV1
            | RCC_CFGR_PPRE1_DIV2
            | RCC_CFGR_PPRE2_DIV1
            | RCC_CFGR_ADCPRE_DIV2
            | RCC_CFGR_PLLSRC
            | RCC_CFGR_PLLXTPRE_HSE
            | RCC_CFGR_PLLMULL9
            | RCC_CFGR_USBPRE
            | RCC_CFGR_MCO_NOCLOCK
            ;
  // enable PLL
  RCC->CR |= RCC_CR_PLLON;
  // wait for PLL starts
  while ( 0 == (RCC->CR & RCC_CR_PLLRDY) ) {}
  // switch clock source from HSI to PLL, it works because SW_HSI = 0
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  // now clock at 72 MHz, AHB 72 MHz, APB1 36 MHz, APB2 72 MHz
  // тактирование AFIO
  RCC->APB2ENR |= ( RCC_APB2ENR_AFIOEN
                  );
  // remap USART1 (PB6 TX, PB7 RX)
  AFIO->MAPR = AFIO_MAPR_USART1_REMAP | AFIO_MAPR_I2C1_REMAP;
  // тактирование остального на APB2
  RCC->APB2ENR |= ( RCC_APB2ENR_IOPCEN
                  | RCC_APB2ENR_IOPBEN
                  | RCC_APB2ENR_IOPAEN
                  | RCC_APB2ENR_USART1EN
                  | RCC_APB2ENR_ADC1EN
                  | RCC_APB2ENR_TIM1EN
                  | RCC_APB2ENR_SPI1EN
                  );
  // USART1 TX/PB6 RX/PB7
  GPIOB->CRL = (GPIOB->CRL & ~( GPIO_CRL_MODE7 | GPIO_CRL_CNF7
                              | GPIO_CRL_MODE6 | GPIO_CRL_CNF6 ))
                | GPIO_CRL_CNF7_1
                | GPIO_CRL_MODE6_0 | GPIO_CRL_CNF6_1
               ;
  // USART1 115200 8N1
  // 72E6 / 16 / 39.0625 = 115200
  USART1->SR = 0;
  USART1->BRR = (39 << USART_BRR_DIV_Mantissa_Pos) | (1 << USART_BRR_DIV_Fraction_Pos);
  USART1->CR3 = 0;
  USART1->CR2 = 0;
  USART1->CR1 = USART_CR1_TE
              | USART_CR1_UE
              ;
  // PIOC13 - output push-pull, low speed
  GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_MODE13_Msk | GPIO_CRH_CNF13_Msk))
               | GPIO_CRH_MODE13_1
               ;
  GPIOC->BSRR = GPIO_BSRR_BS13;
  // включаем тактирование TIM3 и I2C1
  RCC->APB1ENR |= ( RCC_APB1ENR_TIM3EN
                  | RCC_APB1ENR_I2C1EN
                  );
  // включаем тактирование для DMA1
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  // тактирование АЦП 9 МГц (72000000/8)
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8;
  
  // SysTick interrupt for each millisecond
  SysTick_Config( 9000 );
  //
  adc_init();
  gen_dds_init();
  display_init();
  display_write_string_with_bg( 0, 100, 320, 40, "https://www.md4u.ru", &font_25_30_font, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_DARKBLUE );
  delay_ms(3000u);
  display_fill_rectangle_dma( 0, 100, 320, 40, 0 );
  //
  for (;;) {
    bool v_last_flag = adc_buffer_flag();
    while ( adc_buffer_flag() == v_last_flag ) {}
    uint32_t v_start_ts = g_milliseconds;
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
      printf(
          "[%2d] x = %4d | y = %4d | r = %4d | d = %3d.%03d\n"
        , i
        , g_x[i] / 65536
        , g_y[i] / 65536
        , (unsigned int)(columnSqrt((uint64_t)( ((((int64_t)g_x[i])*g_x[i]) >> 16) + ((((int64_t)g_y[i])*g_y[i]) >> 16) )) / 256)
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
        "X/Y: [%5d.%03d/%5d.%03d], r = %4d, d = %3d.%03d\n"
      , v_X / 65536, ((v_X & 0xFFFF) * 1000) / 65536
      , v_Y / 65536, ((v_Y & 0xFFFF) * 1000) / 65536
      , v_a
      , v_d / 65536, ((v_d & 0xFFFF) * 1000) / 65536
      );
    printf( "------------------------------\n" );
    //
    int v_column = (v_d / 8) / 65536;
    display_fill_rectangle_dma_fast( g_last_column * 7, 0, 7, 64, 0 );
    display_fill_rectangle_dma_fast( v_column * 7, 0, 7, 64, 0xFF );
    g_last_column = v_column;
    //
    uint32_t v_time = g_milliseconds - v_start_ts;
    sprintf( g_str, "%u", (unsigned int)v_time );
    display_write_string_with_bg( 0, 64, 64, 30, g_str, &font_25_30_font, DISPLAY_COLOR_YELLOW, DISPLAY_COLOR_BLACK );
    //
    if ( 0 == (GPIOC->ODR & GPIO_ODR_ODR13) ) {
      GPIOC->BSRR = GPIO_BSRR_BS13;
    } else {
      GPIOC->BSRR = GPIO_BSRR_BR13;
    }
    delay_ms( 915 );
  }
}
