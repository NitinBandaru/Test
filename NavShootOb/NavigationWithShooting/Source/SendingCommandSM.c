
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "bitdefs.h"

#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "inc/hw_SSI.h"

#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "SendingCommandSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines



/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static SendingCommandState_t CurrentState;
static uint32_t ReceivedData1[16]; // 1st Array to store received data
static uint32_t ReceivedData2[16]; // 2nd Array to store received data
//static uint16_t index=0; // Index of the received data array
static bool flag= false; // Flag to switch between the two received arrays
static uint8_t CommandNo = 1;//
static bool FirstFlag = false;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunTemplateSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event RunSendingCommandSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   SendingCommandState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
       case WaitCommand :       // If current state is WaitByte
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringWaitByte(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case SendCommand : 
							 {//If event is SendCommand
                  uint8_t i;
							 		for (i=0;i<8;i++)
									{
										if (i==0)
											HWREG(SSI0_BASE +SSI_O_DR) = (0x00ff & CurrentEvent.EventParam);
										else 
											HWREG(SSI0_BASE +SSI_O_DR) = (0x0000);
									}
							
	
									HWREG(SSI0_BASE+SSI_O_IM) |= SSI_IM_TXIM;

									NextState = WaitForEOT;// Set  NextState to WaitForEOT
                  MakeTransition = true; //mark that we are taking a transition
								}
               break;
               
            }
         }
         break;
				 
     
			case WaitForEOT :       // If current state is WaitforEOT
         // Execute During function for WaitForEOT. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringWaitForEOT(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case EOT :
							 {								 //If event is EOT
                  // Execute action function for state one : event one
								 uint8_t i;
								 if (flag==0)
								 {
										if (CommandNo == 1)
										{
											for (i=0;i<8;i++)
											{
												ReceivedData1[i] = HWREG(SSI0_BASE+SSI_O_DR); // Reading the value in the receive FIFO of the TIVA
												ReceivedData1[i] = (ReceivedData1[i] & SSI_DR_DATA_M ); //finding the last 16 bits of data register
												//printf("%d\r\n",ReceivedData1[i]);
											}
									//		printf("\r\n");
									  }
									 else if (CommandNo == 2)
										{
											for (i=8;i<16;i++)
											{
												ReceivedData1[i] = HWREG(SSI0_BASE+SSI_O_DR); // Reading the value in the receive FIFO of the TIVA
												ReceivedData1[i] = (ReceivedData1[i] & SSI_DR_DATA_M ); //finding the last 16 bits of data register
												//printf("%d\r\n",ReceivedData1[i]);
										  } 
											if (FirstFlag!=false)
												{
												
													if (ReceivedData1[14]==255)
													{
														ReceivedData1[15] = 360 - (65536-((ReceivedData1[14] << 8) | (ReceivedData1[15])));
													}
													ReceivedData1[15] = ((ReceivedData2[15]>>2)+(ReceivedData1[15] - (ReceivedData1[15]>>2)));
								//					printf("Filtered angle 1 is %d\r\n", ReceivedData1[15]);
										//		printf("\r\n");
											}
										}
										FirstFlag = true;
									}
									

									else if (flag ==1)
									{
											if (CommandNo == 1)
											{
												for (i=0;i<8;i++)
												{
													ReceivedData2[i] = HWREG(SSI0_BASE+SSI_O_DR); // Reading the value in the receive FIFO of the TIVA
													ReceivedData2[i] = (ReceivedData2[i] & SSI_DR_DATA_M ); //finding the last 16 bits of data register
													//printf("%d\r\n",ReceivedData2[i]);
												} 
												
											//	printf("\r\n");
											}
									 
											else if (CommandNo == 2)
											{
												for (i=8;i<16;i++)
												{
													ReceivedData2[i] = HWREG(SSI0_BASE+SSI_O_DR); // Reading the value in the receive FIFO of the TIVA
													ReceivedData2[i] = (ReceivedData2[i] & SSI_DR_DATA_M ); //finding the last 16 bits of data register
													//printf("%d\r\n",ReceivedData2[i]);
											  } 
												if (ReceivedData2[14]==255)
												{
													ReceivedData2[15] = 360 - (65536-((ReceivedData2[14] << 8) | (ReceivedData2[15])));
												}
												ReceivedData2[15] = ((ReceivedData1[15]>>2)+(ReceivedData2[15] - (ReceivedData2[15]>>2)));
									//			printf("Filtered angle 2 is %d\r\n", ReceivedData2[15]);
											//	printf("\r\n");
											}
									}
									ES_Timer_InitTimer(ComInterval_TIMER,2); // Start the command interval timer set to 2ms
									NextState = WaitFor2ms;// Set  NextState to WaitFor2ms
                  MakeTransition = true; //mark that we are taking a transition
                  
                //  ReturnEvent = CurrentEvent; //Set ReturnEvent to CurrentEvent
								}
                  break;
                // repeat cases as required for relevant events
            }
					}
         break;
				 
			case WaitFor2ms :       // If current state is WaitFor2ms
         // Execute During function for WaitFor2ms. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringWaitFor2ms(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMOEUT
								 // Condition for the timer for timeout event?
                  // Execute action function for state one : event one
									if (CurrentEvent.EventParam==ComInterval_TIMER){									
										NextState = WaitCommand;// Set  NextState to WaitByte
										MakeTransition = true; //mark that we are taking a transition
                  }
                //  ReturnEvent = CurrentEvent; //Set ReturnEvent to CurrentEvent
                  break;
                // repeat cases as required for relevant events
            }
         }
         break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
      CurrentEvent.EventType = ES_EXIT;
      RunSendingCommandSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

//       //   Execute entry function for new state
//       // this defaults to ES_ENTRY
       RunSendingCommandSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartTemplateSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartSendingCommandSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = WaitCommand;
   }
   // call the entry function (if any) for the ENTRY_STATE
   //RunSendingByteSM(CurrentEvent); // No entry function
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
SendingCommandState_t QuerySendingByteSM ( void )
{
   return(CurrentState);
}

/*===================== Public Functions =================*/

/*===================== Function to query received data =================*/

uint32_t *QueryReceivedData(void)
{
	if (flag==0){
		return ReceivedData2; // Return pointer to the received data 2 array
	}
	else {
		return ReceivedData1; // Return pointer to the received data 1 array
	}
	
}

/*===================== Function to reset index of the array =================*/

void SetCommandNo (uint8_t NewCommand)
{
	
	CommandNo = NewCommand;
	
}

/*===================== Function to change the flag value =================*/

void ChangeFlag (void)
{
	if (flag==0){
			flag=1;
	}
	else{
			flag=0;
	}
}



