#include "stm8l15x.h"
#include "stm8l15x_conf.h"
#include "Radio.h"
//#include "FreqLUIT.h"
#define IS_MASTER 1U

#define RF_FREQUENCY                                2400000000UL// Hz
#define TX_OUTPUT_POWER                             13U// dBm
#define RX_TIMEOUT_TICK_SIZE                        RADIO_TICK_SIZE_1000_US
#define RX_TIMEOUT_VALUE                             1000U // ms
#define TX_TIMEOUT_VALUE                             10000U // ms
#define BUFFER_SIZE                                  255U
#define NO_OF_RANGING                               1U
#define PINGPONGSIZE                                4U
const uint32_t rangingAddress[] = {
  0x10000000,
  0x32100000,
  0x20012301,
  0x20000abc,
  0x32101230
};
#define RANGING_ADDRESS_SIZE    5U
#define SLAVE_ADDRESS_START     2U
#define SLAVE_ADDRESS_END       2U
int masterID               =    0U;
int rangingAddressIndex    =    2U;
uint32_t RangingCalib = 13376U;// Bandwith 1600, SF10   377577

const uint16_t RNG_CALIB_0400[] = { 10299,  10271,  10244,  10242,  10230,  10246  };
const uint16_t RNG_CALIB_0800[] = { 11486,  11474,  11453,  11426,  11417,  11401  };
const uint16_t RNG_CALIB_1600[] = { 13308,  13493,  13528,  13515,  13430,  13376};//13376
const double   RNG_FGRAD_0400[] = { -0.148, -0.214, -0.419, -0.853, -1.686, -3.423 };
const double   RNG_FGRAD_0800[] = { -0.041, -0.811, -0.218, -0.429, -0.853, -1.737 };
const double   RNG_FGRAD_1600[] = { 0.103,  -0.041, -0.101, -0.211, -0.424, -0.87  };
const double   RNG_RATIO_1600 = 0.000000023;
const char* IrqRangingCodeName[] = {
  "IRQ_RANGING_SLAVE_ERROR_CODE",
  "IRQ_RANGING_SLAVE_VALID_CODE",
  "IRQ_RANGING_MASTER_ERROR_CODE",
  "IRQ_RANGING_MASTER_VALID_CODE"
};


typedef enum
{
  APP_IDLE,
  APP_RX,
  APP_RX_TIMEOUT,
  APP_RX_ERROR,
  APP_TX,
  APP_TX_TIMEOUT,
  APP_RX_SYNC_WORD,
  APP_RX_HEADER,
  APP_RANGING,
  APP_CAD
} AppStates_t;

void txDoneIRQ( void );
void rxDoneIRQ( void );
void rxSyncWordDoneIRQ( void );
void rxHeaderDoneIRQ( void );
void txTimeoutIRQ( void );
void rxTimeoutIRQ( void );
void rxErrorIRQ( IrqErrorCode_t errCode );
void rangingDoneIRQ( IrqRangingCode_t val );
void cadDoneIRQ( bool cadFlag );
void getStatusIRQ(uint32_t regs);
RadioCallbacks_t Callbacks = {
  txDoneIRQ,
  rxDoneIRQ,
  rxSyncWordDoneIRQ,
  rxHeaderDoneIRQ,
  txTimeoutIRQ,
  rxTimeoutIRQ,
  rxErrorIRQ,
  rangingDoneIRQ,
  cadDoneIRQ,
  getStatusIRQ
};

extern const Radio_t Radio;

//uint16_t RxIrqMask = IRQ_RANGING_MASTER_RESULT_VALID | IRQ_RX_TX_TIMEOUT;
//uint16_t TxIrqMask = IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT;
uint16_t masterIrqMask = IRQ_RANGING_MASTER_RESULT_VALID | IRQ_RANGING_MASTER_TIMEOUT;
uint16_t slaveIrqMask = IRQ_RANGING_SLAVE_RESPONSE_DONE | IRQ_RANGING_SLAVE_REQUEST_DISCARDED;


PacketParams_t packetParams;
PacketStatus_t packetStatus;
ModulationParams_t modulationParams;

volatile AppStates_t AppState = APP_IDLE;
IrqRangingCode_t MasterIrqRangingCode = IRQ_RANGING_MASTER_ERROR_CODE;

uint8_t Buffer[BUFFER_SIZE];
uint8_t BufferSize = BUFFER_SIZE;
void Master_Init(int rangingIndex);
void Slave_Init(int rangingIndex);
void RangingPacketInit(int rangingIndex);
void RangRingFunc();
void main() {
 //Serial.begin(115200);
  Radio.Init(&Callbacks);
  // Uart_SendData8String("SX1280 MASTER");
  if (IS_MASTER)
  {
    Uart_SendData8String(" SX1280 MASTER");
  }
  else
  {
   Uart_SendData8String("SX1280 SLAVE");
  }
  
  Radio.SetRegulatorMode( USE_DCDC ); // Can also be set in LDO mode but consume more power
  Uart_SendData8String( "\n\n\r     SX1280 Ranging Application. \n\n\r");
  //if IS_MASTER getSlaves(House_ID)
  //else getAddress(Slave_ID)
  RangingPacketInit(rangingAddressIndex);
  //rangingDoneIRQ(IRQ_RANGING_MASTER_VALID_CODE);

  while(1)
  {
    RangRingFunc();
    
  }
}

