
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "bitdefs.h"

#include "InputCapture2.h"


#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ControlService.h"

// 40,000 ticks per mS assumes a 40Mhz clock
#define TicksPerMS 40000

static uint32_t Period2;
static uint32_t LastCapture2 = 0;


// we will use Timer A in Wide Timer 2 to capture the input

void InitInputCapturePeriod2( void ){

  // start by enabling the clock to the timer (Wide Timer 2 )
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R2;
	
	// enable the clock to Port D
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;

	//checking timer clock 
  while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R2) != SYSCTL_PRWTIMER_R2)
    ;
  // make sure that timer (Timer A) is disabled before configuring
  HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
  
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
  HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

// we want to use the full 32 bit count, so initialize the Interval Load
// register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER2_BASE+TIMER_O_TAILR) = 0xffffffff;

// we don't want any prescaler (it is unnecessary with a 32 bit count)
//  HWREG(WTIMER0_BASE+TIMER_O_TAPR) = 0;

// set up timer A in capture mode (TAMR=3, TAAMS = 0), 
// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
  HWREG(WTIMER2_BASE+TIMER_O_TAMR) = 
      (HWREG(WTIMER2_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | 
        (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);

// To set the event to rising edge, we need to modify the TAEVENT bits 
// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
  HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
	
	 while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != SYSCTL_PRGPIO_R3)
    ;

// Now Set up the port to do the capture (clock was enabled earlier)
// start by setting the alternate function for Port D bit 0 (WT2CCP0)
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT0HI;

// Then, map bit 4's alternate function to WT0CCP0
// 7 is the mux value to select WT2CCP0, 0 to shift it over to the
// right nibble for bit 0 (4 bits/nibble * 4 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = 
    (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffffff0) + (7);

// Enable pin 0 on Port D for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT0HI;
	
// make pin 0 on Port D into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT0LO;

// back to the timer to enable a local capture interrupt
  HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;

// enable the Timer A in Wide Timer 2 interrupt in the NVIC
// it is interrupt number 98 so appears in EN2 at bit 30
  HWREG(NVIC_EN3) |= BIT2HI;
	
	//Assigning priority 1
	HWREG(NVIC_PRI24)|= BIT21HI;
	

// make sure interrupts are enabled globally
  __enable_irq();


// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
  HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
	
		printf("Input Capture 2 Initialized\n\r");

}

void InputCaptureResponse2( void ){
  uint32_t ThisCapture2;
  
// start by clearing the source of the interrupt, the input capture event
    HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
// now grab the captured value and calculate the period
    ThisCapture2 = HWREG(WTIMER2_BASE+TIMER_O_TAR);
    Period2 = ThisCapture2 - LastCapture2;
    
// update LastCapture to prepare for the next edge  
    LastCapture2 = ThisCapture2;
	
	//set flagstate in AD service so as to know when input signal stops and resume LED update when inoput starts again
		SetFlag2(true);
		ES_Timer_InitTimer(NOPULSE2_TIMER,500); //200 ms timer that expires only when input signal stops
}


//Public fuction to see the period of the caught input
uint32_t GetPeriod2( void ){
	
  return Period2;
}





