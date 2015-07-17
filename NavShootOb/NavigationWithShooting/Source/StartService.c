#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "string.h"



#include "ES_Configure.h"
#include "ES_Framework.h"

#include "StartService.h"

#include "RobotSM.h"
#include "CommunicationSM.h"
#include "SendingCommandSM.h"


#define ALL_BITS (0xff<<2)

static uint8_t MyPriority;   //Module Variable: MyPriority, 




//Takes a priority number, returns True.
bool InitializeStartService(uint8_t Priority)
{
	//Initialize the MyPriority variable with the passed in parameter.
	MyPriority = Priority;
	puts("Finished_Initialize_Start_Service\n\r");
	ES_Timer_InitTimer(CHECK_TIMER,20); 
	return true;
}

//Service Post Function- Takes an event ,puts it in the service queue using its priority number, returns True	
bool PostStartService (ES_Event ThisEvent)
{
	return ES_PostToService (MyPriority, ThisEvent);
}
	

//Service Run Function 
ES_Event RunStartService (ES_Event ThisEvent)
//The EventType field of ThisEvent will be one of: ES_TIMEOUT
{
	ES_Event ReturnEvent;
	ReturnEvent.EventType = ES_NO_EVENT;
	uint32_t *QueryDRS; // To store the data received from querying the DRS
	static uint8_t LastGameStatus=0;
  
	if (ThisEvent.EventType==ES_TIMEOUT && ThisEvent.EventParam==CHECK_TIMER) 
	{
		
		 QueryDRS = QueryReceivedData(); // Call the function to receive the DRS data
		 uint8_t CurrentGameStatus = (*(QueryDRS+3)&(BIT3HI|BIT4HI));
		 
		if (CurrentGameStatus!=LastGameStatus)
		{
				if (CurrentGameStatus == BIT3HI)
				{
						// Flag Dropped
							ES_Event NewEvent;
							NewEvent.EventType = GameStart;
							PostRobotSM(NewEvent);
							printf("Game Started\n\r");
				}
				
				else if (CurrentGameStatus == (BIT3HI|BIT4HI))
				{
						// Caution Flag 
							ES_Event NewEvent;
							NewEvent.EventType = CautionFlag;
							PostRobotSM(NewEvent);
							printf("Game Stopped\n\r");
				}
	  }
		
		LastGameStatus=CurrentGameStatus; // Updating last game status
		
			ES_Timer_InitTimer(CHECK_TIMER,20); 
	}
  return ReturnEvent;
}


