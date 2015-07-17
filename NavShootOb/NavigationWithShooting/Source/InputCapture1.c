
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "bitdefs.h"

#include "InputCapture1.h"
#include "ControlService.h"

#include "ES_Configure.h"
#include "ES_Framework.h"

// 40,000 ticks per mS assumes a 40Mhz clock
#define TicksPerMS 40000

static uint32_t Period1;
static uint32_t LastCapture1 = 0;


// we will use Timer A in Wide Timer 3 to capture the input

void InitInputCapturePeriod1( void ){

  // start by enabling the clock to the timer (Wide Timer 0)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
	// enable the clock to Port D
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
		//checking timer clock 
  while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R3) != SYSCTL_PRWTIMER_R3)
		;
  
  // make sure that timer (Timer A) is disabled before configuring
  HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
  
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
  HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

// we want to use the full 32 bit count, so initialize the Interval Load
// register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER3_BASE+TIMER_O_TAILR) = 0xffffffff;

// we don't want any prescaler (it is unnecessary with a 32 bit count)
//  HWREG(WTIMER0_BASE+TIMER_O_TAPR) = 0;

// set up timer A in capture mode (TAMR=3, TAAMS = 0), 
// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
  HWREG(WTIMER3_BASE+TIMER_O_TAMR) = 
      (HWREG(WTIMER3_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | 
        (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);

// To set the event to rising edge, we need to modify the TAEVENT bits 
// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
  HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;


	//checking GPIO Periferral ready 
		 while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != SYSCTL_PRGPIO_R3)
    ;
// start by setting the alternate function for Port D bit 2 (WT3CCP0)
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT2HI;

// Then, map bit 2's alternate function to WT0CCP0
// 7 is the mux value to select WT3CCP0, 8 to shift it over to the
// right nibble for bit 2 (4 bits/nibble * 2 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = 
    (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffff0ff) + (7<<8);

// Enable pin 2 on Port D for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT2HI;
	
// make pin 2 on Port D into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT2LO;

// back to the timer to enable a local capture interrupt
  HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;

// enable the Timer A in Wide Timer 3 interrupt in the NVIC
// it is interrupt number 100 so appears in EN2 at bit 30
  HWREG(NVIC_EN3) |= BIT4HI;

//Assigning priority 1
HWREG(NVIC_PRI25) |= BIT5HI;
// make sure interrupts are enabled globally
  __enable_irq();


// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
  HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
	
	printf("Input Capture 1 Initialized\n\r");

}

void InputCaptureResponse1( void ){
  uint32_t ThisCapture1;
  
// start by clearing the source of the interrupt, the input capture event
    HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
// now grab the captured value and calculate the period
    ThisCapture1 = HWREG(WTIMER3_BASE+TIMER_O_TAR);
    Period1 = ThisCapture1 - LastCapture1;
	
    
// update LastCapture to prepare for the next edge  
    LastCapture1 = ThisCapture1;
	
	//set flagstate in AD service so as to know when input signal stops and resume LED update when inoput starts again
		SetFlag1(true);
		ES_Timer_InitTimer(NOPULSE1_TIMER,500); //200 ms timer that expires only when input signal stops
}


//Public fuction to see the period of the caught input
uint32_t GetPeriod1( void ){
	
  return Period1;
}

