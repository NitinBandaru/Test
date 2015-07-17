/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef RobotSM_H
#define RobotSM_H

// State definitions for use with the query function
typedef enum { Navigation, Shooting, Obstacle, NavigationTempStop, ShootingTempStop, ObstacleTempStop } RobotState_t ;

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"

// Public Function Prototypes  

ES_Event RunRobotSM( ES_Event CurrentEvent );
void StartRobotSM ( ES_Event CurrentEvent );
bool PostRobotSM( ES_Event ThisEvent );
bool InitRobotSM ( uint8_t Priority );

#endif /*TopHSMTemplate_H */

