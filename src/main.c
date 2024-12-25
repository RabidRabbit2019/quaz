#include "adc.h"
#include "calc.h"
#include "gen_dds.h"
#include "display.h"
#include "settings.h"
#include "buttons.h"
#include "gui.h"
#include "fonts/font_25_30.h"
#include "stm32g431xx.h"

#include <stdio.h>

// STM32G431CBU6 назначение выводов, всё использованное
// -> АЦП
// PA1 - ADC12_IN2
// PA2 - ADC1_IN3
// PA3 - ADC1_IN4
// -> ЦАП
// PA4 - DAC1_OUT1, PA5 - DAC1_OUT2 (Analog)
// -> ШИМ, звук
// PA8 - TIM1_CH1 (AF6)
// -> UART, отладка
// PA9 - USART1_TX (AF7)
// -> экран 
// PB3 - SPI1/SCK, PB5 - SPI1/MOSI (AF5)
// PB4 - сброс, PB6 - подсветка, PB7 - команда/данные, PB8 - nCS, 
// -> индикатор
// PC6 - LED

// PA: 1, 2, 3, 4, 5, 8, 9
// PB: 3, 4, 5, 6, 7, 8
// PC: 6


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
  // configure RCC: 8 MHz HSE + PLL/1*36/2 = 144 MHz
  // enable HSE
  RCC->CR |= RCC_CR_HSEON;
  // wait for HSE starts
  while ( 0 == (RCC->CR & RCC_CR_HSERDY) ) {}
  // FLASH latency 2
  FLASH->ACR = (FLASH->ACR & ~(FLASH_ACR_LATENCY))
             | FLASH_ACR_LATENCY_4WS
             ;
  // clock params: PLL = (HSE/1)*36 = 288 MHz, PLLR = PLL/2 = 144 MHz, AHB = PLLR/1, APB1 = PLLR/2, APB2 = PLLR/2
  RCC->CFGR = RCC_CFGR_SW_HSI
            | RCC_CFGR_HPRE_DIV2   // сначала подключаем частоту AHB=PLL/2=72MHz
            | RCC_CFGR_PPRE1_DIV2
            | RCC_CFGR_PPRE2_DIV2
            ;
  RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE
               | RCC_PLLCFGR_PLLN_5 // 
               | RCC_PLLCFGR_PLLN_2 // *36 (32+4)
               | RCC_PLLCFGR_PLLREN
               ;
  // enable PLL
  RCC->CR |= RCC_CR_PLLON;
  // wait for PLL starts
  while ( 0 == (RCC->CR & RCC_CR_PLLRDY) ) {}
  // switch clock source from HSI to PLL, it works because SW_HSI16 = 1, SW_PLL = 3
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  // SysTick interrupt for each millisecond
  SysTick_Config2( 18000 );
  __enable_irq();
  // ждём больше 1 мкс.
  delay_ms( 2 );
  // ставим делитель 1 для AHB (частота AHB  = 144МГц)
  RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE))
            | RCC_CFGR_HPRE_DIV1
            ;
  // now clock at 144 MHz, AHB 144 MHz, APB1 72 MHz, APB2 72 MHz
  // DMAMUX1
  RCC->AHB1ENR |= RCC_AHB1ENR_DMAMUX1EN;
  // GPIO
  RCC->AHB2ENR |= ( RCC_AHB2ENR_GPIOAEN
                  | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN
                  );
  // тактирование USART1
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  // USART1 TX/PA9 AF7
  GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL9)
                | (7 << GPIO_AFRH_AFSEL9_Pos)
                ;
  GPIOA->OTYPER = (GPIOA->OTYPER & ~GPIO_OTYPER_OT9);
  GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~GPIO_OSPEEDR_OSPEED9)
                 | GPIO_OSPEEDR_OSPEED9_0
                 ;
  GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE9)
               | GPIO_MODER_MODE9_1
               ;
  // USART1 115200 8N1
  // 72E6 / 625 = 115200
  USART1->CR1 = 0;
  USART1->BRR = 625;
  USART1->CR2 = 0;
  USART1->CR1 |= USART_CR1_UE;
  USART1->CR1 |= USART_CR1_TE;
  // PC6 - output push-pull, low speed, синий светодиод, подключен к GND
  GPIOC->OTYPER = 0;
  GPIOC->OSPEEDR = 0;
  GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODE6)
               | GPIO_MODER_MODE6_0
               ;
  GPIOC->BSRR = GPIO_BSRR_BS6;
  //
  settings_init();
  // buttons_init();
  adc_init();
  gen_dds_init();
  display_init();
  //
  display_write_string_with_bg( 0, 100, 320, 40, "https://www.md4u.ru", &font_25_30_font, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_DARKBLUE );
  delay_ms(3000u);
  // gui_init();
  // основной цикл
  printf( "Hello from WeAct STM32G431CBU6 fat board!\n" );
  for (;;) {
    /*
    buttons_scan();
    gui_process();
    */
    //
    GPIOC->ODR ^= GPIO_ODR_OD6;
    delay_ms( 500u );
  }
}
