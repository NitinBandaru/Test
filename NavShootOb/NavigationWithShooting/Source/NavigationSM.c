
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "CommunicationSM.h"
#include "SendingCommandSM.h"

#include "RobotSM.h"
#include "NavigationSM.h"

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

#include "PWMModule.h"
#include "PeriodicInterrupt.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define UpperAngleTolerance 15
#define UpperXTolerance 20
#define UpperYTolerance 20
#define LowerAngleTolerance 8
#define LowerXTolerance 10 
#define LowerYTolerance 10 
#define HighRotTargetRPM 30
#define LowRotTargetRPM 30
#define HighLinearTargetRPM 45
#define LowLinearTargetRPM 30
#define DistanceScaling 375  //divided in equation by 1000
#define LinearTimeDelay 200


#define ALL_BITS (0xff<<2)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/

NavigationState_t QueryNavigationSM ( void );
void Wheel1Forward(void);
void Wheel1Reverse(void);
void Wheel2Forward(void);
void Wheel2Reverse(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static NavigationState_t CurrentState;
static uint32_t WayPoints[] = {0,96,87,157,175,229,255,23,0,165,0,89,180,132,84,94,180} ;
static uint8_t ArrayIndex=0;
static uint32_t TargetRPM=0;



/*------------------------------ Module Code ------------------------------*/

ES_Event RunNavigationSM( ES_Event CurrentEvent )
{
    bool MakeTransition = false;/* are we making a state transition? */
   NavigationState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
		
	 uint32_t *QueryDRS; // To store the data received from querying the DRS
	 int16_t DelAngle, DelX, DelY; // Differences in current position from desired position
   uint16_t DRSAngle, DRSX, DRSY; // Positions from DRS
	 uint32_t WheelTime; // Time for the wheels are run
	 
	 switch ( CurrentState )
   {
       case Stop :       // If current state is Stop
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
       //  CurrentEvent = DuringStop(CurrentEvent); // No SubSM
         //process any events
			 {
				//  printf("In stop state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case RobotStopped : //If event is RobotStopped
							 {
								printf("RobotStopped Event"); 
                 QueryDRS = QueryReceivedData(); // Call the function to receive the DRS data
								 DRSX = (*(QueryDRS+10) << 8) | (*(QueryDRS+11)); // Getting the X position
								 DRSY = (*(QueryDRS+12) << 8) | (*(QueryDRS+13)); // Getting the Y position
							 
								 
//								  if (*(QueryDRS+14)==255)
//								 { 
//									DRSAngle = 360 - (65536-((*(QueryDRS+14) << 8) | (*(QueryDRS+15)))); // Getting the Angle
//								 }
//								 else
//								 {									 
//									 DRSAngle = (*(QueryDRS+14) << 8) | (*(QueryDRS+15)); 
//								 }
								 
								  DRSAngle = *(QueryDRS+15); // Filtered angle from the DRS Query
//								 printf("DRS Angle is %d", DRSAngle);
								 
								 // Main cases 
								 switch (ArrayIndex)
									 {
											// Main rotation cases
											case 0:
											case 2:
											case 4:
											case 6:
											case 8:
											case 10:
											case 12:
											case 15:
											{
												DelAngle = WayPoints[ArrayIndex]-DRSAngle; // Calculate diff in angle 
//											 printf("Angle is %d", DelAngle);
														 
														 // Set directions of the motors
														 switch (ArrayIndex)
														 {
															 case 0 :
															 case 8 :
															 case 10 :
															 {
																	if (abs(DelAngle)<=180){
																		//If angle is between 0 and 180
																		Wheel1Forward(); //Function to set wheel1 direction to forward
																		Wheel2Reverse(); //Function to set wheel2 direction to reverse
																	}
																	else {
																		//If angle is between 180 and 360
																		Wheel1Reverse(); //Function to set wheel1 direction to reverse
																		Wheel2Forward(); //Function to set wheel2 direction to forward
																		DelAngle = 360+DelAngle; // Rotate in the opposite direction for 360-angle
																	}
															 
																}
															 break;
															 
															 case 2:
															 case 15:
																 
															 {
																	if ((DelAngle>=0)|| (abs(DelAngle>180)))
																		{
																	 Wheel1Reverse(); //Function to set wheel1 direction to reverse
																	 Wheel2Forward(); //Function to set wheel2 direction to forward
																	 if (abs(DelAngle)>180)
																		 DelAngle = 360+DelAngle; // If angle goes into 300s instead of 0
																		}
																	 else
																	 {
																		 Wheel2Reverse(); //Function to set wheel2 direction to reverse
																		 Wheel1Forward(); //Function to set wheel1 direction to forward
																	 }
																}
																
															 break;
																
															 case 4:
															 case 12:
															 { if (DelAngle>=0)
																 {
																	 Wheel1Reverse(); //Function to set wheel1 direction to reverse
																	 Wheel2Forward(); //Function to set wheel2 direction to forward
																 }
																 else
																	 {
																		 Wheel2Reverse(); //Function to set wheel2 direction to reverse
																		 Wheel1Forward(); //Function to set wheel1 direction to forward
																	 }
															 }
															 break;
															 
															 case 6:
															{ 	if ((DelAngle >0)&& (DelAngle<180))
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
															 }
															 break; 
													 
														 }
														 
															if (abs(DelAngle) >= UpperAngleTolerance){
																
																	WheelTime = ((abs(DelAngle)*130)/10); //Get WheelTime from HighRotTarget RPM and modulus of DelAngle
																	ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																	TargetRPM = HighRotTargetRPM; //Set TargetRPM to HighRotTargetRPM
																	NextState = Rotating; //Set  NextState to Rotating
															}
															
															else if (abs(DelAngle)< UpperAngleTolerance  && abs(DelAngle)>= LowerAngleTolerance){
										
																	WheelTime = ((abs(DelAngle)*130)/10); //Get WheelTime from HighRotTarget RPM and modulus of DelAngle
																	ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																	TargetRPM = HighRotTargetRPM; //Set TargetRPM to HighRotTargetRPM
																	NextState = Rotating; //Set  NextState to Rotating
									
															}
															
															else {
																
																	switch(ArrayIndex)
																	{
																		
																			case 2: // Testing shot success
																			{
																				switch (KnobAnalogValue)
																				{
																						case 1:
																						{
																							if((*(QueryDRS+3) & BIT7HI)!=BIT7HI)
																								ArrayIndex=11;
																						}
																						break;
																						
																						case 2:
																						{
																							if((*(QueryDRS+4) & BIT7HI)!=BIT7HI)
																								ArrayIndex=11;
																						}
																						break;
																						
																						case 3:
																						{
																							if((*(QueryDRS+5) & BIT7HI)!=BIT7HI)
																								ArrayIndex=11;
																						}
																						break;
																				}
																			}
																			break;
																			
																			case 4: // Testing Obstacle success
																			{
																				switch (KnobAnalogValue)
																				{
																						case 1:
																						{
																							if((*(QueryDRS+3) & BIT6HI)!=BIT6HI)
																								ArrayIndex=16;
																						}
																						break;
																						
																						case 2:
																						{
																							if((*(QueryDRS+4) & BIT6HI)!=BIT6HI)
																								ArrayIndex=16;
																						}
																						break;
																						
																						case 3:
																						{
																							if((*(QueryDRS+5) & BIT6HI)!=BIT6HI)
																								ArrayIndex=16;
																						}
																						break;
																				}
																			}
																			break;
																			
																			case 15: // Shooting finished
																			{
																				ArrayIndex=3;
																			}
																			break;
																			
																			case 10: // Lap reset
																			{
																				ArrayIndex=1;
																			}
																			break;
																			
																			case 0:
																			case 6:
																			case 8:
																			case 12:// Lap reset
																			{
																				ArrayIndex++; // increas the array index
																			}
																			break;
																			
																			
																	}
																
																	
																	switch (ArrayIndex)
																	{
																			case 1:
																			case 14:
																				{
																				DelX = WayPoints[ArrayIndex]-DRSX; // Calculate diff in X position 
																				if (DelX<=0){
																						Wheel1Forward(); //Function to set wheel1 direction to forward
																						Wheel2Forward(); //Function to set wheel2 direction to forward
																				}
																		
																				else{
																						Wheel1Reverse(); //Function to set wheel1 direction to reverse
																						Wheel2Reverse(); //Function to set wheel2 direction to reverse
																				}
																				
																				WheelTime = (((abs(DelX)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from HighLinearTargetRPM and modulus of DelAngle
																				ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																				TargetRPM = HighLinearTargetRPM; //Set TargetRPM to HighLinearTargetRPM
																				NextState = MovingForward; //Set  NextState to MovingForward
																				
																			}
																			break;
																			
																			case 5:
																			case 13:
																			case 16:
																			{
																				DelX = WayPoints[ArrayIndex]-DRSX; // Calculate diff in X position 
																				if (DelX>=0){
																						Wheel1Forward(); //Function to set wheel1 direction to forward
																						Wheel2Forward(); //Function to set wheel2 direction to forward
																				}
																		
																				else{
																						Wheel1Reverse(); //Function to set wheel1 direction to reverse
																						Wheel2Reverse(); //Function to set wheel2 direction to reverse
																				}
																				
																				WheelTime = (((abs(DelX)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from HighLinearTargetRPM and modulus of DelAngle
																				ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																				TargetRPM = HighLinearTargetRPM; //Set TargetRPM to HighLinearTargetRPM
																				NextState = MovingForward; //Set  NextState to MovingForward
																				
																			}
																			break;
																			
																			case 3:
																			case 11:
																			{
																				DelY = WayPoints[ArrayIndex]-DRSY; // Calculate diff in Y position
																				
																				if (DelY>=0){
																						Wheel1Forward(); //Function to set wheel1 direction to forward
																						Wheel2Forward(); //Function to set wheel2 direction to forward
																				}
																			
																				else{
																						Wheel1Reverse(); //Function to set wheel1 direction to reverse
																						Wheel2Reverse(); //Function to set wheel2 direction to reverse
																				}
																				
																				WheelTime = (((abs(DelY)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay);//Get WheelTime from HighLinearTargetRPM and modulus of DelAngle
																				ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																				TargetRPM = HighLinearTargetRPM; //Set TargetRPM to HighLinearTargetRPM
																				NextState = MovingForward; //Set  NextState to MovingForward
																				
																			}
																			break;
																			
																			case 7:
																			{
																				
				//																printf("In 7 of rotation 6\r\n");
																				DelY = WayPoints[ArrayIndex]-DRSY; // Calculate diff in Y position
				//																printf("Del Y is %d\r\n", DelY);
																				if (DelY<=0){
																						Wheel1Forward(); //Function to set wheel1 direction to forward
																						Wheel2Forward(); //Function to set wheel2 direction to forward
																				}
																			
																				else if (DelY>0){
																						Wheel1Reverse(); //Function to set wheel1 direction to reverse
																						Wheel2Reverse(); //Function to set wheel2 direction to reverse
																				}
																				
																				WheelTime = (((abs(DelY)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay);//Get WheelTime from HighLinearTargetRPM and modulus of DelAngle
																				ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																				TargetRPM = HighLinearTargetRPM; //Set TargetRPM to HighLinearTargetRPM
																				NextState = MovingForward; //Set  NextState to MovingForward
																				
																			}
																			break;
																			
																			case 9:
																			{
																				//ArrayIndex=1; // Set array index to 1
																				
																				// Set directions corresponding to array index 1
																				
																				DelX = WayPoints[ArrayIndex]-DRSX; // Calculate diff in X position 
																				
																				if (DelX<=0){
																					Wheel1Forward(); //Function to set wheel1 direction to forward
																					Wheel2Forward(); //Function to set wheel2 direction to forward
																				}
																	
																				else{
																					Wheel1Reverse(); //Function to set wheel1 direction to reverse
																					Wheel2Reverse(); //Function to set wheel2 direction to reverse
																				}			

																				WheelTime = (((abs(DelX)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from HighLinearTargetRPM and modulus of DelAngle
																				ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																				TargetRPM = HighLinearTargetRPM; //Set TargetRPM to HighLinearTargetRPM
																				NextState = MovingForward; //Set  NextState to MovingForward																				
																				
																				
																			}
																			break;
																			
																	}
																}
														 
														 MakeTransition=true; // Set make transition to true
														 EntryEventKind.EventType = ES_ENTRY_HISTORY;
														 ReturnEvent = CurrentEvent; // Set return event to current event
									 
											}
											break;
											
											// Main X direction cases
											
											case 1:
											case 5:
											case 13:
											case 14:
											case 16:
											{
												  // if array index corresponds to moving forward in X direction
														DelX = WayPoints[ArrayIndex]-DRSX; // Calculate diff in X position 
														
														// Set directions of the motors
														switch (ArrayIndex)
																{
																		case 1:
																		case 14: 
																		{
																			if (DelX<=0){
																					Wheel1Forward(); //Function to set wheel1 direction to forward
																					Wheel2Forward(); //Function to set wheel2 direction to forward
																			}
																	
																			else{
																					Wheel1Reverse(); //Function to set wheel1 direction to reverse
																					Wheel2Reverse(); //Function to set wheel2 direction to reverse
																			}															
																																		
																		}
																		break;
																		
																		case 5:
																		case 13:
																		case 16:
																		{
																			if (DelX>=0){
																					Wheel1Forward(); //Function to set wheel1 direction to forward
																					Wheel2Forward(); //Function to set wheel2 direction to forward
																			}
																	
																			else{
																					Wheel1Reverse(); //Function to set wheel1 direction to reverse
																					Wheel2Reverse(); //Function to set wheel2 direction to reverse
																			}
																																																	
																		}
																		break;
																		
																}
													 
														 if (abs(DelX) >= UpperXTolerance){
																							
																WheelTime = (((abs(DelX)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from HighLinearTargetRPM and modulus of DelAngle
																ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																TargetRPM = HighLinearTargetRPM; //Set TargetRPM to HighLinearTargetRPM
																NextState = MovingForward; //Set  NextState to MovingForward
																MakeTransition=true; // Set make transition to true
																EntryEventKind.EventType = ES_ENTRY_HISTORY;
																ReturnEvent = CurrentEvent; // Set return event to current event
															 
															}
															
															else if (abs(DelX)< UpperXTolerance  && abs(DelX)>= LowerXTolerance){
																
																	WheelTime = (((abs(DelX)*DistanceScaling*477)/(LowLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from LowRotTarget RPM and modulus of DelX
																	ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																	TargetRPM = LowLinearTargetRPM; //Set TargetRPM to LowLinearTargetRPM
																	NextState = MovingForward; //Set  NextState to MovingForward
																	MakeTransition=true; // Set make transition to true
																	EntryEventKind.EventType = ES_ENTRY_HISTORY;
																	ReturnEvent = CurrentEvent; // Set return event to current event
									
															}
															
															else {
																
																if (ArrayIndex==13)
																{
																	ES_Event NewEvent;
																	NewEvent.EventType = GoToShooting;
																	PostRobotSM(NewEvent);
																	//ES_Timer_InitTimer(TEMP_TIMER, 5000);
																	ArrayIndex++; // increase the array index
																	
																}
																
																else if (ArrayIndex==16)
																{
																	ES_Event NewEvent;
																	NewEvent.EventType = GoToObstacle;
																	PostRobotSM(NewEvent);
																	//ES_Timer_InitTimer(TEMP_TIMER, 5000);
																	ArrayIndex=8; // Set the array index to rotate back on track
																	
																}
																
																else
																{
																	ArrayIndex++; // increase the array index
																	
	//																if(ArrayIndex%2==0){
																		
																		 DelAngle = WayPoints[ArrayIndex]-DRSAngle; // Calculate diff in angle
				//														 printf("DelAngle is %d", DelAngle);
																			// Set directions of the motors
																				 switch (ArrayIndex)
																				 {
																					 case 0 :
																					 case 8:
																					 case 10:
																					 {
																							if (abs(DelAngle)<=180){
																								//If angle is between 0 and 180
																								Wheel1Forward(); //Function to set wheel1 direction to forward
																								Wheel2Reverse(); //Function to set wheel2 direction to reverse
																							}
																							else {
																								//If angle is between 180 and 360
																								Wheel1Reverse(); //Function to set wheel1 direction to reverse
																								Wheel2Forward(); //Function to set wheel2 direction to forward
																								DelAngle = 360+DelAngle; // Rotate in the opposite direction for 360-angle
																							}
																					 
																						}
																					 break;
																					 
																					 case 2:
																					 case 15:
																							 {
																									if ((DelAngle>=0)|| (abs(DelAngle>180)))
																										{
																									 Wheel1Reverse(); //Function to set wheel1 direction to reverse
																									 Wheel2Forward(); //Function to set wheel2 direction to forward
																									 if (abs(DelAngle)>180)
																										 DelAngle = 360+DelAngle; // If angle goes into 300s instead of 0
																										}
																									 else
																									 {
																										 Wheel2Reverse(); //Function to set wheel2 direction to reverse
																										 Wheel1Forward(); //Function to set wheel1 direction to forward
																									 }
																								}
																					 break;
																						
																					 case 4:
																					 case 12:
																					 { if (DelAngle>=0)
																						 {
																							 Wheel1Reverse(); //Function to set wheel1 direction to reverse
																							 Wheel2Forward(); //Function to set wheel2 direction to forward
																						 }
																						 else
																						 {
																							 Wheel2Reverse(); //Function to set wheel2 direction to reverse
																							 Wheel1Forward(); //Function to set wheel1 direction to forward
																						 }
																					 }
																					 break;
																					 
																					 case 6:
																						{ 	if ((DelAngle >0)&& (DelAngle<180))
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
																						 }
																						 break; 
																																										 
																				 }
																	 
																		WheelTime = ((abs(DelAngle)*130)/10); //Get WheelTime from HighRotTarget RPM and modulus of DelAngle
																		ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																		TargetRPM = HighRotTargetRPM; //Set TargetRPM to HighRotTargetRPM
																		NextState = Rotating; //Set  NextState to Rotating
																		
	//																}
																	
																
																
																		MakeTransition=true; // Set make transition to true
																		EntryEventKind.EventType = ES_ENTRY_HISTORY;
																		ReturnEvent = CurrentEvent; // Set return event to current event

																 }
																
														}
														
												
											}
											break;
											
											
											// Main Y direction cases
											
											case 3:
											case 7:
											case 11 :
											{
												 // if array index corresponds to moving forward in Y direction
															DelY = WayPoints[ArrayIndex]-DRSY; // Calculate diff in Y position 
		//													printf("Del Y is %d\r\n", DelY);
															// Set directions for the motors
														 switch (ArrayIndex)
																	{
																			
																			case 3:
																			case 11 :
																			{
																																		
																				if (DelY>=0){
																						Wheel1Forward(); //Function to set wheel1 direction to forward
																						Wheel2Forward(); //Function to set wheel2 direction to forward
																				}
																			
																				else{
																						Wheel1Reverse(); //Function to set wheel1 direction to reverse
																						Wheel2Reverse(); //Function to set wheel2 direction to reverse
																				}
																																																	
																			}
																			break;
																			
																			case 7:
																			{
																				
																				if (DelY<=0){
																						Wheel1Forward(); //Function to set wheel1 direction to forward
																						Wheel2Forward(); //Function to set wheel2 direction to forward
																				}
																			
																				else if (DelY>0){
																						Wheel1Reverse(); //Function to set wheel1 direction to reverse
																						Wheel2Reverse(); //Function to set wheel2 direction to reverse
																				}
																				
																			}
																			break;
																			
																	}
															
															 if (abs(DelY) >= UpperYTolerance){
																 
		//														 printf("In upper tolerance of 7 \r\n");
																	
																		WheelTime = (((abs(DelY)*DistanceScaling*477)/(HighLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from HighLinearTarget RPM and modulus of DelY
																		ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																		TargetRPM = HighLinearTargetRPM; //Set TargetRPM to HighLinearTargetRPM
																		NextState = MovingForward; //Set  NextState to MovingForward
																}
																
																else if (abs(DelY)< UpperYTolerance  && abs(DelY)>= LowerYTolerance){
											
			//														 printf("In middle tolerance of 7 \r\n");
																		WheelTime = (((abs(DelY)*DistanceScaling*477)/(LowLinearTargetRPM*100))+ LinearTimeDelay); //Get WheelTime from LowRotTarget RPM and modulus of DelY
																		ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																		TargetRPM = LowLinearTargetRPM; //Set TargetRPM to LowLinearTargetRPM
																		NextState = MovingForward; //Set  NextState to MovingForward
										
																}
																
																else {
																	
																	ArrayIndex++; // increas the array index
																	
//																	if(ArrayIndex%2==0){
					//													if(ArrayIndex%2==0){
																		
																		 DelAngle = WayPoints[ArrayIndex]-DRSAngle; // Calculate diff in angle
			//															 printf("DelAngle is %d", DelAngle);
																			// Set directions of the motors
																				 switch (ArrayIndex)
																				 {
																					 case 0 : 
																					 case 8:
																					 case 10:
																					 {
																							if (abs(DelAngle)<=180){
																								//If angle is between 0 and 180
																								Wheel1Forward(); //Function to set wheel1 direction to forward
																								Wheel2Reverse(); //Function to set wheel2 direction to reverse
																							}
																							else {
																								//If angle is between 180 and 360
																								Wheel1Reverse(); //Function to set wheel1 direction to reverse
																								Wheel2Forward(); //Function to set wheel2 direction to forward
																								DelAngle = 360+DelAngle; // Rotate in the opposite direction for 360-angle
																							}
																					 
																						}
																					 break;
																					 
																					 case 2:
																					 case 15:
																						 
																						{
																							if ((DelAngle>=0)|| (abs(DelAngle>180)))
																								{
																							 Wheel1Reverse(); //Function to set wheel1 direction to reverse
																							 Wheel2Forward(); //Function to set wheel2 direction to forward
																							 if (abs(DelAngle)>180)
																								 DelAngle = 360+DelAngle; // If angle goes into 300s instead of 0
																								}
																							 else
																							 {
																								 Wheel2Reverse(); //Function to set wheel2 direction to reverse
																								 Wheel1Forward(); //Function to set wheel1 direction to forward
																							 }
																						}
																					 break;
																						
																					 case 4:
																					 case 12:
																					 { if (DelAngle>=0)
																						 {
																							 Wheel1Reverse(); //Function to set wheel1 direction to reverse
																							 Wheel2Forward(); //Function to set wheel2 direction to forward
																						 }
																						 else
																						 {
																							 Wheel2Reverse(); //Function to set wheel2 direction to reverse
																							 Wheel1Forward(); //Function to set wheel1 direction to forward
																						 }
																					 }
																					 break;
																					 
																					 case 6:
																					{ 	if ((DelAngle >0)&& (DelAngle<180))
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
																					 }
																					 break; 
																					
																				 }
																	 
																		WheelTime = ((abs(DelAngle)*130)/10); //Get WheelTime from HighRotTarget RPM and modulus of DelAngle
																		ES_Timer_InitTimer(Navigate_TIMER, WheelTime); //Start Navigate_TIMER with value set to WheelTime
																		TargetRPM = HighRotTargetRPM; //Set TargetRPM to HighRotTargetRPM
																		NextState = Rotating; //Set  NextState to Rotating
																		
//																	}
																	
																	
																}
																
																MakeTransition=true; // Set make transition to true
																EntryEventKind.EventType = ES_ENTRY_HISTORY;
																ReturnEvent = CurrentEvent; // Set return event to current event

												
												
											}
											break;
																			 
										 
										 
									 }
								 
																		 
//									if (ArrayIndex ==9) ArrayIndex = 1;
									 
								 }
								 
								 break;
               }
						 }
					 }
						break;
      // repeat state pattern as required for other states
			case MovingForward :  {     // If current state is MovingForward
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
	//		printf("In moving forward state\n\r");
			
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT?
                  if (CurrentEvent.EventParam==Navigate_TIMER){
										
										TargetRPM = 0; // Set the target RPM to 0
										HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN; //Stop Periodic Interrupt
										SetPWMDuty1(0); 
										SetPWMDuty2(0);
										ResetSumError();
										ResetLastPeriod();
//										ES_Event NewEvent;
//										NewEvent.EventType = RobotStopped;
//										PostRobotSM(NewEvent);
										
										NextState = Stop;// Set  NextState to Stop
										MakeTransition = true; //mark that we are taking a transition
										EntryEventKind.EventType = ES_ENTRY_HISTORY;
										ReturnEvent=CurrentEvent; //Set ReturnEvent to CurrentEvent
								}
									break;
						        
            }
         }
			 }
         break;
				 
			 case Rotating :   
			 {				 // If current state is Rotating
         // Execute During function for WaitByte. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
        // CurrentEvent = DuringMovingForward(CurrentEvent);
         //process any events
//				 printf("In rotating state\n\r");
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is ES_TIMEOUT
								 // Set condition for the timer of ES_TIMEOUT?
                  if (CurrentEvent.EventParam==Navigate_TIMER){
										TargetRPM = 0; // Set the target RPM to 0
										HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN; //Stop Periodic Interrupt
										SetPWMDuty1(0); 
										SetPWMDuty2(0);
										ResetSumError();
										ResetLastPeriod();
//										ES_Event NewEvent;
//										NewEvent.EventType = RobotStopped;
//										PostRobotSM(NewEvent);
										NextState = Stop;// Set  NextState to Stop
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
       RunNavigationSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunNavigationSM(EntryEventKind);
     }
     return(ReturnEvent);
}


void StartNavigationSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = Stop;
   }
   // call the entry function (if any) for the ENTRY_STATE
   //RunSendingCommandSM(CurrentEvent); // No entry function
}

NavigationState_t QueryNavigationSM ( void )
{
   return(CurrentState);
}



/***************************************************************************
 public functions
 ***************************************************************************/
uint32_t GetTargetRPM(void)
{
	// Function to return the target RPM
	 return TargetRPM; 
}

/***************************************************************************
 private functions
 ***************************************************************************/
/*========= Function to set the direction of wheel 1 to move forward===========*/
void Wheel1Forward(void)
{
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) |= BIT4HI; // Set PB4 is low
}

/*========= Function to set the direction of wheel 1 to move forward===========*/
void Wheel1Reverse(void)
{
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= ~BIT4HI; // Set PB4 is high
}

/*========= Function to set the direction of wheel 1 to move forward===========*/
void Wheel2Forward(void)
{
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) |= BIT5HI; // Set PB5 is low
}

/*========= Function to set the direction of wheel 1 to move forward===========*/
void Wheel2Reverse(void)
{
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= ~ BIT5HI; // Set PB5 is high
}

void ResetTargetRPM (uint32_t NewVal)
{
	TargetRPM =NewVal;
}
