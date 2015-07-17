/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef SendingByteSM_H
#define SendingByteSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum { WaitByte, WaitForEOT, WaitFor2ms } SendingByteState_t ;


// Public Function Prototypes

ES_Event RunSendingByteSM( ES_Event CurrentEvent );
void StartSendingByteSM ( ES_Event CurrentEvent );
SendingByteState_t QuerySendingByteSM ( void );
void ChangeFlag (void);
void ResetIndex (void);
uint32_t *QueryReceivedData(void);

#endif /*SHMTemplate_H */

