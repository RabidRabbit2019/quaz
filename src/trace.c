#include "trace.h"
#include "stm32g431xx.h"

//------------------------------------------------------------------------------
/// Outputs a character on the DBGU line.
/// \note This function is synchronous (i.e. uses polling).
/// \param c  Character to send.
//------------------------------------------------------------------------------
void DBGU_PutChar(unsigned char c)
{
  while ( 0 == (USART1->ISR & USART_ISR_TXE) ) {}
  USART1->TDR = c;
}
