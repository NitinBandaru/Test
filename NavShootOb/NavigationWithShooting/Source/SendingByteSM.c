/****************************************************************************
 Module
   HSMTemplate.c

 Revision
   2.0.1

 Description
   This is a template file for implementing state machines.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/07/13 21:00 jec      corrections to return variable (should have been
                         ReturnEvent, not CurrentEvent) and several EV_xxx
                         event names that were left over from the old version
 02/08/12 09:56 jec      revisions for the Events and Services Framework Gen2
 02/13/10 14:29 jec      revised Start and run to add new kind of entry function
                         to make implementing history entry cleaner
 02/13/10 12:29 jec      added NewEvent local variable to During function and
                         comments about using either it or Event as the return
 02/11/10 15:54 jec      more revised comments, removing last comment in during
                         function that belongs in the run function
 02/09/10 17:21 jec      updated comments about internal transitions on During funtion
 02/18/09 10:14 jec      removed redundant call to RunLowerlevelSM in EV_Entry
                         processing in During function
 02/20/07 21:37 jec      converted to use enumerated type for events & states
 02/13/05 19:38 jec      added support for self-transitions, reworked
                         to eliminate repeated transition code
 02/11/05 16:54 jec      converted to implment hierarchy explicitly
 02/25/03 10:32 jec      converted to take a passed event parameter
 02/18/99 10:19 jec      built template from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
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
#include "SendingByteSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE STATE_ZERO

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static SendingByteState_t CurrentState;
static uint32_t ReceivedData1[16]; // 1st Array to store received data
static uint32_t ReceivedData2[16]; // 2nd Array to store received data
static uint8_t index=0; // Index of the received data array
static bool flag= false; // Flag to switch between the two received arrays

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
ES_Event RunSendingByteSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   SendingByteState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
       case WaitByte :       // If current state is WaitByte
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringWaitByte(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case SendByte : //If event is SendByte
                  
                  HWREG(SSI0_BASE +SSI_O_DR) = CurrentEvent.EventParam; //Write EventParam to SSDIR
									HWREG(SSI0_BASE+SSI_O_IM) |= SSI_IM_TXIM;//Local interrupt mask removed
									NextState = WaitForEOT;// Set  NextState to WaitForEOT
                  MakeTransition = true; //mark that we are taking a transition
                  
                //  ReturnEvent = CurrentEvent; //Set ReturnEvent to CurrentEvent
                  break;
               
            }
         }
         break;
      // repeat state pattern as required for other states
			case WaitForEOT :       // If current state is WaitByte
         // Execute During function for WaitForEOT. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringWaitForEOT(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case EOT : //If event is EOT
                  // Execute action function for state one : event one
									ES_Timer_InitTimer(ComInterval_TIMER,2); // Start the command interval timer set to 2ms
									printf("Received data is %d", CurrentEvent.EventParam);
                  if (flag==0){
										ReceivedData1[index]=CurrentEvent.EventParam; //Read SSDIR and set ReceivedData1 (index) to that read value 
									}
									else{
										ReceivedData2[index]=CurrentEvent.EventParam; //Read SSDIR and set ReceivedData2 (index) to that read value 
									}
									index++; //increse the index value
									
									NextState = WaitFor2ms;// Set  NextState to WaitFor2ms
                  MakeTransition = true; //mark that we are taking a transition
                  
                //  ReturnEvent = CurrentEvent; //Set ReturnEvent to CurrentEvent
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
										NextState = WaitByte;// Set  NextState to WaitByte
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
       RunSendingByteSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunSendingByteSM(EntryEventKind);
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
void StartSendingByteSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = WaitByte;
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
SendingByteState_t QuerySendingByteSM ( void )
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

void ResetIndex (void)
{
	
	index=0; //Set index to zero.
	
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












///***************************************************************************
// private functions
// ***************************************************************************/

//static ES_Event DuringStateOne( ES_Event Event)
//{
//    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

//    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
//    if ( (Event.EventType == ES_ENTRY) ||
//         (Event.EventType == ES_ENTRY_HISTORY) )
//    {
//        // implement any entry actions required for this state machine
//        
//        // after that start any lower level machines that run in this state
//        //StartLowerLevelSM( Event );
//        // repeat the StartxxxSM() functions for concurrent state machines
//        // on the lower level
//    }
//    else if ( Event.EventType == ES_EXIT )
//    {
//        // on exit, give the lower levels a chance to clean up first
//        //RunLowerLevelSM(Event);
//        // repeat for any concurrently running state machines
//        // now do any local exit functionality
//      
//    }else
//    // do the 'during' function for this state
//    {
//        // run any lower level state machine
//        // ReturnEvent = RunLowerLevelSM(Event);
//      
//        // repeat for any concurrent lower level machines
//      
//        // do any activity that is repeated as long as we are in this state
//    }
//    // return either Event, if you don't want to allow the lower level machine
//    // to remap the current event, or ReturnEvent if you do want to allow it.
//    return(ReturnEvent);
//}
