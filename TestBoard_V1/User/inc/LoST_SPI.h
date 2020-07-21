#include "stm8l15x_spi.h"
void initSPI1(void); //MISO : PB7, MOSI : PB6, SCK : PB5
uint8_t SPI_Transfer(uint8_t data);