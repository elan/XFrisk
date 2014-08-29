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
 *   $Id: utils.h,v 1.5 2000/01/04 21:41:53 tony Exp $
 */

#ifndef _UTILS
#define _UTILS

#include <X11/Intrinsic.h>
#include <stdio.h>
#include "types.h"

#ifdef __GNUC__
#define UNUSED(x) ((void)x)
#else
#define UNUSED(x)
#endif

#define QUERY_YES          0
#define QUERY_NO           1
#define QUERY_CANCEL       2
#define QUERY_INPROGRESS  -1

#define PRINT_BLANK       -1

#define PLAYER_NONE       -1

#define CURSOR_PLAY        0
#define CURSOR_WAIT        1

#ifdef ENGLISH
#define SERVERNAME "Server"
#endif
#ifdef FRENCH
#define SERVERNAME "Serveur"
#endif

#define OUT_OF_MEM 1

/* X11 Graphical */
void  UTIL_PrintArmies(Int32 iCountry, Int32 iNumArmies, Int32 iColor);
void  UTIL_DisplayMessage(Int32 iFrom, Int32 iTo, CString strMessage);
void  UTIL_DisplayComment(CString strComment);
void  UTIL_DisplayError(CString strError);
void  UTIL_SetPlayerTurnIndicator(Int32 iPlayer);
void  UTIL_DisplayActionCString(Int32 iState, Int32 iPlayer);
void  UTIL_CenterShell(Widget wCenter, Widget wBase, Int32 *x, Int32 *y);
Int32 UTIL_GetArmyNumber(Int32 iMinArmies, Int32 iMaxArmies, Flag fLetCancel);
Int32 UTIL_PopupDialog(CString strTitle, CString strQuestion, 
		       Int32 iNumOptions, CString strOption1, 
		       CString strOption2, CString strOption3);
void  UTIL_QueryYes(Widget w, XtPointer pData, XtPointer pCalldata);
void  UTIL_QueryNo(Widget w, XtPointer pData, XtPointer pCalldata); 
void  UTIL_QueryCancel(Widget w, XtPointer pData, XtPointer pCalldata);
void  UTIL_LightCountry(Int32 iCountry);
void  UTIL_DarkenCountry(Int32 iCountry);
void  UTIL_RefreshMsgDest(Int32 iNumCStrings);
void  UTIL_SetCursorShape(Int32 iShape);
void  UTIL_DrawNiceLine(Int32 iSrcCountry, Int32 iDstCountry);

/* Notification */
void  UTIL_PlaceNotification(XtPointer msgMess, XtIntervalId *pId);
void  UTIL_AttackNotification(XtPointer msgMess, XtIntervalId *pId);
void  UTIL_MoveNotification(XtPointer msgMess, XtIntervalId *pId);

/* Non-X11 */
void    UTIL_ServerEnterState(Int32 iNewState);
Flag    UTIL_PlayerIsLocal(Int32 iPlayer);
Int32   UTIL_NumPlayersAtClient(Int32 iClient);
void    UTIL_ExitProgram(Int32 iExitValue);
FILE   *UTIL_OpenFile(CString strName, CString strOptions);

#endif


