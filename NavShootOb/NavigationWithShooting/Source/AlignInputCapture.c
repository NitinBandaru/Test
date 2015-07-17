
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "bitdefs.h"
#include "driverlib/gpio.h"

#include "AlignInputCapture.h"

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PWMModule.h"
#include "RobotSM.h"

#define ALL_BITS (0xff<<2)

// 40,000 ticks per mS assumes a 40Mhz clock
#define TicksPerMS 40000

// we will use Timer B in Wide Timer 2 to capture the input

void InitAlignInputCapture( void ){

  // start by enabling the clock to the timer (Wide Timer 2 )
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R2;
	
	// enable the clock to Port D
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;

	//checking timer clock 
  while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R2) != SYSCTL_PRWTIMER_R2)
    ;
  // make sure that timer (Timer B) is disabled before configuring
  HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
  
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
  HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

// we want to use the full 32 bit count, so initialize the Interval Load
// register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER2_BASE+TIMER_O_TBILR) = 0xffffffff;

// we don't want any prescaler (it is unnecessary with a 32 bit count)
//  HWREG(WTIMER0_BASE+TIMER_O_TAPR) = 0;

// set up timer B in capture mode (TBMR=3, TBAMS = 0), 
// for edge time (TBCMR = 1) and up-counting (TBCDIR = 1)
  HWREG(WTIMER2_BASE+TIMER_O_TBMR) = 
      (HWREG(WTIMER2_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) | 
        (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);

// To set the event to rising edge, we need to modify the TAEVENT bits 
// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
  HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEVENT_M;
	
	 while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != SYSCTL_PRGPIO_R3)
    ;

// Now Set up the port to do the capture (clock was enabled earlier)
// start by setting the alternate function for Port D bit 1 (WT2CCP1)
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT1HI;

// Then, map bit 4's alternate function to WT2CCP1
// 7 is the mux value to select WT2CCP1, 4 to shift it over to the
// right nibble for bit 1 (4 bits/nibble * 1 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = 
    (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xffffff0f) + (7<<4);

// Enable pin 1 on Port D for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT1HI;
	
// make pin 1 on Port D into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT1LO;

// back to the timer to enable a local capture interrupt
  HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_CBEIM;

// enable the Timer B in Wide Timer 2 interrupt in the NVIC
// it is interrupt number 99 so appears in EN2 at bit 30
  HWREG(NVIC_EN3) |= BIT3HI;
	
	//priority for WT2CCP1 as 1 
	HWREG(NVIC_PRI24)&= ~(BIT29HI|BIT30HI|BIT31HI);
	

// make sure interrupts are enabled globally
  __enable_irq();
	
	//HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
	
	printf("Align Input Capture Initialized\n\r");
}

void AlignInputCaptureResponse( void ){
// start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_CBECINT;
	
	HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN; //Stop Periodic Interrupt
	HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN; //Stop Input capture 1 Interrupt
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN; //Stop Input capture 2 Interrupt
	SetPWMDuty1(0); 
	SetPWMDuty2(0);
	
		ES_Event NewEvent;
		NewEvent.EventType = AlignComplete;
		PostRobotSM(NewEvent);

	//ES_Timer_InitTimer(TEMP_TIMER,10);

	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;//Stop Align input capture interrupt
	
}



