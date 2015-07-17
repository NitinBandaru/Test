
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
#include "ObstacleSM.h"

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "bitdefs.h"
#include "driverlib/gpio.h"

#include "PWMModule.h"
#include "PeriodicInterrupt.h"

/*----------------------------- Module Defines ----------------------------*/


#define ALL_BITS (0xff<<2)
#define UpperAngleTolerance 15
#define UpperYTolerance 20
#define LowerAngleTolerance 10
#define LowerYTolerance 10 
#define HighRotTargetRPM 30
#define LowRotTargetRPM 30
#define HighLinearTargetRPM 40
#define LowLinearTargetRPM 25 
#define ClimbUpLinearTargetRPM 25
#define ClimbDownLinearTargetRPM 15
#define DistanceScaling 375  //divided in equation by 1000
#define LinearTimeDelay 100

/*---------------------------- Module Functions ---------------------------*/

ObstacleState_t QueryObstacleSM ( void );

/*---------------------------- Module Variables ---------------------------*/

static ObstacleState_t CurrentState;
static uint32_t WayPointsOb[4]={259,96,17,17};
//static uint32_t TargetRPM=0;


ES_Event RunObstacleSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   ObstacleState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
		
	  uint32_t *QueryDRS; // To store the data received from querying the DRS
	  int16_t DelAngle, DelY; // Differences in current position from desired position
		uint16_t DRSAngle, DRSY; // Positions from DRS
		uint32_t ObstacleWheelTime; // Time for the wheels are run
	
	  QueryDRS = QueryReceivedData(); // Call the function to receive the DRS data
	//	DRSX = (*(QueryDRS+10) << 8) | (*(QueryDRS+11)); // Getting the X position
		DRSY = (*(QueryDRS+12) << 8) | (*(QueryDRS+13)); // Getting the Y position
		DRSAngle = *(QueryDRS+15);

	 switch ( CurrentState )
   {
       case WaitingOb :       // If current state is Waiting
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
       //  CurrentEvent = DuringStop(CurrentEvent); // No SubSM
         //process any events
			 {
				// printf("In waiting state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case StartObstacle : //If event is StartObstacle
							 {
																					
								 //start the Ultrasound input capture interrupt by enabling it and enabling the timer to stall while stopped by the debugger
								 
											HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
											
											MakeTransition=false; // Set make transition to true
											EntryEventKind.EventType = ES_ENTRY;
											ReturnEvent = CurrentEvent; // Set return event to current event

								}		 
								 break;
								
								case UltraSoundInt : //If event is UltraSound Input capture Interrupt
							 {
										// Ultrasound input capture Interrupt disabled
											ES_Event NewEvent;
											NewEvent.EventType = RobotStopped;
											PostRobotSM(NewEvent);
								 
								 			NextState = RotationStop;// Set  NextState to Stop
											MakeTransition=true; // Set make transition to true
											EntryEventKind.EventType = ES_ENTRY;
											ReturnEvent = CurrentEvent; // Set return event to current event

								}		 
								 break;
               }
						 }
					 }
						break;
      // repeat state pattern as required for other states
				case RotationStop :  {     // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
			//printf("In Rotating state\n\r");
			
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case RobotStopped : {
								 										
										DelAngle = WayPointsOb[0]-DRSAngle; // Calculate diff in angle 
									 	if ((DelAngle >0)&& (DelAngle<180))
										 {
											 Wheel1Reverse(); //Function to set wheel1 direction to reverse
											 Wheel2Forward(); //Function to set wheel2 direction to forward
										 }
										 else
										 {
											
											 Wheel2Reverse(); //Function to set wheel2 direction to reverse
											 Wheel1Forward(); //Function to set wheel1 direction to forward
												if (DelAngle>180)
													DelAngle  = 360 - DelAngle;
										 }
															


										if (abs(DelAngle) >= UpperAngleTolerance){
																
											ObstacleWheelTime = ((abs(DelAngle)*130)/10); //Get WheelTime from HighRotTarget RPM and modulus of DelAngle
											ES_Timer_InitTimer(Navigate_TIMER, ObstacleWheelTime); //Start Navigate_TIMER with value set to WheelTime
											ResetTargetRPM (HighRotTargetRPM); //Set TargetRPM to HighRotTargetRPM
											NextState = RotatingOb; //Set  NextState to Rotating
										}
															
										else if (abs(DelAngle)< UpperAngleTolerance  && abs(DelAngle)>= LowerAngleTolerance){
										
											ObstacleWheelTime = ((abs(DelAngle)*130)/10); //Get WheelTime from HighRotTarget RPM and modulus of DelAngle
											ES_Timer_InitTimer(Navigate_TIMER, ObstacleWheelTime); //Start Navigate_TIMER with value set to WheelTime
											ResetTargetRPM(HighRotTargetRPM); //Set TargetRPM to HighRotTargetRPM
											NextState = RotatingOb; //Set  NextState to Rotating
									
										}
															
										else {

											NextState = ClimbUp; //Set  NextState to ClimbUp
											ES_Event NewEvent;
											NewEvent.EventType = ClimbingUp;
											PostRobotSM(NewEvent);

										}
										
										
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
							 }
									break;
								
						 }             
            }
         }

         break;


				 
			 case ClimbUp :   
			 {				 // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
			//	 printf("In rotating state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ClimbingUp :
							 {		

										Wheel1Forward();
										Wheel2Forward();
										DelY = WayPointsOb[1]-DRSY; // Calculate diff in Y
										ObstacleWheelTime = (((abs(DelY)*DistanceScaling*477)/(ClimbUpLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from LowRotTarget RPM and modulus of DelY
										ES_Timer_InitTimer(Navigate_TIMER, ObstacleWheelTime); //Start Navigate_TIMER with value set to WheelTime
										ResetTargetRPM(ClimbUpLinearTargetRPM); //Set TargetRPM to ClimbUpLinearTargetRPM

										 
																			
										NextState = ClimbDown;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
								}
									break;
			              
            }
         }
			 }
         break;

			 case ClimbDown :   
			 {				 // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
		//		 printf("In rotating state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT :
							 {	

								if (CurrentEvent.EventType==Navigate_TIMER){	
										
										Wheel1Forward();
										Wheel2Forward();
										DelY = WayPointsOb[2]-DRSY; // Calculate diff in Y
										ObstacleWheelTime = (((abs(DelY)*DistanceScaling*477)/(ClimbDownLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from LowRotTarget RPM and modulus of DelY
										ES_Timer_InitTimer(Navigate_TIMER, ObstacleWheelTime); //Start Navigate_TIMER with value set to WheelTime
										ResetTargetRPM(ClimbDownLinearTargetRPM); //Set TargetRPM to ClimbUpLinearTargetRPM

//										ES_Event NewEvent;
//										NewEvent.EventType = RobotStopped;
//										PostRobotSM(NewEvent);
																			
										NextState = MovingForwardOb;// Set  NextState to Stop
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
			 
			case ForwardStop :  {     // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
			//printf("In Rotating state\n\r");
			
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
              case RobotStopped :
							 {	
										DelY = WayPointsOb[3]-DRSY; // Calculate diff in angle 
										
										if (DelY<0)
										{
											Wheel1Forward(); //Function to set wheel1 direction to reverse
									 		Wheel2Forward(); //Function to set wheel2 direction to forward
										}
										else
										{

											Wheel1Reverse(); //Function to set wheel1 direction to reverse
									 		Wheel2Reverse(); //Function to set wheel2 direction to forward

										}


										if (abs(DelY) >= UpperYTolerance){
																
											ObstacleWheelTime = (((abs(DelY)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from LowRotTarget RPM and modulus of DelY
											ES_Timer_InitTimer(Navigate_TIMER, ObstacleWheelTime); //Start Navigate_TIMER with value set to WheelTime
											ResetTargetRPM(HighLinearTargetRPM); //Set TargetRPM to ClimbUpLinearTargetRPM
											NextState = MovingForwardOb;

										}
															
										else if (abs(DelAngle)< UpperAngleTolerance  && abs(DelAngle)>= LowerAngleTolerance){
										
																
											ObstacleWheelTime = (((abs(DelY)*DistanceScaling*477)/(LowLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from LowRotTarget RPM and modulus of DelY
											ES_Timer_InitTimer(Navigate_TIMER, ObstacleWheelTime); //Start Navigate_TIMER with value set to WheelTime
											ResetTargetRPM(LowLinearTargetRPM); //Set TargetRPM to ClimbUpLinearTargetRPM
											NextState = MovingForwardOb;
									
										}
															
										else {

											NextState = WaitingOb;
											ES_Event NewEvent;
											NewEvent.EventType = ObstacleFinished;
											PostRobotSM(NewEvent);

										}
										
										
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
										
									
							 }
									break;
								
						 }             
            }
         }

         break;


			 case MovingForwardOb :  {     // If current state is MovingForward
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
		//	printf("In moving forward state\n\r");
			
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT?
                  if (CurrentEvent.EventParam==Navigate_TIMER){
										
										ResetTargetRPM(0); // Set the target RPM to 0
										HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN; //Stop Periodic Interrupt
										SetPWMDuty1(0); 
										SetPWMDuty2(0);
										ResetSumError();
										ResetLastPeriod();

										
										NextState = ForwardStop;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY_HISTORY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
								}
									break;
						        
            }
         }
			 }
         break;
				 
			 case RotatingOb :   
			 {				 // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
		//		 printf("In rotating state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT?
                  if (CurrentEvent.EventParam==Navigate_TIMER){
										ResetTargetRPM(0); // Set the target RPM to 0
										HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN; //Stop Periodic Interrupt
										SetPWMDuty1(0); 
										SetPWMDuty2(0);
										ResetSumError();
										ResetLastPeriod();
																			
										NextState = RotationStop;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY_HISTORY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
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

void StartObstacleSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = WaitingOb;
   }
   // call the entry function (if any) for the ENTRY_STATE
   //RunObstacleSM(CurrentEvent); // No entry function
}


ObstacleState_t QueryObstacleSM ( void )
{
   return(CurrentState);
}



/***************************************************************************
 private functions
 ***************************************************************************/
