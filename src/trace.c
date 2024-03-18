#include "trace.h"
#include "stm32f103x6.h"

//------------------------------------------------------------------------------
/// Outputs a character on the DBGU line.
/// \note This function is synchronous (i.e. uses polling).
/// \param c  Character to send.
//------------------------------------------------------------------------------
void DBGU_PutChar(unsigned char c)
{
  while ( 0 == (USART1->SR & USART_SR_TXE) ) {}
  USART1->DR = c;
}
