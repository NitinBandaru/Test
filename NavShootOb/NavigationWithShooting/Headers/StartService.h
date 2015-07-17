#ifndef StartSERVICE_H
#define StartSERVICE_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"


// Public Function Prototypes

bool InitializeStartService(uint8_t Priority);
bool PostStartService (ES_Event ThisEvent);
ES_Event RunStartService (ES_Event ThisEvent);



#endif 

