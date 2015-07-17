
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/

#include "ShootingSM.h"
#include "CommunicationSM.h"
#include "SendingCommandSM.h"
#include "RobotSM.h"
#include "NavigationSM.h"
#include "LoadPWMModule.h"
#include "ShootPWMModule.h"

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "bitdefs.h"
#include "driverlib/gpio.h"
#include "inc/hw_pwm.h"


#include "PWMModule.h"
#include "PeriodicInterrupt.h"

/*----------------------------- Module Defines ----------------------------*/


#define ALL_BITS (0xff<<2)

/*---------------------------- Module Functions ---------------------------*/

ShootingState_t QueryShootingSM ( void );

/*---------------------------- Module Variables ---------------------------*/

static ShootingState_t CurrentState;


ES_Event RunShootingSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   ShootingState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
		
	 switch ( CurrentState )
   {
       case Waiting :       // If current state is Stop
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
       //  CurrentEvent = DuringStop(CurrentEvent); // No SubSM
         //process any events
			 {
				 printf("In waiting state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case StartShoot : //If event is 
							 {
											//Make sure that when StartShoot is posted the periodic interrupt and input captures for encoders are off
											printf("Rotate the wheels 1\r\n");
											//start the input cpature timer by enabling it and enabling the timer to stall while stopped by the debugger
											HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
											 
											printf("Rotate the wheels\r\n");
								 
											Wheel1Forward();
											Wheel2Reverse();
											//Set wheel directions
								 			ResetTargetRPM(25);

											//StartRotating
								
								 			NextState = Aligning;// Set  NextState to Stop
											MakeTransition=true; // Set make transition to true
											EntryEventKind.EventType = ES_ENTRY;
											ReturnEvent = CurrentEvent; // Set return event to current event

								}		 
								 break;
								
								case ES_TIMEOUT :
							 {								 //If event is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT
                  if (CurrentEvent.EventParam==BallWait_TIMER){


										HWREG( PWM0_BASE+PWM_O_2_CTL ) = 0; 										//Stop Loading Servo PWM
										HWREG(PWM0_BASE+ PWM_O_3_CTL) |= (PWM_3_CTL_MODE | PWM_3_CTL_ENABLE);										//Start Firing Servo PWM
										//SetShootServoPWM(1500);
										
										ES_Timer_InitTimer(FIRING_TIMER,2000);								
										NextState = Firing;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
									}
															 //If event is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT
                  if (CurrentEvent.EventParam==RELOAD_TIMER){
										//Start Firing Servo
										//Start Firing timer
										HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL); // Re enable periodic interrupt for control
										HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL); // Re enable input capture1 interrupt
										HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL); // Re enable input capture2 interrupt
										ES_Event NewEvent;
										NewEvent.EventType = ShootingFinished;
										PostRobotSM (NewEvent);
										 HWREG( PWM0_BASE+PWM_O_3_CTL ) = 0; //disable firing servo
										MakeTransition = false; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
									}
								}
									break;
								
               }
						 }
					 }
						break;
      // repeat state pattern as required for other states
			case Aligning :  {     // If current state is MovingForward
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
			printf("In aligning state\n\r");
			
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case AlignComplete : {
								 										
										//Start loading - rotate the servo
										//Start Loading timer 
								 		ResetTargetRPM(0);
										ResetSumError();
										ResetLastPeriod();
									//Enabling Load Servo PWM
								 		HWREG(PWM0_BASE+ PWM_O_2_CTL) |= (PWM_2_CTL_MODE | PWM_2_CTL_ENABLE);

										printf("Align complete");
										SetLoadServoPWM(875);
										ES_Timer_InitTimer(LOADING_TIMER,127);
										NextState = Loading;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
							 }
									break;
								
						 }             
            }
         }

         break;
				 
			 case Loading :   
			 {				 // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
				 printf("In rotating state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT :
							 {								 //If event is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT
                  if (CurrentEvent.EventParam==LOADING_TIMER){
										//Start Firing Servo
										//Start Firing timer
										SetLoadServoPWM(0);
										ES_Timer_InitTimer(BallWait_TIMER,2000);								
										NextState = Waiting;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
									}
								}
									break;
			              
            }
         }
			 }
         break;
			 
			 case Firing :   
			 {				 // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
				 printf("In firing state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT :
							 { //If ev0ent is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT
                  if (CurrentEvent.EventParam==FIRING_TIMER){
												
										//SetShootServoPWM(500);
										ES_Timer_InitTimer(RELOAD_TIMER,2000);
										NextState = Waiting;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
									}
								}
									break;
			              
            }
         }
			 }
         break;
			 }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunShootingSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunShootingSM(EntryEventKind);
     }
     return(ReturnEvent);
}

void StartShootingSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = Waiting;
   }
   // call the entry function (if any) for the ENTRY_STATE
   //RunSendingCommandSM(CurrentEvent); // No entry function
}


ShootingState_t QueryShootingSM ( void )
{
   return(CurrentState);
}



/***************************************************************************
 private functions
 ***************************************************************************/
