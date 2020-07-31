#include "UART.h"
#include "stdio.h"
extern void delay(uint32_t a);

void Init_Uart()
{
  GPIO_ExternalPullUpConfig(GPIOC, GPIO_Pin_3|GPIO_Pin_2, ENABLE);
  
  USART_DeInit(USART1);
  CLK_PeripheralClockConfig(CLK_Peripheral_USART1, ENABLE);
  USART_Init(USART1, 9600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No,  USART_Mode_RxTx);
  USART_ClockInit(USART1, USART_Clock_Disable, USART_CPOL_Low, USART_CPHA_1Edge, USART_LastBit_Disable);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  USART_Cmd(USART1, ENABLE);        
  enableInterrupts();
}
void Uart_SendData8String(char Str[])
{
  //char str[]="HuuDuc";
  int i=0;
  while(Str[i]!='\0')
  {
    USART_SendData8(USART1, Str[i]);
    i++;
    delay(600);
  }
}
#define PUTCHAR_PROTOPYE char __putchar(char c)

PUTCHAR_PROTOPYE
{
  USART_SendData8(USART1, c);
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET);
  return (c);
}


void Uart_SendNumber(float x)
{ 
  char value [10];
  int i=0;
  int n=12;

  n=sprintf (value, "%f",x); //after decimal point 3 numbers of zero
      while(i<n) 
  {
    USART_SendData8(USART1, value[i++]);
    delay(5000);
  }
    
}