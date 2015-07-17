#ifndef ObstacleSM_H
#define ObstacleSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"


// typedefs for the states
// State definitions for use with the query function
typedef enum { WaitingOb, RotationStop, ClimbUp, ClimbDown, ForwardStop, MovingForwardOb, RotatingOb } ObstacleState_t ;


// Public Function Prototypes

ES_Event RunObstacleSM( ES_Event CurrentEvent );
void StartObstacleSM ( ES_Event CurrentEvent );
ObstacleState_t QueryObstacleSM ( void );

#endif /*SHMTemplate_H */

