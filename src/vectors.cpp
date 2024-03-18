#include "stm32f103x6.h"

void run();

volatile uint32_t g_milliseconds = 0;

typedef void (*IntFunc) (void);

extern "C" {
void Reset_Handler() { run(); for (;;) {} }
void ih_NMI() {}
void ih_HardFault() {}
void ih_MemManage() {}
void ih_BusFault() {}
void ih_UsageFault() {}
void ih_SVC() {}
void ih_DebugMon() {}
void ih_PendSV() {}
void ih_SysTick() {
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
void ih_DMA1_Channel1_IRQ() {}
void ih_DMA1_Channel2_IRQ() {}
void ih_DMA1_Channel3_IRQ() {}
void ih_DMA1_Channel4_IRQ() {}
void ih_DMA1_Channel5_IRQ() {}
void ih_DMA1_Channel6_IRQ() {}
void ih_DMA1_Channel7_IRQ() {}
void ih_ADC1_2_IRQ() {}
void ih_USB_HP_CAN1_TX_IRQ() {}
void ih_USB_LP_CAN1_RX0_IRQ() {}
void ih_CAN1_RX1_IRQ() {}
void ih_CAN1_SCE_IRQ() {}
void ih_EXTI9_5_IRQ() {}
void ih_TIM1_BRK_IRQ() {}
void ih_TIM1_UP_IRQ();
void ih_TIM1_TRG_COM_IR() {}
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
}

__attribute__ ((section(".isr_vector")))
IntFunc exception_table[] = {
  // Configure Initial Stack Pointer, using STM32F103C6 memory map
	(IntFunc)0x20005000
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
, ih_CAN1_RX1_IRQ
, ih_CAN1_SCE_IRQ
, ih_EXTI9_5_IRQ
, ih_TIM1_BRK_IRQ
, ih_TIM1_UP_IRQ
, ih_TIM1_TRG_COM_IR
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
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, (IntFunc)0xF108F85F
};
