#include "LoST_EXT.h"

void initExternal_Interrupt_DIO1(void)
{
  EXTI_DeInit();
  EXTI_SelectPort(EXTI_Port_B);
  EXTI_SetPinSensitivity(EXTI_Pin_1, EXTI_Trigger_Rising_Falling);
}