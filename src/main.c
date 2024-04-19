#include "adc.h"
#include "calc.h"
#include "gen_dds.h"
#include "display.h"
#include "settings.h"
#include "buttons.h"
#include "gui.h"
#include "fonts/font_25_30.h"
#include "stm32f103x6.h"

#include <stdio.h>


extern volatile uint32_t g_milliseconds;


void delay_ms( uint32_t a_ms ) {
  uint32_t a_from = g_milliseconds;
  while ( ((uint32_t)(g_milliseconds - a_from)) < a_ms ) {}
}


extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;


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
  GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13))
               | GPIO_CRH_MODE13_1
               ;
  GPIOC->BSRR = GPIO_BSRR_BS13;
  // включаем тактирование TIM3
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  // включаем тактирование для DMA1 и CRC
  RCC->AHBENR |= ( RCC_AHBENR_DMA1EN
                 | RCC_AHBENR_CRCEN
                 );
  // тактирование АЦП 9 МГц (72000000/8)
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8;
  
  // SysTick interrupt for each millisecond
  SysTick_Config( 9000 );
  //
  settings_init();
  buttons_init();
  adc_init();
  gen_dds_init();
  display_init();
  //
  display_write_string_with_bg( 0, 100, 320, 40, "https://www.md4u.ru", &font_25_30_font, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_DARKBLUE );
  delay_ms(3000u);
  gui_init();
  // основной цикл
  for (;;) {
    buttons_scan();
    gui_process();
    //
    if ( 0 == (GPIOC->ODR & GPIO_ODR_ODR13) ) {
      GPIOC->BSRR = GPIO_BSRR_BS13;
    } else {
      GPIOC->BSRR = GPIO_BSRR_BR13;
    }
    //delay_ms( 1000u );
  }
}
