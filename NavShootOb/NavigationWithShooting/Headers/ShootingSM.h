#ifndef ShootingSM_H
#define ShootingSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"


// typedefs for the states
// State definitions for use with the query function
typedef enum { Waiting, Aligning, Loading, Firing } ShootingState_t ;


// Public Function Prototypes

ES_Event RunShootingSM( ES_Event CurrentEvent );
void StartShootingSM ( ES_Event CurrentEvent );
ShootingState_t QueryShootingSM ( void );



#endif /*SHMTemplate_H */

