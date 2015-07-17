/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef CommunicationSM_H
#define CommunicationSM_H

#define KnobAnalogValue 1	 //remove later
// State definitions for use with the query function
typedef enum { Wait, SendingGameStatus, SendingPosition1, SendingPosition2, SendingPosition3 } CommunicationState_t ;

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"

// Public Function Prototypes  

ES_Event RunCommunicationSM( ES_Event CurrentEvent );
void StartCommunicationSM ( ES_Event CurrentEvent );
bool PostCommunicationSM( ES_Event ThisEvent );
bool InitCommunicationSM ( uint8_t Priority );

#endif /*TopHSMTemplate_H */

