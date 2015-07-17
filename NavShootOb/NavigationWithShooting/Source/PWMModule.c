#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_pwm.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "bitdefs.h"

#include "PWMModule.h"


// 40,000 ticks per mS assumes a 40Mhz clock, we will use SysClk/32 for PWM
#define PWMTicksPerMS 40000/32

// set 1000 Hz frequency so 1mS period
#define PeriodInMS 1



#define BitsPerNibble 4

// we will use PWM module 0 and program it for up/down counting

void InitPWM( void ){
  volatile uint32_t Dummy; // use volatile to avoid over-optimization
  
// start by enabling the clock to the PWM Module (PWM0)
  HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;

// enable the clock to Port B  
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;

// Select the PWM clock as System Clock/32
  HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
    (SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
  
// make sure that the PWM module clock has gotten going
	while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
    ;

// disable the PWM while initializing
  HWREG( PWM0_BASE+PWM_O_0_CTL ) = 0;

// program generator A to go to 0 at rising compare A, 1 on falling compare A  
  HWREG( PWM0_BASE+PWM_O_0_GENA) = 
    (PWM_0_GENA_ACTCMPAU_ZERO | PWM_0_GENA_ACTCMPAD_ONE );
  
// program generator B to go to 0 at rising compare B, 1 on falling compare B  
  HWREG( PWM0_BASE+PWM_O_0_GENB) = 
    (PWM_0_GENB_ACTCMPBU_ZERO | PWM_0_GENB_ACTCMPBD_ONE );

// Set the PWM period. Since we are counting both up & down, we initialize
// the load register to 1/2 the desired total period
  HWREG( PWM0_BASE+PWM_O_0_LOAD) = (((PeriodInMS)* PWMTicksPerMS))>>1;
  
// Set the initial Duty cycle on A to 0% by programming the compare value
// to 1/2 the period to count up (or down) 
  HWREG( PWM0_BASE+PWM_O_0_CMPA) = 0;  //(((PeriodInMS) * PWMTicksPerMS))/8;
	
	// Set the initial Duty cycle on B to 0% by programming the compare value
// to 1/4 the period to count up (or down) 
  HWREG( PWM0_BASE+PWM_O_0_CMPB) = 0; //(((PeriodInMS) * PWMTicksPerMS))/8;

// set changes to the PWM output Enables to be locally synchronized to a 
// zero count
  HWREG(PWM0_BASE+PWM_O_ENUPD) =  (HWREG(PWM0_BASE+PWM_O_ENUPD) & 
      ~(PWM_ENUPD_ENUPD0_M | PWM_ENUPD_ENUPD1_M)) |
      (PWM_ENUPD_ENUPD0_LSYNC | PWM_ENUPD_ENUPD1_LSYNC);

// enable the PWM outputs
  HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM1EN | PWM_ENABLE_PWM0EN);

	while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1)
    ;
// now configure the Port B pin to be PWM outputs
// start by selecting the alternate function for PB6 & 7
  HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT7HI | BIT6HI);

// now choose to map PWM to those pins, this is a mux value of 4 that we
// want to use for specifying the function on bits 6
  HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) = 
    (HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) & 0x00fffff) + (4<<(7*BitsPerNibble)) + (4<<(6*BitsPerNibble));

// Enable pins 6 & 7 on Port B for digital I/O
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT7HI | BIT6HI);
	
// make pin 6 & 7 on Port B into outputs
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT7HI | BIT6HI);;
  
// set the up/down count mode and enable the PWM generator
  HWREG(PWM0_BASE+ PWM_O_0_CTL) |= (PWM_0_CTL_MODE | PWM_0_CTL_ENABLE);
	
	printf("PWM Initialized\n\r");

}


void SetPWMDuty1(uint8_t DutyCycle)
  {

	if(DutyCycle == 0){
		// Set the Duty cycle on A to 0% by programming the compare value
		// to 0. However, since the CmpADn action (set to one) wins, we also
		// need to invert the output  
 		HWREG( PWM0_BASE+PWM_O_0_CMPA) = 0;
 		 HWREG( PWM0_BASE+PWM_O_ENABLE) &= ~PWM_ENABLE_PWM0EN;
	}

	else 
	{
		if (HWREG( PWM0_BASE+PWM_O_0_CMPA) == 0) //if the previous value was zero, we need to enable PWM output again
		{
			HWREG( PWM0_BASE+PWM_O_ENABLE) |= ( PWM_ENABLE_PWM0EN);
		}

		HWREG( PWM0_BASE+PWM_O_0_CMPA) = (PeriodInMS * PWMTicksPerMS)*DutyCycle/200; //Set the Comparator A value for New duty
  }

}


void SetPWMDuty2( uint8_t NewDuty )
{
	if (NewDuty==0) //disable PWM output for 0 duty cycle
	{
		HWREG( PWM0_BASE+PWM_O_0_CMPB) = 0;
		HWREG( PWM0_BASE+PWM_O_ENABLE) &= ~PWM_ENABLE_PWM1EN;
	}
	else 
	{
		if (HWREG( PWM0_BASE+PWM_O_0_CMPB) == 0) //if the previous value was zero, we need to enable PWM output again
		{
			HWREG( PWM0_BASE+PWM_O_ENABLE) |= ( PWM_ENABLE_PWM1EN);
		}

		HWREG( PWM0_BASE+PWM_O_0_CMPB) = (PeriodInMS * PWMTicksPerMS)*NewDuty/200; //Set the Comparator B value for New duty
	}
}
