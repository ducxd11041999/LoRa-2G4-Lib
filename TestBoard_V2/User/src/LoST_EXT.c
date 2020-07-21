#include "LoST_EXT.h"

void initExternal_Interrupt_DIO1(void)
{
  EXTI_DeInit();
  EXTI_SelectPort(EXTI_Port_D);
  EXTI_SetPinSensitivity(EXTI_Pin_0, EXTI_Trigger_Rising_Falling);
}