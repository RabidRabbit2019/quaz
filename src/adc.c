#include "adc.h"
#include "gen_dds.h"

#include "stm32f103x6.h"

void delay_ms( uint32_t a_ms );


// двойной буфер
uint16_t g_adc_buffer[ADC_SAMPLES_COUNT*2];
// фазы TX
uint32_t g_tx_phase_1 = 0;
uint32_t g_tx_phase_2 = 0;

// false - заполнилась вторая половина буфера
// true - заполнилась первая половина буфера
volatile bool g_adc_buffer_flag = false;
// флаг ошибки DMA
volatile bool g_adc_dma_error = false;

//
bool adc_buffer_flag() {
  return g_adc_buffer_flag;
}

//
bool adc_error_flag() {
  return g_adc_dma_error;
}

//
int adc_get_tx_phase() {
  uint32_t v_tx_phase = g_adc_buffer_flag ?  g_tx_phase_1 : g_tx_phase_2;
  return v_tx_phase;
}

//
uint16_t * adc_get_buffer() {
  return g_adc_buffer_flag ? g_adc_buffer : (g_adc_buffer + ADC_SAMPLES_COUNT);
}


//
void adc_startup( int a_channel ) {
  // настраиваем канал 1 DMA для получения данных от АЦП
  DMA1_Channel1->CCR = 0;
  DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
  DMA1_Channel1->CMAR = (uint32_t)g_adc_buffer;
  DMA1->IFCR = DMA_IFCR_CTCIF1
             | DMA_IFCR_CHTIF1
             | DMA_IFCR_CTEIF1
             | DMA_IFCR_CGIF1
             ;
  DMA1_Channel1->CNDTR = ADC_SAMPLES_COUNT*2;
  DMA1_Channel1->CCR = DMA_CCR_PL
                     | DMA_CCR_MSIZE_0
                     | DMA_CCR_PSIZE_0
                     | DMA_CCR_MINC
                     | DMA_CCR_TEIE
                     | DMA_CCR_HTIE
                     | DMA_CCR_TCIE
                     | DMA_CCR_CIRC
                     | DMA_CCR_EN
                     ;
  __NVIC_EnableIRQ( DMA1_Channel1_IRQn );
  // настраиваем ADC1, одиночное преобразование (IN3) по триггеру TIM3_TRGO
  ADC1->SMPR2 = ADC_SMPR2_SMP3_1 | ADC_SMPR2_SMP1_1 | ADC_SMPR2_SMP0_1;
  ADC1->SQR1 = 0;
  ADC1->SQR3 = (a_channel << ADC_SQR3_SQ1_Pos) & ADC_SQR3_SQ1;
  ADC1->SR = 0;
  ADC1->CR1 = ADC_CR1_SCAN;
  ADC1->CR2 = ADC_CR2_EXTTRIG
            | ADC_CR2_EXTSEL_2
            | ADC_CR2_DMA
            | ADC_CR2_ADON
            ;
}


// ADC1 работает по событию TIM1_CC3
// буфер на ADC_SAMPLES_COUNT*2 значений, по прерываниям половины буфера и полного буфера
// запоминается фаза DDS генератора TX
// частота ADC 9 МГц, sample_time = 13.5 тактов, время преобразования (13.5+12.5)/9000000 ~= 2.9 мкс
// при этом период выборки 256/72000000 ~= 3.6 мкс, т.е. АЦП всё успевает
void adc_init() {
  GPIOA->CRL &= ~( GPIO_CRL_MODE3 | GPIO_CRL_CNF3
                 | GPIO_CRL_MODE1 | GPIO_CRL_CNF1
                 | GPIO_CRL_MODE0 | GPIO_CRL_CNF0
                 );
  adc_startup( ADC_IN_RX );
}


void adc_shutdown() {
  // отключаем прерывание
  __NVIC_DisableIRQ( DMA1_Channel1_IRQn );
  // отключаем ADC1
  ADC1->CR2 = 0;
  // делаем сброс ADC1
  RCC->APB2RSTR = RCC_APB2RSTR_ADC1RST;
  delay_ms( 2u );
  RCC->APB2RSTR = 0;
  // отключаем DMA1 Channel1
  DMA1_Channel1->CCR = 0;
  DMA1->IFCR = DMA_IFCR_CTCIF1
             | DMA_IFCR_CHTIF1
             | DMA_IFCR_CTEIF1
             | DMA_IFCR_CGIF1
             ;
  // начальные значения переменных
  g_tx_phase_1 = 0;
  g_tx_phase_2 = 0;
  g_adc_buffer_flag = false;
  g_adc_dma_error = false;
}


void ih_DMA1_Channel1_IRQ() {
  // прервание от канала DMA
  uint32_t v_dma_status = DMA1->ISR;
  //
  if ( 0 != (v_dma_status & DMA_ISR_TEIF1) ) {
    // ошибка
    DMA1_Channel1->CCR = 0;
    // флаг ошибки
    g_adc_dma_error = true;
  } else {
    //
    if ( 0 != (v_dma_status & DMA_ISR_HTIF1 ) ) {
      // набрали первую половину буфера
      // фаза для начала второй половины буфера
      g_tx_phase_2 = get_tx_phase();
      g_adc_buffer_flag = true;
    } else {
      // мы заполнили вторую половину буфера
      // фаза для начала первой половины буфера
      g_tx_phase_1 = get_tx_phase();
      g_adc_buffer_flag = false;
    }
  }
  // очищаем флаги
  DMA1->IFCR = DMA_IFCR_CTCIF1
             | DMA_IFCR_CHTIF1
             | DMA_IFCR_CTEIF1
             | DMA_IFCR_CGIF1
             ;
}


//
void adc_select_channel( int a_channel ) {
  gen_dds_shutdown();
  adc_shutdown();
  ADC1->SQR3 = (a_channel << ADC_SQR3_SQ1_Pos) & ADC_SQR3_SQ1;
  adc_init();
  gen_dds_init();
}
