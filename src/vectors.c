#include "stm32g431xx.h"

void run();

volatile uint32_t g_milliseconds = 0;

typedef void (*IntFunc) (void);

void Reset_Handler() { run(); for (;;) {} }
void ih_NMI() {}
void ih_HardFault() {}
void ih_MemManage() {}
void ih_BusFault() {}
void ih_UsageFault() {}
void ih_SVC() {}
void ih_DebugMon() {}
void ih_PendSV() {}
void ih_SysTick() { // in use
  ++g_milliseconds;
}
void ih_WWDG_IRQ() {}
void ih_PVD_IRQ() {}
void ih_TAMPER_IRQ() {}
void ih_RTC_IRQ() {}
void ih_FLASH_IRQ() {}
void ih_RCC_IRQ() {}
void ih_EXTI0_IRQ() {}
void ih_EXTI1_IRQ() {}
void ih_EXTI2_IRQ() {}
void ih_EXTI3_IRQ() {}
void ih_EXTI4_IRQ() {}
void ih_DMA1_Channel1_IRQ(); // in use
void ih_DMA1_Channel2_IRQ() {}
void ih_DMA1_Channel3_IRQ() {}
void ih_DMA1_Channel4_IRQ() {}
void ih_DMA1_Channel5_IRQ() {}
void ih_DMA1_Channel6_IRQ() {}
void ih_DMA1_Channel7_IRQ() {}
void ih_ADC1_2_IRQ() {}
void ih_USB_HP_CAN1_TX_IRQ() {}
void ih_USB_LP_CAN1_RX0_IRQ() {}
void ih_FDCAN1_IT0_IRQ() {}
void ih_FDCAN1_IT1_IRQ() {}
void ih_EXTI9_5_IRQ() {}
void ih_TIM1_BRK_IRQ() {}
void ih_TIM1_UP_TIM16_IRQ(); // in use
void ih_TIM1_TRG_COM_IRQ() {}
void ih_TIM1_CC_IRQ() {}
void ih_TIM2_IRQ() {}
void ih_TIM3_IRQ() {}
void ih_TIM4_IRQ() {}
void ih_I2C1_EV_IRQ() {}
void ih_I2C1_ER_IRQ() {}
void ih_I2C2_EV_IRQ() {}
void ih_I2C2_ER_IRQ() {}
void ih_SPI1_IRQ() {}
void ih_SPI2_IRQ() {}
void ih_USART1_IRQ() {}
void ih_USART2_IRQ() {}
void ih_USART3_IRQ() {}
void ih_EXTI15_10_IRQ() {}
void ih_RTC_Alarm_IRQ() {}
void ih_USBWakeUp_IRQ() {}

void ih_TIM8_BRK_IRQ() {}
void ih_TIM8_UP_IRQ() {}
void ih_TIM8_TRG_COM_IRQ() {}
void ih_TIM8_CC_IRQ() {}
void ih_ADC3_IRQ() {}
void ih_FSMC_IRQ() {}
void ih_LPTIM1_IRQ() {}
void ih_TIM5_IRQ() {}
void ih_SPI3_IRQ() {}
void ih_UART4_IRQ() {}
void ih_UART5_IRQ() {}
void ih_TIM6_DAC1_3_UR_IRQ() {}
void ih_TIM7_DAC2_4_UR_IRQ() {}
void ih_DMA2_Channel1_IRQ() {}
void ih_DMA2_Channel2_IRQ() {}
void ih_DMA2_Channel3_IRQ() {}
void ih_DMA2_Channel4_IRQ() {}
void ih_DMA2_Channel5_IRQ() {}
void ih_ADC4_IRQ() {}
void ih_ADC5_IRQ() {}
void ih_UCPD1_IRQ() {}
void ih_COMP1_2_3_IRQ() {}
void ih_COMP4_5_6_IRQ() {}
void ih_COMP7_IRQ() {}
void ih_HRTIM_MASTER_IRQ() {}
void ih_HRTIM_TIMA_IRQ() {}
void ih_HRTIM_TIMB_IRQ() {}
void ih_HRTIM_TIMC_IRQ() {}
void ih_HRTIM_TIMD_IRQ() {}
void ih_HRTIM_TIME_IRQ() {}
void ih_HRTIM_FAULT_IRQ() {}
void ih_HRTIM_TIMF_IRQ() {}
void ih_CRS_IRQ() {}
void ih_SAI_IRQ() {}
void ih_TIM20_BRK_IRQ() {}
void ih_TIM20_UP_IRQ() {}
void ih_TIM20_TRG_COM_IRQ() {}
void ih_TIM20_CC_IRQ() {}
void ih_FPU_IRQ() {}
void ih_I2C4_EV_IRQ() {}
void ih_I2C4_ER_IRQ() {}
void ih_SPI4_IRQ() {}
void ih_AES_IRQ() {}
void ih_FDCAN2_IT0_IRQ() {}
void ih_FDCAN2_IT1_IRQ() {}
void ih_FDCAN3_IT0_IRQ() {}
void ih_FDCAN3_IT1_IRQ() {}
void ih_RNG_IRQ() {}
void ih_LPUART_IRQ() {}
void ih_I2C3_EV_IRQ() {}
void ih_I2C3_ER_IRQ() {}
void ih_DMAMUX_IRQ() {}
void ih_QUADSPI_IRQ() {}
void ih_DMA1_Channel8_IRQ() {}
void ih_DMA2_Channel6_IRQ() {}
void ih_DMA2_Channel7_IRQ() {}
void ih_DMA2_Channel8_IRQ() {}
void ih_CORDIC_IRQ() {}
void ih_FMAC_IRQ() {}



