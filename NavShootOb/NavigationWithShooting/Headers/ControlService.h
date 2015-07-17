#ifndef CONTROLSERVICE_H
#define CONTROLSERVICE_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"


// Public Function Prototypes

bool InitializeControlService(uint8_t Priority);
bool PostControlService (ES_Event ThisEvent);
ES_Event RunControlService (ES_Event ThisEvent);
void SetFlag1 (bool flag1);
void SetFlag2 (bool flag2);


#endif 

