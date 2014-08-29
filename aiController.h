/*****************************************************************************
 *                                                                           *
 *  Copyright (c) 1993-1997 Elan Feingold (elan@jeeves.net)                  *
 *                                                                           *
 *     PERMISSION TO USE, COPY, MODIFY, AND TO DISTRIBUTE THIS SOFTWARE      *
 *     AND ITS DOCUMENTATION FOR ANY PURPOSE IS HEREBY GRANTED WITHOUT       *
 *     FEE, PROVIDED THAT THE ABOVE COPYRIGHT NOTICE APPEAR IN ALL           *
 *     COPIES AND MODIFIED COPIES AND THAT BOTH THAT COPYRIGHT NOTICE AND    *
 *     THIS PERMISSION NOTICE APPEAR IN SUPPORTING DOCUMENTATION.  THERE     *
 *     IS NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR      *
 *     ANY PURPOSE.  THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS       *
 *     OR IMPLIED WARRANTY.                                                  *
 *                                                                           *
 *****************************************************************************/

#ifndef _AICONTROLLER
#define _AICONTROLLER

#include "types.h"

/* These routines check that the player is in a valid state */
void AI_Init(void);
void AI_CheckCards(void);
void AI_CheckFortification(void);
void AI_CheckPlacement(void);

#endif
