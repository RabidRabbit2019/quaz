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
  // enable clock for PIOB, PIOC, USART1, TIM1, AFIO
  RCC->APB2ENR |= ( RCC_APB2ENR_IOPAEN
                  | RCC_APB2ENR_IOPBEN
                  | RCC_APB2ENR_IOPCEN
                  | RCC_APB2ENR_TIM1EN
                  | RCC_APB2ENR_USART1EN
                  | RCC_APB2ENR_AFIOEN
                  );
  // PIOC13 - output push-pull, low speed
  GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_MODE13_Msk | GPIO_CRH_CNF13_Msk))
               | GPIO_CRH_MODE13_1
               ;
  // remap USART1 (PB6 TX, PB7 RX)
  AFIO->MAPR = AFIO_MAPR_USART1_REMAP;
  // USART1 115200 8N1
  // 72E6 / 16 / 39.0625 = 115200
  USART1->BRR = (39 << USART_BRR_DIV_Mantissa_Pos) | (1 << USART_BRR_DIV_Fraction_Pos);
  USART1->CR3 = 0;
  USART1->CR2 = 0;
  USART1->CR1 = USART_CR1_TE
              | USART_CR1_UE
              ;
  //
  printf( "Hello from china STM32F103C6 clone!\n" );
  // SysTick interrupt for each millisecond
  SysTick_Config( 9000 );
  //
  gen_dds_init();
  //
  for (;;) {
    delay_ms( 100 );
    GPIOC->BSRR = GPIO_BSRR_BR13;
    delay_ms( 100 );
    GPIOC->BSRR = GPIO_BSRR_BS13;
  }
}