__attribute__ ((section(".isr_vector")))
IntFunc exception_table[] = {
  
// Configure Initial Stack Pointer, using STM32G431BU6 memory map
	(IntFunc)0x20008000
// специфичные для Cortex-M4 прерывания
, Reset_Handler
, ih_NMI
, ih_HardFault
, ih_MemManage
, ih_BusFault
, ih_UsageFault
, 0
, 0
, 0
, 0
, ih_SVC
, ih_DebugMon
, 0
, ih_PendSV
, ih_SysTick
// специфичные для контроллера прерывания
, ih_WWDG_IRQ
, ih_PVD_IRQ
, ih_TAMPER_IRQ
, ih_RTC_IRQ
, ih_FLASH_IRQ
, ih_RCC_IRQ
, ih_EXTI0_IRQ
, ih_EXTI1_IRQ
, ih_EXTI2_IRQ
, ih_EXTI3_IRQ
, ih_EXTI4_IRQ
, ih_DMA1_Channel1_IRQ
, ih_DMA1_Channel2_IRQ
, ih_DMA1_Channel3_IRQ
, ih_DMA1_Channel4_IRQ
, ih_DMA1_Channel5_IRQ
, ih_DMA1_Channel6_IRQ
, ih_DMA1_Channel7_IRQ
, ih_ADC1_2_IRQ
, ih_USB_HP_CAN1_TX_IRQ
, ih_USB_LP_CAN1_RX0_IRQ
, ih_FDCAN1_IT0_IRQ
, ih_FDCAN1_IT1_IRQ
, ih_EXTI9_5_IRQ
, ih_TIM1_BRK_IRQ
, ih_TIM1_UP_TIM16_IRQ
, ih_TIM1_TRG_COM_IRQ
, ih_TIM1_CC_IRQ
, ih_TIM2_IRQ
, ih_TIM3_IRQ
, ih_TIM4_IRQ
, ih_I2C1_EV_IRQ
, ih_I2C1_ER_IRQ
, ih_I2C2_EV_IRQ
, ih_I2C2_ER_IRQ
, ih_SPI1_IRQ
, ih_SPI2_IRQ
, ih_USART1_IRQ
, ih_USART2_IRQ
, ih_USART3_IRQ
, ih_EXTI15_10_IRQ
, ih_RTC_Alarm_IRQ
, ih_USBWakeUp_IRQ

, ih_TIM8_BRK_IRQ
, ih_TIM8_UP_IRQ
, ih_TIM8_TRG_COM_IRQ
, ih_TIM8_CC_IRQ
, ih_ADC3_IRQ
, ih_FSMC_IRQ
, ih_LPTIM1_IRQ
, ih_TIM5_IRQ
, ih_SPI3_IRQ
, ih_UART4_IRQ
, ih_UART5_IRQ
, ih_TIM6_DAC1_3_UR_IRQ
, ih_TIM7_DAC2_4_UR_IRQ
, ih_DMA2_Channel1_IRQ
, ih_DMA2_Channel2_IRQ
, ih_DMA2_Channel3_IRQ
, ih_DMA2_Channel4_IRQ
, ih_DMA2_Channel5_IRQ
, ih_ADC4_IRQ
, ih_ADC5_IRQ
, ih_UCPD1_IRQ
, ih_COMP1_2_3_IRQ
, ih_COMP4_5_6_IRQ
, ih_COMP7_IRQ
, ih_HRTIM_MASTER_IRQ
, ih_HRTIM_TIMA_IRQ
, ih_HRTIM_TIMB_IRQ
, ih_HRTIM_TIMC_IRQ
, ih_HRTIM_TIMD_IRQ
, ih_HRTIM_TIME_IRQ
, ih_HRTIM_FAULT_IRQ
, ih_HRTIM_TIMF_IRQ
, ih_CRS_IRQ
, ih_SAI_IRQ
, ih_TIM20_BRK_IRQ
, ih_TIM20_UP_IRQ
, ih_TIM20_TRG_COM_IRQ
, ih_TIM20_CC_IRQ
, ih_FPU_IRQ
, ih_I2C4_EV_IRQ
, ih_I2C4_ER_IRQ
, ih_SPI4_IRQ
, ih_AES_IRQ
, ih_FDCAN2_IT0_IRQ
, ih_FDCAN2_IT1_IRQ
, ih_FDCAN3_IT0_IRQ
, ih_FDCAN3_IT1_IRQ
, ih_RNG_IRQ
, ih_LPUART_IRQ
, ih_I2C3_EV_IRQ
, ih_I2C3_ER_IRQ
, ih_DMAMUX_IRQ
, ih_QUADSPI_IRQ
, ih_DMA1_Channel8_IRQ
, ih_DMA2_Channel6_IRQ
, ih_DMA2_Channel7_IRQ
, ih_DMA2_Channel8_IRQ
, ih_CORDIC_IRQ
, ih_FMAC_IRQ
};
