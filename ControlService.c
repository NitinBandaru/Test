#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "string.h"

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

#include "ES_Configure.h"
#include "ES_Framework.h"

#include "ControlService.h"
#include "PWMModule.h"
#include "LoadPWMModule.h"
#include "ShootPWMModule.h"
#include "InputCapture1.h"
#include "InputCapture2.h"
#include "PeriodicInterrupt.h"
#include "AlignInputCapture.h"
#include "UltrasonicInputCapture.h"

#include "RobotSM.h"



#define ALL_BITS (0xff<<2)

static uint8_t MyPriority;   //Module Variable: MyPriority, 
static bool FlagState1=false;
static bool FlagState2=false;//Module Variable: FlagState which can be changed by the Interruput response function



//Takes a priority number, returns True.
bool InitializeControlService(uint8_t Priority)
{
	//Initialize the MyPriority variable with the passed in parameter.
	MyPriority = Priority;
	puts("Finished_Initialize_Control_Service\n\r");
	InitPWM();
	InitInputCapturePeriod1();
	InitInputCapturePeriod2();
	InitPeriodicInt();
	InitLoadPWM();
	//InitShootPWM();
	InitAlignInputCapture();
	InitUltraInputCapture();
//			ES_Event NewEvent;
//			NewEvent.EventType = RobotStopped;
//			PostRobotSM(NewEvent);
	return true;
}

//Service Post Function- Takes an event ,puts it in the service queue using its priority number, returns True	
bool PostControlService (ES_Event ThisEvent)
{
	return ES_PostToService (MyPriority, ThisEvent);
}
	

//Service Run Function 
ES_Event RunControlService (ES_Event ThisEvent)
//The EventType field of ThisEvent will be one of: ES_TIMEOUT
{
	ES_Event ReturnEvent;
	ReturnEvent.EventType = ES_NO_EVENT;
//	uint32_t RPM1, RPM2;
//	uint32_t  EncoderPeriod1, EncoderPeriod2 ;
	if (ThisEvent.EventType==ES_TIMEOUT && ThisEvent.EventParam== TEMP_TIMER) 
	{
		printf("TEMpTIMER from control\n\r");
//		ES_Event NewEvent;
//		NewEvent.EventType = AlignComplete;
//		PostRobotSM(NewEvent);
			ES_Event NewEvent;
			NewEvent.EventType = RobotStopped;
			PostRobotSM(NewEvent);
		
	}

	if (ThisEvent.EventType==ES_TIMEOUT && ThisEvent.EventParam==NOPULSE1_TIMER) 
	{
		FlagState1 = false;//if there was no input capture for 20 ms set flag false so that LEDs are no longer updated
	//	printf("Wheel 1 Stopped\n\r");
		
		if (FlagState2 == false)
		{
			ES_Event NewEvent;
			NewEvent.EventType = RobotStopped;
			PostRobotSM(NewEvent);
			printf("Robot Stopped no pulse1\n\r");
			
			HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);//Reenabling periodic interrupt
						
			printf("Both wheels stopped\n\r");
		}
	}
	
	if (ThisEvent.EventType==ES_TIMEOUT && ThisEvent.EventParam==NOPULSE2_TIMER) 
	{
		FlagState2 = false;//if there was no input capture for 20 ms set flag false so that LEDs are no longer updated
		printf("Robot Stopped no pulse2\n\r");
		if (FlagState1 == false)
		{
			ES_Event NewEvent;
			NewEvent.EventType = RobotStopped;
			PostRobotSM(NewEvent);
			
		printf("Both wheels stopped\n\r");
			HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);//Reenabling periodic interrupt
		}
	}
  return ReturnEvent;
}

//Public function to allow InputCapturte module to change the value of FlagState
void SetFlag1 (bool flag1)
{
	FlagState1 = flag1; 
}

void SetFlag2 (bool flag2)
{
	FlagState2 = flag2; 
}


