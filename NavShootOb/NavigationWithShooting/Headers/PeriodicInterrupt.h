#ifndef PERIODIC_H
#define PERIODIC_H

#include <stdint.h>

void InitPeriodicInt( void );
void PeriodicIntResponse( void );
uint32_t GetRequestedDuty1 (void);
uint32_t GetTargetRPM1(void);
uint32_t GetRequestedDuty2 (void);
uint32_t GetRPM2 (void);
void ResetSumError (void);
void ResetLastPeriod (void);
#endif 
