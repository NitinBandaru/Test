/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef NavigationSM_H
#define NavigationSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"


// typedefs for the states
// State definitions for use with the query function
typedef enum { Stop, MovingForward, Rotating } NavigationState_t ;


// Public Function Prototypes

ES_Event RunNavigationSM( ES_Event CurrentEvent );
void StartNavigationSM ( ES_Event CurrentEvent );
NavigationState_t QueryNavigationSM ( void );
uint32_t GetTargetRPM(void);
void ResetTargetRPM (uint32_t NewVal);
void Wheel1Forward(void);
void Wheel1Reverse(void);
void Wheel2Forward(void);
void Wheel2Reverse(void);


#endif /*SHMTemplate_H */

