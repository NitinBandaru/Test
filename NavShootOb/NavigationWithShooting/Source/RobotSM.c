
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ES_Configure.h"
#include "ES_Framework.h"

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

#include "CommunicationSM.h"
#include "SendingCommandSM.h" 
#include "SPIModule.h"
#include "RobotSM.h"
#include "NavigationSM.h"
#include "ShootingSM.h"
#include "ObstacleSM.h"
#include "PWMModule.h"
#include "PeriodicInterrupt.h"
#include "InputCapture1.h"
#include "InputCapture2.h"



/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/

RobotState_t QueryRobotSM ( void );
static ES_Event DuringNavigation( ES_Event Event);
static ES_Event DuringShooting( ES_Event Event);
static ES_Event DuringObstacle( ES_Event Event);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static RobotState_t CurrentState;
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
bool InitRobotSM ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;  // save our priority
	
	// Start the Master State machine
	
	printf("\n\rRobot HSM initilaised\n\r");

  StartRobotSM( ThisEvent );

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
bool PostRobotSM( ES_Event ThisEvent )
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
ES_Event RunRobotSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   RobotState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch ( CurrentState )
   {		
		 
			case Navigation :       // If current state is navigation
			{
				printf("In Navigation STate\n\r");
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
			 
			 //printf("In Navigation State of ROBOTSM\n\r");
         CurrentEvent = DuringNavigation(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
							case GameStart:
							 {
								  // Start the game 
									 ES_Event NewEvent;
									 NewEvent.EventType = RobotStopped;
									 PostRobotSM(NewEvent);
									 ReturnEvent.EventType = ES_NO_EVENT;
								 							 	printf("RObotStopped from game start\n\r");
									 	// No state transition								
							 }								
							 break;
                // repeat cases as required for relevant events
							 
							 case CautionFlag : 
							 {//If event is CautionFlag
                  // Execute action function for state one : event one
//								  TargetRPM = 0; // Set the target RPM to 0
								 
										//Stopping the game
										HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN; //Stop Periodic Interrupt
										HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN; //Stop Input capture 1 Interrupt
										HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN; //Stop Input capture 2 Interrupt
								    ResetTargetRPM(0);
										SetPWMDuty1(0); 
										SetPWMDuty2(0);
										ResetSumError();
										ResetLastPeriod();
										ES_Timer_StopTimer(Navigate_TIMER);
										ES_Timer_StopTimer(NOPULSE1_TIMER);
										ES_Timer_StopTimer(NOPULSE2_TIMER);
                  NextState = NavigationTempStop;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
							 }
                  break;
							 
							 case GoToShooting: 
							 {
								 ES_Event NewEvent;
								 NewEvent.EventType=StartShoot;
								 PostRobotSM(NewEvent);
								 NextState = Shooting;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
								 
								 
							 }
							break;

							case GoToObstacle: 
							 {
								 ES_Event NewEvent;
								 NewEvent.EventType=StartObstacle;
								 PostRobotSM(NewEvent);
								 NextState = Obstacle;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
            }
							 break;
         }
			 }
		 }
         break;
      // repeat state pattern as required for other states
				 
			case NavigationTempStop :       // If current state is NavigationTempStop
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
			 
			 //printf("In Navigation State of ROBOTSM\n\r");
        // CurrentEvent = DuringNavigationTempStop(CurrentEvent);
         //process any events
			
					printf("In navigation temp stop state\r\n");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
							case GameStart:
							 {
								  // Restart the game 
								 HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL); // Re enable periodic interrupt for control
								 HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL); // Re enable input capture1 interrupt
								 HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL); // Re enable input capture2 interrupt
								// printf("periodic re enabled");

								 //if (QueryNavigationSM ()== Stop){
								 	printf("RObotStopped from navigation temp stop\n\r");
										 ES_Event NewEvent;
										 NewEvent.EventType = RobotStopped;
										 PostRobotSM(NewEvent);
									//}
									//else {
//										 ES_Event NewEvent;
//										 NewEvent.EventType = Restart;
//										 PostRobotSM(NewEvent);
									//	printf("Sending Restart evenmt\n\r");
									//}
									NextState = Navigation;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
									
									 	// No state transition								
							 }								
							 break;
                // repeat cases as required for relevant events
							 
						}
         }
         break;
			
			case Shooting :       // If current state is Shooting
			{
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringShooting(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
						
            switch (CurrentEvent.EventType)
            {		
							case ShootingFinished:
							{
								ES_Event NewEvent;
								NewEvent.EventType = RobotStopped;
								PostRobotSM(NewEvent);
									NextState = Navigation;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
								
							}
							break;
               case CautionFlag : 
							 {//If event is CautionFlag
                  // Execute action function for state one : event one
                  NextState = ShootingTempStop;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
							 }
								break;
               
            }
         }
			 }
         break;
				 
			 case ShootingTempStop :       // If current state is ShootingTempStop
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
			 
			 //printf("In Navigation State of ROBOTSM\n\r");
        // CurrentEvent = DuringNavigationTempStop(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
							case GameStart:
							 {
								  // Restart the game 
//								 
//								  if (QueryShootingSM ()== Stop){
//										 ES_Event NewEvent;
//										 NewEvent.EventType = RobotStopped;
//										 PostRobotSM(NewEvent);
//									}
//									else {
//										 ES_Event NewEvent;
//										 NewEvent.EventType = ES_TIMEOUT;
//										 NewEvent.EventParam = Navigate_TIMER;
//										 PostRobotSM(NewEvent);
//									}
									NextState = Shooting;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
									
									 	// No state transition								
							 }								
							 break;
                // repeat cases as required for relevant events
							 
						}
         }
         break;
			 
			case Obstacle :       // If current state is Obstacle
			{
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringObstacle(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
							case ObstacleFinished:
							{
								ES_Event NewEvent;
								NewEvent.EventType = RobotStopped;
								PostRobotSM(NewEvent);
									NextState = Navigation;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
								
							}
							break;
							
               case CautionFlag : 
							 {//If event is CautionFlag
                  // Execute action function for state one : event one
                  NextState = ObstacleTempStop;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
							 }
								break;
                // repeat cases as required for relevant events
            }
         }
			 }
         break;
				
			 case ObstacleTempStop :       // If current state is ObstacleTempStop
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
			 
			 //printf("In Navigation State of ROBOTSM\n\r");
        // CurrentEvent = DuringNavigationTempStop(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
							case GameStart:
							 {
								  // Restart the game 
								 
//								  if (QueryObstacleSM ()== Stop){
//										 ES_Event NewEvent;
//										 NewEvent.EventType = RobotStopped;
//										 PostRobotSM(NewEvent);
//									}
//									else {
//										 ES_Event NewEvent;
//										 NewEvent.EventType = ES_TIMEOUT;
//										 NewEvent.EventParam = Navigate_TIMER;
//										 PostRobotSM(NewEvent);
//									}
									NextState = Obstacle;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
									
									 	// No state transition								
							 }								
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
       RunRobotSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunRobotSM(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
   return(ReturnEvent);
}


void StartRobotSM ( ES_Event CurrentEvent )
{
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = Navigation;
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  //RunRobotSM(CurrentEvent);//No entry function
  return;
}

/*====================== Query Function ========================*/

RobotState_t QueryRobotSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringNavigation( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartNavigationSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunNavigationSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
         ReturnEvent = RunNavigationSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringShooting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartShootingSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunShootingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
         ReturnEvent = RunShootingSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringObstacle( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartObstacleSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunObstacleSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
         ReturnEvent = RunObstacleSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
