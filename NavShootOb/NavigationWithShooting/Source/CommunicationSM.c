/****************************************************************************
 Module
   TopHSMTemplate.c

 Revision
   2.0.1

 Description
   This is a template for the top level Hierarchical state machine

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/08/12 01:39 jec      converted from MW_MasterMachine.c
 02/06/12 22:02 jec      converted to Gen 2 Events and Services Framework
 02/13/10 11:54 jec      converted During functions to return Event_t
                         so that they match the template
 02/21/07 17:04 jec      converted to pass Event_t to Start...()
 02/20/07 21:37 jec      converted to use enumerated type for events
 02/21/05 15:03 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ES_Configure.h"
#include "ES_Framework.h"

#include "CommunicationSM.h"
#include "SendingCommandSM.h" 
#include "SPIModule.h"




/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/

CommunicationState_t QueryCommunicationSM ( void );
static ES_Event DuringSendingGameStatus( ES_Event Event);
static ES_Event DuringSendingPosition1( ES_Event Event);
static ES_Event DuringSendingPosition2( ES_Event Event);
static ES_Event DuringSendingPosition3( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static CommunicationState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMasterSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:06
****************************************************************************/
bool InitCommunicationSM ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;  // save our priority
	
	InitSPI();// Initialize SPIModule
  
	ES_Timer_InitTimer(QUERY_TIMER,100); // Start the Query timer set to 100ms
  ThisEvent.EventType = ES_ENTRY;
  // Start the Master State machine
	
	printf("\n\rCommunication HSM initilaised\n\r");

  StartCommunicationSM( ThisEvent );

  return true;
}

/****************************************************************************
 Function
     PostMasterSM

 Parameters
     ES_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the post operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostCommunicationSM( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMasterSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   the run function for the top level state machine 
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 02/06/12, 22:09
****************************************************************************/
ES_Event RunCommunicationSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   CommunicationState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

	 
	 switch ( CurrentState )
   {
       case Wait :       // If current state is wait
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringWait(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 if (CurrentEvent.EventParam==QUERY_TIMER){
									 //Set condition on timer - query timer
									//	printf("\r\nGoing from Wait to SendingGameStatus\r\n");
										// Execute action function for state one : event one
										ES_Event NewEvent;
										ES_Timer_InitTimer(QUERY_TIMER,100); // Start the Query timer set to 100ms
										NewEvent.EventType = SendCommand; 
										NewEvent.EventParam = 0x3f; //Set SendCommand.EventParam to 0x3f
										SetCommandNo(1);
										PostCommunicationSM(NewEvent); //Post SendCommand to CommunicationSM

										NextState = SendingGameStatus;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										//EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
									  //ReturnEvent = CurrentEvent;
								 }                 
								 break;
                // repeat cases as required for relevant events
            }
         }
         break;
      // repeat state pattern as required for other states
			
			case SendingGameStatus :   {    // If current state is SendingGameStatus
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
				//printf("In Sending Game Status State\n\r");
         CurrentEvent = DuringSendingGameStatus(CurrentEvent);
					
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
						ES_Event NewEvent;
            switch (CurrentEvent.EventType)
            {		
		
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								if (CurrentEvent.EventParam==ComInterval_TIMER){
									 //Set condition on timer - cominterval timer
								 
									if (KnobAnalogValue==1){
										// Kart 1
										// printf("\r\nGoing from SendingGameStatus to SendingPosition1\r\n");
										NewEvent.EventType = SendCommand;
										NewEvent.EventParam = 0xc3; //Set SendCommand.EventParam to 0xc3
										SetCommandNo(2);
										PostCommunicationSM(NewEvent); //Post SendCommand to CommunicationSM
										NextState = SendingPosition1; // Set next state to sending position 1
									}
									else if (KnobAnalogValue==2){
										// kart 2
										NewEvent.EventType = SendCommand;
										NewEvent.EventParam = 0x5a; //Set SendCommand.EventParam to 0x5a
										SetCommandNo(2);
										PostCommunicationSM(NewEvent); //Post SendCommand to CommunicationSM
										NextState = SendingPosition2; // Set next state to sending position 2
									}
									else if (KnobAnalogValue==3){
										// kart 3
										NewEvent.EventType = SendCommand;
										NewEvent.EventParam = 0x7e; //Set SendCommand.EventParam to 0x7e
										SetCommandNo(2);
										PostCommunicationSM(NewEvent); //Post SendCommand to CommunicationSM
										NextState = SendingPosition3; // Set next state to sending position 3
									}
										//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										//EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										//ReturnEvent = CurrentEvent;
							}  
								break;
                // repeat cases as required for relevant events
            }
         }
			 }
         break;
				 
			case SendingPosition1 :   {   // If current state is Sendingposition1
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringSendingPosition1(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 if (CurrentEvent.EventParam==ComInterval_TIMER){
									 //Set condition on timer - com interval timer
								 
										// Execute action function for state one : event one
								 
										ChangeFlag(); // Call the function to change the flag

										NextState = Wait;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										//EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										//ReturnEvent.EventType = ES_NO_EVENT;
								 }
									break;
                // repeat cases as required for relevant events
            }
         }
			 }
         break;
				
			case SendingPosition2 :   {    // If current state is Sendingposition2
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringSendingPosition2(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 if (CurrentEvent.EventParam==ComInterval_TIMER){
									 //Set condition on timer - com interval timer
								 
										// Execute action function for state one : event one
								 
										// Call the function to reset the index
										ChangeFlag(); // Call the function to change the flag

										NextState = Wait;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										//EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										//ReturnEvent.EventType = ES_NO_EVENT;
								 }
									break;
                // repeat cases as required for relevant events
            }
         }
			 }
         break;
				 
			case SendingPosition3 :  {     // If current state is Sendingposition3
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringSendingPosition3(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 if (CurrentEvent.EventParam==ComInterval_TIMER){
									 //Set condition on timer - com interval timer
								 
										// Execute action function for state one : event one
								 
										 // Call the function to reset the index
										ChangeFlag(); // Call the function to change the flag

										NextState = Wait;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										//EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										//ReturnEvent.EventType = ES_NO_EVENT;
								 }
									break;
                // repeat cases as required for relevant events
            }
         }
			 }
         break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
		//	printf("Making Transition in Communication SM\n\r");
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunCommunicationSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunCommunicationSM(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
	//	printf("End of Run CommunicationSM\n\r");
   return(ReturnEvent);
	
}
/****************************************************************************
 Function
     StartMasterSM

 Parameters
     ES_Event CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:15
****************************************************************************/
void StartCommunicationSM ( ES_Event CurrentEvent )
{
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = Wait;
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunCommunicationSM(CurrentEvent);//No entry function
  return;
}

/*====================== Query Function ========================*/

CommunicationState_t QueryCommunicationSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringSendingGameStatus( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
		//printf("During Sending Game Status\n\r");
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
			//	printf("Starting Sending Command SM\n\r");
        StartSendingCommandSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
		
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendingCommandSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    } else
    // do the 'during' function for this state
    {
        // run any lower level state machine
			//		printf("Event is now Send Command\n\r");
         ReturnEvent = RunSendingCommandSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.

    return(ReturnEvent);
			 
}

static ES_Event DuringSendingPosition1( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartSendingCommandSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendingCommandSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
         ReturnEvent = RunSendingCommandSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringSendingPosition2( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartSendingCommandSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendingCommandSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
         ReturnEvent = RunSendingCommandSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringSendingPosition3( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartSendingCommandSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendingCommandSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
         ReturnEvent = RunSendingCommandSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
