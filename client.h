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
 *   $Id: client.h,v 1.6 1999/11/13 23:08:32 tony Exp $
 */

#ifndef _CLIENT_H
#define _CLIENT_H

#include "types.h"

void   CLNT_Init(int argc, CString *argv);
void   CLNT_Connect(void);
Int32  CLNT_AllocPlayer(void (*pfMsgHandler)(Int32, void *));
void   CLNT_FreePlayer(Int32 i);
void   CLNT_RecoverFailure(CString strReason, Int32 iCommLink);
void   CLNT_PreViewVector(Int32 iMessType, void *pMess);
void   CLNT_PostViewVector(Int32 iMessType, void *pMess);

/* Methods */
Int32  CLNT_GetCommLinkOfClient(Int32 iNumClient);
void   CLNT_SetCommLinkOfClient(Int32 iNumClient, Int32 iCommLink);
Int32  CLNT_GetLightCountOfCountry(Int32 iCountry);
void   CLNT_SetLightCountOfCountry(Int32 iCountry, Int32 iCount);
Int32  CLNT_GetThisClientID(void);

#endif
