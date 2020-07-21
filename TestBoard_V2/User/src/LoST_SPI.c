#include "LoST_SPI.h"

//MISO : PB7, MOSI : PB6, SCK : PB5
void initSPI1(void){
 SPI_DeInit(SPI1);
  CLK_PeripheralClockConfig(CLK_Peripheral_SPI1, ENABLE);
  GPIO_Init(GPIOB, GPIO_Pin_5, GPIO_Mode_Out_PP_High_Fast);
  GPIO_Init(GPIOB, GPIO_Pin_6, GPIO_Mode_Out_PP_High_Fast);
  GPIO_Init(GPIOB, GPIO_Pin_7, GPIO_Mode_In_FL_No_IT);
  ///init spi st lib
  SPI_Init(SPI1, SPI_FirstBit_MSB, SPI_BaudRatePrescaler_2, SPI_Mode_Master, SPI_CPOL_Low, SPI_CPHA_1Edge, SPI_Direction_2Lines_FullDuplex, SPI_NSS_Soft, 1);
  SPI_NSSInternalSoftwareCmd(SPI1, ENABLE);
  SPI_Cmd(SPI1, ENABLE);
};

uint8_t SPI_Transfer(uint8_t data)
{
  SPI_SendData(SPI1, data);
  //while(IS_SPI_FLAG(SPI1->SR) == SPI_FLAG_BSY);
  while(!SPI_GetFlagStatus(SPI1, SPI_FLAG_RXNE));
  return SPI_ReceiveData(SPI1);
};