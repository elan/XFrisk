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
 *   $Id: clients.h,v 1.9 1999/12/26 01:44:16 morphy Exp $
 *
 *   $Log: clients.h,v $
 *   Revision 1.9  1999/12/26 01:44:16  morphy
 *   Moved data type definitions to clients.c.
 *   Moved _CLIENTS_GetNthClient() declaration to clients.c and made it private to that file.
 *
 *   Revision 1.8  1999/12/26 01:00:54  morphy
 *   Experiment with CVS Log tag and whitespace.
 *
 *   Revision 1.7  1999/12/26 00:59:25  morphy
 *   CVS change log tag added to header.
 */

/** \file
 * Interface definitions for client list.
 */

#include "riskgame.h"
#include "types.h"

Int32 CLIENTS_Alloc(void);
CString CLIENTS_GetAddress(Int32 iNumClient);
Int32 CLIENTS_GetAllocationState(Int32 iNumClient);
Int32 CLIENTS_GetCommLinkOfClient(Int32 iNumClient);
Int32 CLIENTS_GetCommLinkToClient(Int32 iCommLink);
Int32 CLIENTS_GetFailureCount(Int32 iNumClient);
Int32 CLIENTS_GetNumFailures(void);
CString CLIENTS_GetFailureReason(Int32 iNumClient);
Int32 CLIENTS_GetNumClients(void);
Int32 CLIENTS_GetNumClientsStarted(void);
Int32 CLIENTS_GetSpecies(Int32 iNumClient);
Int32 CLIENTS_GetStartState(Int32 iNumClient);
Int32 CLIENTS_GetType(Int32 iNumClient);
void CLIENTS_Init(void);
void CLIENTS_ResetFailure(Int32 iNumClient);
void CLIENTS_SetAddress(Int32 iNumClient, CString strAddress);
void CLIENTS_SetAllocationState(Int32 iNumClient, Int32 iState);
void CLIENTS_SetCommLink(Int32 iNumClient, Int32 iCommLink);
void CLIENTS_SetFailure(Int32 iNumClient, CString strReason);
void CLIENTS_SetNumClients(Int32 iClients);
void CLIENTS_SetNumFailures(Int32 num);
void CLIENTS_SetSpecies(Int32 iNumClient, Int32 iSpecies);
void CLIENTS_SetType(Int32 iNumClient, Int32 iType);
void CLIENTS_SetStartState(Int32 iNumClient, Int32 iState);
void CLIENTS_TellAboutOthers(Int32 iCommLink, Int32 iClientType);

/* EOF */
