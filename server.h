/*
 *   XFrisk - The classic board game for X
 *   Copyright (C) 1993-1999 Elan Feingold (elan@aetherworks.com)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   $Id: server.h,v 1.6 1999/12/26 21:53:17 morphy Exp $
 *
 *   $Log: server.h,v $
 *   Revision 1.6  1999/12/26 21:53:17  morphy
 *   Comment fixes, logical grouping & commenting of function prototypes
 *
 *   Revision 1.5  1999/12/25 20:15:10  morphy
 *   Added file comment and log header
 *
 *
 */

/** \file
 * Interface definitions for server
 */

#ifndef _SERVER
#define _SERVER

#include "types.h"

/* Server states */
#define SERVER_REGISTERING  0
#define SERVER_FORTIFYING   1
#define SERVER_PLAYING      2

/* Initialization, reset and game startup */
void     SRV_Init(void);
void     SRV_Reset(void);
void     SRV_PlayGame(void);

/* Message broadcast & data replication */
void     SRV_BroadcastTextMessage(CString strMessage);
void     SRV_BroadcastMessage(Int32 iMessType, void *pvMessage);
void     SRV_CompleteBroadcast(Int32 iClientExclude, Int32 iMessType, 
			       void *pvMess);
void     SRV_Replicate(Int32 iMessType, void *pvMess, Int32 iType, Int32 iSrc);

/* Species handling */
Int32    SRV_AllocSpecies(void);
void     SRV_FreeSpecies(Int32 i);

/* Client failure handling & cleanup */
void     SRV_LogFailure(CString strReason, Int32 iCommLink);
void     SRV_RecoverFailures(void);
void     SRV_FreeClient(Int32 iClient);

#endif
