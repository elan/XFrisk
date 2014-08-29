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
 *   $Id: callbacks.h,v 1.5 1999/11/13 21:58:30 morphy Exp $
 */

#ifndef _CALLBACKS
#define _CALLBACKS

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "types.h"

void CBK_XIncomingMessage(XtPointer pClientData, int *iSource, XtInputId *id);
void CBK_IncomingMessage(Int32 iMessType, void *pvMessage);
void CBK_RefreshMap(Widget w, XtPointer pData, XtPointer call_data);
void CBK_RefreshDice(Widget w, XtPointer pData, XtPointer call_data);
void CBK_MapClick(Widget w, XEvent *pEvent, String *str, Cardinal *card);
void CBK_Quit(Widget w, XtPointer pData, XtPointer call_data);
void CBK_ShowCards(Widget w, XtPointer pData, XtPointer call_data);
void CBK_CancelCards(Widget w, XtPointer pData, XtPointer call_data);
void CBK_ShowMission(Widget w, XtPointer pData, XtPointer call_data);
void CBK_Attack(Widget w, XtPointer pData, XtPointer call_data);
void CBK_Action(Widget w, XtPointer pData, XtPointer call_data);
void CBK_MsgDest(Widget w, XtPointer pData, XtPointer call_data);
void CBK_SendMessage(void);
void CBK_CancelAttack(Widget w, XtPointer pData, XtPointer call_data);
void CBK_About(Widget w, XtPointer pData, XtPointer call_data);
void CBK_Repeat(Widget w, XtPointer pData, XtPointer call_data);
void CBK_Help(Widget w, XtPointer pData, XtPointer call_data);
void CBK_ExchangeCards(Widget w, XtPointer pData, XtPointer call_data);
void CBK_EndTurn(Widget w, XtPointer pData, XtPointer call_data);
void CBK_ArmiesShell(Widget w, XtPointer pData, XtPointer call_data);
void CBK_HelpOk(Widget w, XtPointer pData, XtPointer call_data);
void CBK_HelpSelectTopic(Widget w, XtPointer pData, XtPointer call_data);
void CBK_Replicate(Int32 iMessType, void *pvMess, Int32 iType, Int32 iSrc);
void CBK_Callback(Int32 iMessType, void *pvMess);

extern Int32      iActionState, iCurrentPlayer;
extern Int32      iState;
extern Flag       fGameStarted;
extern Int32      iReply;
extern Int32	  iFirstPlayer; /* The first fortifier */

/* Relating to the message destination listbox */
extern CString    pstrMsgDstCString[];

#endif

