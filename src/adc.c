#include "adc.h"
#include "gen_dds.h"

#include "stm32g431xx.h"

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

// ADC1 работает по событию TIM1_CC3
// буфер на ADC_SAMPLES_COUNT*2 значений, по прерываниям половины буфера и полного буфера
// запоминается фаза DDS генератора TX
// частота ADC 36 МГц, sample_time = 92.5 тактов, время преобразования (92.5+12.5)/36000000 ~= 2.9 мкс
// при этом период выборки 256/72000000 ~= 3.6 мкс, т.е. АЦП всё успевает
// ADC12_IN2     PA1 - контроль напряжения питания
// ADC1_IN3      PA2 - контроль тока в контуре TX
// ADC1_IN4      PA3 - сигнал от входного усилителя


void adc_startup( unsigned int a_adc_input ) {
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
                     | DMA_CCR_CIRC
                     | DMA_CCR_TEIE
                     | DMA_CCR_HTIE
                     | DMA_CCR_TCIE
                     ;
  __NVIC_EnableIRQ( DMA1_Channel1_IRQn );
  // настраиваем DMAMUX1_Channel0, запрос от ADC1 (resource input #5)
  DMAMUX1_Channel0->CCR = (5 << DMAMUX_CxCR_DMAREQ_ID_Pos);
  // включаем DMA1_Channel1
  DMA1_Channel1->CCR |= DMA_CCR_EN;
  // тактирование АЦП
  ADC12_COMMON->CCR = ADC_CCR_CKMODE; // делим 144 МГц (AHB) на 4, получаем 36 МГц
  // настраиваем ADC1, одиночное преобразование (вход с номером a_adc_input) по триггеру TIM3_TRGO
  ADC1->ISR = ADC_ISR_ADRDY
            | ADC_ISR_EOSMP
            | ADC_ISR_EOC
            | ADC_ISR_EOS
            | ADC_ISR_OVR
            | ADC_ISR_JEOC
            | ADC_ISR_JEOS
            | ADC_ISR_AWD1
            | ADC_ISR_AWD2
            | ADC_ISR_AWD3
            | ADC_ISR_JQOVF
            ;
  ADC1->CR |= ADC_CR_ADEN;
  // ждём включения АЦП1
  while ( 0 == (ADC1->ISR & ADC_ISR_ADRDY) ) {}
  ADC1->ISR = ADC_ISR_ADRDY;
  ADC1->CR &= ~ADC_CR_DEEPPWD;
  ADC1->CR |= ADC_CR_ADVREGEN;
  delay_ms( 2u );
  ADC1->SMPR1 = ADC_SMPR1_SMP4_0 | ADC_SMPR1_SMP4_2
              | ADC_SMPR1_SMP3_0 | ADC_SMPR1_SMP3_2
              | ADC_SMPR1_SMP2_0 | ADC_SMPR1_SMP2_2
              ;
  ADC1->SQR1 = (a_adc_input & 0x1F) << ADC_SQR1_SQ1_Pos;
  ADC1->CFGR = ADC_CFGR_EXTEN_0
             | ADC_CFGR_EXTSEL_2
             | ADC_CFGR_DMACFG
             | ADC_CFGR_DMAEN
             ;
  // запускаем преобразования
  ADC1->CR |= ADC_CR_ADSTART;
}


void adc_init() {
  // источник тактирования ADC1 - System clock, т.е. 144 МГц
  RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_ADC12SEL))
             | RCC_CCIPR_ADC12SEL_1
             ;
  // включаем тактирование ADC
  RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
  // тактирование DMA1 и DMAMUX1 включено в main.c
  // PA1, PA2, PA3 - аналоговый режим
  GPIOA->MODER |= ( GPIO_MODER_MODE1
                  | GPIO_MODER_MODE2
                  | GPIO_MODER_MODE3
                  );
  //
  adc_startup( ADC_IN_RX );
}


void adc_shutdown() {
  // отключаем прерывание
  __NVIC_DisableIRQ( DMA1_Channel1_IRQn );
  // останавливаем преобразование
  ADC1->CR |= ADC_CR_ADSTP;
  // ждём останова
  while ( 0 != (ADC1->CR & (ADC_CR_ADSTART | ADC_CR_ADSTP)) ) {}
  // отключаем ADC1
  ADC1->CR |= ADC_CR_ADDIS;
  while ( 0 != (ADC1->CR & ADC_CR_ADEN) ) {}
  // делаем сброс ADC1
  RCC->AHB2RSTR = RCC_AHB2RSTR_ADC12RST;
  delay_ms( 2u );
  RCC->AHB2RSTR = 0;
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
