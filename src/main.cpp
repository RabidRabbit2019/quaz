#include "adc.h"
#include "calc.h"
#include "gen_dds.h"
#include "stm32f103x6.h"

#include <stdio.h>


extern volatile uint32_t g_milliseconds;


void delay_ms( uint32_t a_ms ) {
  uint32_t a_from = g_milliseconds;
  while ( ((uint32_t)(g_milliseconds - a_from)) < a_ms ) {}
}


extern "C" {
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
}


int g_x[ADC_SAMPLES_COUNT], g_y[ADC_SAMPLES_COUNT];

void run() {
  // at start clock source = HSI
  // configure RCC: 8 MHz HSE + PLL x9 = 72 MHz
  // enable HSE
  RCC->CR |= RCC_CR_HSEON;
  // wait for HSE starts
  while ( 0 == (RCC->CR & RCC_CR_HSERDY) ) {}
  // FLASH latency 1
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
  uint32_t * v_from = (uint32_t *)&_sidata;
  uint32_t * v_to = (uint32_t *)&_sdata;
  uint32_t * v_end = (uint32_t *)&_edata;
  while ( v_to < v_end ) {
    *v_to++ = *v_from++;
  }
  v_to = (uint32_t *)&_sbss;
  v_end = (uint32_t *)&_ebss;
  while ( v_to < v_end ) {
    *v_to++ = 0;
  }
  // тактирование AFIO
  RCC->APB2ENR |= ( RCC_APB2ENR_AFIOEN
                  );
  // remap USART1 (PB6 TX, PB7 RX)
  AFIO->MAPR = AFIO_MAPR_USART1_REMAP;
  // тактирование остального на APB2
  RCC->APB2ENR |= ( RCC_APB2ENR_IOPCEN
                  | RCC_APB2ENR_IOPBEN
                  | RCC_APB2ENR_IOPAEN
                  | RCC_APB2ENR_USART1EN
                  | RCC_APB2ENR_ADC1EN
                  | RCC_APB2ENR_TIM1EN
                  );
  // USART1 TX/PB6 RX/PB7
  GPIOB->CRL = (GPIOB->CRL & ~( GPIO_CRL_MODE7 | GPIO_CRL_CNF7
                              | GPIO_CRL_MODE6 | GPIO_CRL_CNF6 ))
                | GPIO_CRL_CNF7_1
                | GPIO_CRL_MODE6_1 | GPIO_CRL_CNF6_1
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
  // включаем тактирование TIM3
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  // включаем тактирование для DMA1
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  // тактирование АЦП 9 МГц (72000000/8)
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8;
  
  // SysTick interrupt for each millisecond
  SysTick_Config( 9000 );
  //
  adc_init();
  gen_dds_init();
  //
  for (;;) {
    bool v_last_flag = adc_buffer_flag();
    while ( adc_buffer_flag() == v_last_flag ) {}
    GPIOC->BSRR = GPIO_BSRR_BR13;
    // копируем выборки в буфер
    uint16_t * v_from = adc_get_buffer();
    for ( int i = 0; i < ADC_SAMPLES_COUNT; ++i ) {
      g_x[i] = ((int)*v_from++) << 16;
      g_y[i] = 0;
    }
    BPF( g_x, g_y );
    //
    printf( "------------------------------\n" );
    for ( int i = 0; i < 10; ++i ) {
      printf(
          "[%d] x = %d  | y = %d | r = %d | d = %u\n"
        , i
        , g_x[i] / 65536
        , g_y[i] / 65536
        , (unsigned int)(columnSqrt((uint64_t)( ((((int64_t)g_x[i])*g_x[i]) >> 16) + ((((int64_t)g_y[i])*g_y[i]) >> 16) )) / 256)
        , full_atn( g_x[i], g_y[i] ) / 65536
        );
    }
    printf( "------------------------------\n" );
    delay_ms( 1000 );
    //
    v_last_flag = adc_buffer_flag();
    while ( adc_buffer_flag() == v_last_flag ) {}
    GPIOC->BSRR = GPIO_BSRR_BS13;
    // копируем выборки в буфер
    v_from = adc_get_buffer();
    for ( int i = 0; i < ADC_SAMPLES_COUNT; ++i ) {
      g_x[i] = ((int)*v_from++) << 16;
      g_y[i] = 0;
    }
    BPF( g_x, g_y );
    //
    printf( "------------------------------\n" );
    for ( int i = 0; i < 10; ++i ) {
      printf(
          "[%d] x = %d  | y = %d | r = %d | d = %u\n"
        , i
        , g_x[i] / 65536
        , g_y[i] / 65536
        , (unsigned int)(columnSqrt((uint64_t)( ((((int64_t)g_x[i])*g_x[i]) >> 16) + ((((int64_t)g_y[i])*g_y[i]) >> 16) )) / 256)
        , full_atn( g_x[i], g_y[i] ) / 65536
        );
    }
    printf( "------------------------------\n" );
    delay_ms( 1000 );
  }
}
