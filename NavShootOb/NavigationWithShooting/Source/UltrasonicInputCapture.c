
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

#include "UltrasonicInputCapture.h"
#include "RobotSM.h"


#include "ES_Configure.h"
#include "ES_Framework.h"


#define ALL_BITS (0xff<<2)

// 40,000 ticks per mS assumes a 40Mhz clock
#define TicksPerMS 40000

// we will use Timer B in Wide Timer 3 to capture the input

void InitUltraInputCapture( void ){

  // start by enabling the clock to the timer (Wide Timer 2 )
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
	
	// enable the clock to Port D
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;

	//checking timer clock 
  while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R3) != SYSCTL_PRWTIMER_R3)
    ;
  // make sure that timer (Timer B) is disabled before configuring
  HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
  
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
  HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

// we want to use the full 32 bit count, so initialize the Interval Load
// register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER3_BASE+TIMER_O_TBILR) = 0xffffffff;

// we don't want any prescaler (it is unnecessary with a 32 bit count)
//  HWREG(WTIMER0_BASE+TIMER_O_TAPR) = 0;

// set up timer B in capture mode (TBMR=3, TBAMS = 0), 
// for edge time (TBCMR = 1) and up-counting (TBCDIR = 1)
  HWREG(WTIMER3_BASE+TIMER_O_TBMR) = 
      (HWREG(WTIMER3_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) | 
        (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);

// To set the event to rising edge, we need to modify the TAEVENT bits 
// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
  HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEVENT_M;
	
	 while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != SYSCTL_PRGPIO_R3)
    ;

// Now Set up the port to do the capture (clock was enabled earlier)
// start by setting the alternate function for Port D bit 3 (WT3CCP1)
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT3HI;

// Then, map bit 4's alternate function to WT2CCP1
// 7 is the mux value to select WT3CCP1, 12 to shift it over to the
// right nibble for bit 3 (4 bits/nibble * 3 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = 
    (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xffff0fff) + (7<<12);

// Enable pin 3 on Port D for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT3HI;
	
// make pin3 on Port D into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT3LO;

// back to the timer to enable a local capture interrupt
  HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_CBEIM;

// enable the Timer B in Wide Timer 3 interrupt in the NVIC
// it is interrupt number 101 so appears in EN2 at bit 30
  HWREG(NVIC_EN3) |= BIT5HI;
	
	//priority for WT3CCP1 as 1 
	//HWREG(NVIC_PRI25)|= BIT13HI;
	

// make sure interrupts are enabled globally
  __enable_irq();
	//	HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
		printf("Ultrasound Initialised\n\r");
		
}

void UltraInputCaptureResponse( void ){
// start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_CBECINT;
	

	
ES_Event NewEvent;
		NewEvent.EventType = UltraSoundInt;
		PostRobotSM(NewEvent);
//	
	//ES_Timer_InitTimer(TEMP_TIMER,10);

	HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
	
}