void txDoneIRQ( void )
{
  AppState = APP_TX;
  Uart_SendData8String("Sent\n");
}

void rxDoneIRQ( void )
{
  AppState = APP_RX;
}

void rxSyncWordDoneIRQ( void )
{
   AppState = APP_RX_SYNC_WORD;
}

void rxHeaderDoneIRQ( void )
{
   AppState = APP_RX_HEADER;
}

void txTimeoutIRQ( void )
{
  AppState = APP_TX_TIMEOUT;
}

void rxTimeoutIRQ( void )
{
  AppState = APP_RX_TIMEOUT;
}

void rxErrorIRQ( IrqErrorCode_t errCode )
{
  AppState = APP_RX_ERROR;
}

void rangingDoneIRQ( IrqRangingCode_t val )
{
  AppState = APP_RANGING;
  MasterIrqRangingCode = val;
}

void cadDoneIRQ( bool cadFlag )
{
  AppState = APP_CAD;
}
void getStatusIRQ(uint32_t regs)
{
  Uart_SendNumber(regs);
}
void RangingPacketInit(int rangingIndex)
{
  modulationParams.PacketType = PACKET_TYPE_RANGING;
  modulationParams.Params.LoRa.SpreadingFactor = LORA_SF10;
  modulationParams.Params.LoRa.Bandwidth = LORA_BW_1600;
  modulationParams.Params.LoRa.CodingRate = LORA_CR_LI_4_5;

  packetParams.PacketType = PACKET_TYPE_RANGING;
  packetParams.Params.LoRa.PreambleLength = 12;
  packetParams.Params.LoRa.HeaderType = LORA_PACKET_VARIABLE_LENGTH;
  packetParams.Params.LoRa.PayloadLength = 7;
  packetParams.Params.LoRa.Crc = LORA_CRC_ON;
  packetParams.Params.LoRa.InvertIQ = LORA_IQ_NORMAL;

  Radio.SetStandby( STDBY_RC );
  Radio.SetPacketType( modulationParams.PacketType );
  Radio.SetModulationParams( &modulationParams );
  Radio.SetPacketParams( &packetParams );
  Radio.SetRfFrequency( 2400000000UL);
  Radio.SetTxParams( TX_OUTPUT_POWER, RADIO_RAMP_20_US );
  Radio.SetBufferBaseAddresses( 0x00, 0x00 );
  Radio.SetRangingCalibration( RangingCalib );
  Radio.SetInterruptMode();
  Uart_SendData8String("Init");
  if (IS_MASTER)
  {
    Master_Init(rangingIndex);
  }
  else // SLAVE
  {
    Slave_Init(rangingIndex);
  }
}

void Slave_Init(int rangingIndex)
{
  Uart_SendData8String("Slave Role");
  Radio.SetRangingIdLength(RANGING_IDCHECK_LENGTH_32_BITS);
  Radio.SetDeviceRangingAddress(rangingAddress[rangingIndex]);
  Radio.SetDioIrqParams( slaveIrqMask, slaveIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  Radio.SetRx((TickTime_t) {
    RADIO_TICK_SIZE_1000_US, 0xFFFF
  });
}
void Master_Init(int rangingIndex)
{
  Radio.SetRangingRequestAddress(rangingAddress[rangingIndex]);
  Radio.SetDioIrqParams( masterIrqMask, masterIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  Radio.SetTx((TickTime_t) {
    RADIO_TICK_SIZE_1000_US, 0xFFFF
  });
}

void RangRingFunc(){
   switch (AppState)
  {
    //digitalWrite(10, LOW);
    case APP_IDLE:
      break;
    case APP_RANGING:
      AppState = APP_IDLE;
      if (IS_MASTER)
      {
        //rangingDoneIRQ(IRQ_RANGING_MASTER_VALID_CODE);
        switch (MasterIrqRangingCode)
        {
          case IRQ_RANGING_MASTER_VALID_CODE:
          {
            double rangingResult = Radio.GetRangingResult(RANGING_RESULT_RAW);
            Uart_SendData8String("\nRaw data: \n");
            Uart_SendNumber((u32)rangingResult);
            uint32_t result =(u32) rangingResult * 100;
            //Send distance
            uint32_t  Payload[4] = {(result >> 24) & 0xFF, (result >> 16) & 0xFF, (result >> 8) & 0xFF, (result) & 0xFF};
             Uart_SendData8String("\nRanging result is : \n");
             Uart_SendNumber((Payload[0] + Payload[1] + Payload[2] + Payload[3]) / 10.0);
          };
          break;
          case IRQ_RANGING_MASTER_ERROR_CODE:
          {
            //Serial.println("ERROR");
            Uart_SendData8String("ERROR\n");
          };
          break;
          default:
          {};
            break;
        }
        rangingAddressIndex = rangingAddressIndex + 1 > SLAVE_ADDRESS_END ? SLAVE_ADDRESS_START : rangingAddressIndex + 1;
        Master_Init(rangingAddressIndex);
      }
      else
      {
        Slave_Init(rangingAddressIndex);
      }
      break;
    default:
      AppState = APP_IDLE;
      break;
  }
  delay(50000);
  delay(50000);
  delay(50000);
  delay(50000);
}