#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "bitdefs.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"

#include "PeriodicInterrupt.h"
#include "InputCapture1.h"
#include "InputCapture2.h"
#include "PWMModule.h"
#include "NavigationSM.h"


#include "ES_Configure.h"
#include "ES_Framework.h"

// 40,000 ticks per mS assumes a 40Mhz clock
#define TicksPerMS 40000
static float pGain1 = 1, pGain2 = 1;
static float iGain1 =0.1, iGain2 = 0.1;
static uint32_t RPM1, RPM2;
static float	RequestedDuty1, RequestedDuty2;
#define ALL_BITS (0xff<<2)
static float TargetRPM1;
static float SumError1=0, SumError2=0; //error integrator
static unsigned long LastPeriod1=1000; //Tae
static unsigned long LastPeriod2=1000; 		// Tae

// we will use Timer B in Wide Timer 0 to generate the interrupt

void InitPeriodicInt( void ){
  volatile uint32_t Dummy; // use volatile to avoid over-optimization
  
  // start by enabling the clock to the timer (Wide Timer 0)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
	// kill a few cycles to let the clock get going
	Dummy = HWREG(SYSCTL_RCGCGPIO);
  
  // make sure that timer (Timer B) is disabled before configuring
  HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
  
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
  HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
  
// set up timer B in periodic mode so that it repeats the time-outs
  HWREG(WTIMER0_BASE+TIMER_O_TBMR) = 
     (HWREG(WTIMER0_BASE+TIMER_O_TBMR)& ~TIMER_TBMR_TBMR_M)| 
     TIMER_TBMR_TBMR_PERIOD;

// set timeout to 20mS
  HWREG(WTIMER0_BASE+TIMER_O_TBILR) = TicksPerMS * 20;

// enable a local timeout interrupt
  HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;

// enable the Timer B in Wide Timer 0 interrupt in the NVIC
// it is interrupt number 95 so apppears in EN2 at bit 30
  HWREG(NVIC_EN2) = BIT31HI;
  
  //Setting priority 2 
  HWREG(NVIC_PRI23)|= BIT30HI;

// make sure interrupts are enabled globally
  __enable_irq();
  
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
  HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
	
		printf("Periodic Interrupt Initialized\n\r");

}

void PeriodicIntResponse( void ){
	
	static float RPMError1, RPMError2;  //static for speed
	static float  TargetRPM2;	//static for speed
	static unsigned long ThisPeriod1, ThisPeriod2;  //static for speed
	

	
// start by clearing the source of the interrupt
    HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
	
	ThisPeriod1 = GetPeriod1();
	ThisPeriod2 = GetPeriod2();
	
	if(LastPeriod1 == ThisPeriod1)	// Tae
		RPM1 = 0;
	else
		RPM1 = (16000000/(ThisPeriod1));
	
	
	if(LastPeriod2 == ThisPeriod2)	//Tae
		RPM2 = 0;
	else
		RPM2 = (16000000/(ThisPeriod2));
	
	LastPeriod1 = ThisPeriod1;
	LastPeriod2 = ThisPeriod2;
	
	
	
//	RPM1 = (16000000/(ThisPeriod1));
//	RPM2 = (16000000/(ThisPeriod2));
//	
	
	TargetRPM1 = GetTargetRPM();
	TargetRPM2 = GetTargetRPM();
	
  RPMError1 = (TargetRPM1 - RPM1);
	RPMError2 = (TargetRPM2 - RPM2);
	SumError1 +=RPMError1;
	SumError2 +=RPMError2;
	RequestedDuty1= (pGain1*((RPMError1) + (iGain1*SumError1))); //PI control law
	RequestedDuty2= (pGain2*((RPMError2) + (iGain2*SumError2))); //PI control law
	
	if (RequestedDuty1>100)
	{
		RequestedDuty1=100;		//anti windup
		SumError1-=RPMError1;
	}
	else if (RequestedDuty1<0)
	{
		RequestedDuty1 = 0;		//anti windup
		SumError1-=RPMError1;
	}
	
	if (RequestedDuty2>100)
	{
		RequestedDuty2=100;		//anti windup
		SumError2-=RPMError2;
	}
	else if (RequestedDuty2<0)
	{
		RequestedDuty2 = 0;		//anti windup
		SumError2-=RPMError2;
	}
	
	SetPWMDuty1(RequestedDuty1);	
	SetPWMDuty2(RequestedDuty2);	

	
}

//returns the RPM of the motor
uint32_t GetTargetRPM1 (void)
{
	return TargetRPM1;
}

//returns the control effort calcualted from the control law
uint32_t GetRequestedDuty1 (void)
{
	return RequestedDuty1;
}

uint32_t GetRPM2 (void)
{
	return RPM2;
}

//returns the control effort calcualted from the control law
uint32_t GetRequestedDuty2 (void)
{
	return RequestedDuty2;
}

void ResetSumError (void) 
{
	SumError1  = 0;
	SumError2  = 0;
	return;
}

void ResetLastPeriod (void) 
{
	LastPeriod1=1000;
	LastPeriod2=1000;
	return;
}
