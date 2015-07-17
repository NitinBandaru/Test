#include <stdint.h>
#include <stdbool.h>

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
#include "driverlib/ssi.h"
#include "inc/hw_ssi.h"

// Event Definitions

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"

#include "SpiModule.h"
#include "CommunicationSM.h"


#define BitsPerNibble 4

static uint32_t command;  //variable that can be read from other modules

void InitSPI (void) 
{
//Enable the clock to the GPIO port portA pins 2 to 4 to be used
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0;
	
//Enable the clock to SSI module
	HWREG(SYSCTL_RCGCSSI) |= SYSCTL_RCGCSSI_R0;
	
//Wait for the GPIO port to be ready
	while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0)
    ;
	
//Program the GPIO to use the alternate functions on the SSI pins
	HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= ((BIT2HI)|(BIT3HI)|(BIT4HI)|(BIT5HI));
		
//Select the SSI alternate functions on those pins, this is a mux value of 2 that we
	HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) = 
    (HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) & 0xff0000ff) + (2<<(2*BitsPerNibble)) +
      (2<<(3*BitsPerNibble)) + (2<<(4*BitsPerNibble)) + (2<<(5*BitsPerNibble));
			
//Program the port lines for digital I/O
	HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= ((BIT2HI)|(BIT3HI)|(BIT4HI)|(BIT5HI));
	
//Program the required data directions on the port lines
	HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= ((BIT2HI)|(BIT3HI)|(BIT5HI));
	HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) &= ~(BIT4HI);
	
//If using SPI mode 3, program the pull-up on the clock line
	HWREG(GPIO_PORTA_BASE+GPIO_O_PUR)|= (BIT2HI);
	
//OpenDrain Ouput for SS line
	HWREG(GPIO_PORTA_BASE+GPIO_O_ODR)|= (BIT3HI);
	
//Wait for the SSI0 to be ready
	while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0) != SYSCTL_PRSSI_R0)
    ;
	
//Make sure the the SSI is disabled before programming mode bits
	HWREG(SSI0_BASE+ SSI_O_CR1) &= ~( SSI_CR1_SSE );
	
//Select master mode (MS) & TXRIS indicating End of Transmit (EOT)
		HWREG(SSI0_BASE+ SSI_O_CR1) = ((HWREG(SSI0_BASE+ SSI_O_CR1)&(~SSI_CR1_MS))|(SSI_CR1_EOT));
		
//Configure the SSI clock source to the system clock
	HWREG(SSI0_BASE+ SSI_O_CC)  =  ((HWREG(SSI0_BASE+ SSI_O_CC)&~(SSI_CC_CS_M))|SSI_CC_CS_SYSPLL);
	
//CPSR must be even number, giving value 50
	HWREG(SSI0_BASE + SSI_O_CPSR) = ((HWREG(SSI0_BASE + SSI_O_CPSR)& ~(SSI_CPSR_CPSDVSR_M)) + 50);

	
//Configure  clock rate (SCR = 99), phase & polarity (SPH, SPO), mode (FRF),data size (DSS)
	HWREG(SSI0_BASE + SSI_O_CR0) = ((HWREG(SSI0_BASE + SSI_O_CR0)& ~(SSI_CR0_SCR_M))+(99<<8));
	HWREG(SSI0_BASE + SSI_O_CR0) |= (SSI_CR0_SPH);
	HWREG(SSI0_BASE + SSI_O_CR0) |= (SSI_CR0_SPO);
	HWREG(SSI0_BASE+ SSI_O_CR0) = ((HWREG(SSI0_BASE+ SSI_O_CR0)&(~(SSI_CR0_FRF_M)))|(SSI_CR0_FRF_MOTO));
	HWREG(SSI0_BASE+ SSI_O_CR0) = ((HWREG(SSI0_BASE+ SSI_O_CR0)&(~(SSI_CR0_DSS_M)))|(SSI_CR0_DSS_8));
	
//Locally enable interrupts on TXRIS
	HWREG(SSI0_BASE + SSI_O_IM)|= SSI_IM_TXIM;
	
//Enabling NVIC
	HWREG(NVIC_EN0) |= BIT7HI;
	
//Enabling interrrupts globally
  __enable_irq();
	
//Make sure that the SSI is enabled for operation
		HWREG(SSI0_BASE+ SSI_O_CR1) |=( SSI_CR1_SSE );
		
		printf("SPI Initialized");
}


void EOTIntResponse( void )
{
	HWREG(SSI0_BASE+SSI_O_IM) &= ~(SSI_IM_TXIM); // Re-enabling the EOT interrupt
  ES_Event ThisEvent;
//	command = HWREG(SSI0_BASE+SSI_O_DR); // Reading the value in the receive FIFO of the TIVA
//	command = (command & SSI_DR_DATA_M ); //finding the last 16 bits of data register
  
	ThisEvent.EventType = EOT;	// Interrupt detected
//	ThisEvent.EventParam = command;
	PostCommunicationSM(ThisEvent); // Post the event to the motor service
	 
}
 
uint32_t GetVal (void) {

	return command;
}
