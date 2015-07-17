#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void InitPWM( void );
void SetPWMDuty1(uint8_t DutyCycle);
void SetPWMDuty2(uint8_t NewDuty );

#endif
