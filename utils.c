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
 *   $Id: utils.c,v 1.6 1999/11/13 21:58:32 morphy Exp $
 */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Form.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "network.h"
#include "riskgame.h"
#include "gui-vars.h"
#include "client.h"
#include "colormap.h"
#include "callbacks.h"
#include "game.h"
#include "dice.h"
#include "debug.h"

#define MAX_LINES 5

#define MAX_DIGITS 3
#define MAX_STRING "888"

/* Globals */
static Int32    iQueryResult;
static Cursor   cursorWait=0, cursorPlay=0;

/* Private functions */
void UTIL_SetCountryBrightness(Int32 iCountry, Boolean fLight);

/************************************************************************ 
 *  FUNCTION: UTIL_PrintArmies
 *  HISTORY: 
 *     01.28.94  ESF  Created.
 *     02.05.94  ESF  Added number centering (finally!)
 *     03.30.94  ESF  Changed EXTRA_Y to be smaller.
 *     04.02.94  ESF  Fixed font problem, not XSetFont'ing.
 *     05.05.94  ESF  Fixed so that 0 armies doesn't print anything.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_PrintArmies(Int32 iCountry, Int32 iNumArmies, Int32 iColor)
{
  Int32   x, y, iWidth, iHeight;
  Char    strNumber[4];

  /* Is number of armies right? */
  if (iNumArmies>999)
    {
#ifdef ENGLISH
      (void)UTIL_PopupDialog("Warning", "UTIL: Too many armies", 
			     1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
      (void)UTIL_PopupDialog("Attention", "UTIL: Trop d'armées", 
			     1, "Oui", NULL, NULL);
#endif
      return;
    }
  
  /* Setup what we want to print (could be empty) */
  if (iNumArmies>0)
    snprintf(strNumber, sizeof(strNumber), "%d", iNumArmies);
  else
    snprintf(strNumber, sizeof(strNumber), " ");

  /* Erase the old number, assume three digits */
  x       = RISK_GetTextXOfCountry(iCountry);
  y       = RISK_GetTextYOfCountry(iCountry);
  iHeight = (pFont->max_bounds.ascent + pFont->max_bounds.descent);
  iWidth  = XTextWidth(pFont, MAX_STRING, MAX_DIGITS);

  /* Draw erasing rectangle on screen and pixmap */
  XSetForeground(hDisplay, hGC, COLOR_QueryColor(COLOR_CountryToColor(iCountry)));
  XFillRectangle(hDisplay, hWindow, hGC, x, y-iHeight, 
		 iWidth, iHeight+1);
  XFillRectangle(hDisplay, pixMapImage, hGC, x, y-iHeight, 
		 iWidth, iHeight+1);

  /* Center the number, based on the number of digits */
  if (strlen(strNumber)==1)
    x += iWidth/3;
  else if (strlen(strNumber)==2)
    x += iWidth/4;

  /* Finally draw the text in color requested. */
  XSetForeground(hDisplay, hGC, iColor);
  XSetFont(hDisplay, hGC, pFont->fid);

  if (iNumArmies>=0)
    {
      XDrawString(hDisplay, hWindow, hGC, x, y, strNumber, 
		  strlen(strNumber));
      XDrawString(hDisplay, pixMapImage, hGC, x, y, strNumber, 
		  strlen(strNumber));
    }

  /* Flush the queue of requests */
  XFlush(hDisplay);
}


/************************************************************************ 
 *  FUNCTION: UTIL_DisplayMessage
 *  HISTORY: 
 *     02.04.94  ESF  Created.
 *     01.15.95  ESF  Fixed memory leak.
 *     04.30.95  ESF  Got rid of newlines in incoming message.
 *     04.30.95  ESF  Improved multiline message handling.
 *     29.08.95  JC   Put the newline in the beginning of the message.
 *     29.08.95  JC   strFrom -> iFrom, added iTo.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_DisplayMessage(Int32 iFrom, Int32 iTo, CString strMessage)
{
  static Flag fFirst = TRUE;
  Int32         i, iCount;
  CString       strOldMessage, strNewMessage;
  const char *strFrom = NULL, *strNewLine = "", *strTo = "";
  int has_a_to = 1;
  size_t len;

#ifdef ENGLISH
  const char *strToHead = "To ";
#endif
#ifdef FRENCH
  const char *strToHead = "à ";
#endif

  /* Get the old text */
  XtVaGetValues(wMsgText, XtNstring, &strOldMessage, NULL);
  
  /* Remove any newlines from new message */
  for (i=0, iCount=strlen(strMessage); i!=iCount; i++)
    if (strMessage[i]=='\n')
      strMessage[i]=' ';

  /* Build up the new message */
  if (fFirst)
      fFirst = FALSE;
  else
      strNewLine = "\n";

  switch (iFrom)
    {
    case FROM_SERVER:
#ifdef ENGLISH
      strFrom = "Server";
#endif
#ifdef FRENCH
      strFrom = "Serveur";
#endif
      break;
    case FROM_UNKNOW:
#ifdef ENGLISH
      strFrom = "Unknown";
#endif
#ifdef FRENCH
      strFrom = "Inconnu";
#endif
      break;
    default:
      D_Assert(iFrom >= -1 && iFrom < MAX_PLAYERS, "Bogus sender!");
      strFrom = RISK_GetNameOfPlayer(iFrom);
    }

  switch (iTo)
    {
    case DST_ALLPLAYERS:
#ifdef ENGLISH
      strTo = "Everybody";
#endif
#ifdef FRENCH
      strTo = "tous";
#endif
      break;
    case DST_OTHER:
      has_a_to = 0;
      break;
    default:
      strTo = RISK_GetNameOfPlayer(iTo);
    }

  len = strlen(strOldMessage) + strlen(strNewLine) + strlen(strFrom) + 2
	+ 1 + strlen(strToHead) + strlen(strTo) + 4 + strlen(strMessage) + 1;
  strNewMessage = (CString)MEM_Alloc(len);

  snprintf(strNewMessage, len, 
	   "%s%s%s: %s%s%s%s%s%s", strOldMessage, strNewLine, strFrom,
	   has_a_to ? "("       : "",
	   has_a_to ? strToHead : "",
	   has_a_to ? strTo     : "",
	   has_a_to ? ") \""    : "",
	   strMessage,
	   has_a_to ? "\""      : "");

  /* Set the new message, go to the end */
  XtVaSetValues(wMsgText, XtNstring, strNewMessage, NULL);
  XawTextSetInsertionPoint(wMsgText, strlen(strNewMessage));

  MEM_Free(strNewMessage);
}


/************************************************************************ 
 *  FUNCTION: UTIL_DisplayComment
 *  HISTORY: 
 *     02.07.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_DisplayComment(CString strComment)
{
  Arg     pArgs[1];
  Int32   iCount;

  iCount = 0;
  XtSetArg(pArgs[iCount], XtNlabel, strComment); iCount++;
  XtSetValues(wCommentLabel, pArgs, iCount);
}



/************************************************************************ 
 *  FUNCTION: UTIL_SetPlayerTurnIndicator
 *  HISTORY: 
 *     03.03.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_SetPlayerTurnIndicator(Int32 iPlayer)
{
  UNUSED(iPlayer);

  D_Assert(FALSE, "Unimplemented!");
}


/************************************************************************ 
 *  FUNCTION: UTIL_DisplayActionCString
 *  HISTORY: 
 *     03.03.94  ESF  Created.
 *     03.29.94  ESF  Added player attack mode history.
 *     03.30.94  ESF  Fixed bug, not erasing iAttackSrc.
 *     04.02.94  ESF  Added clearing of error.
 *     05.07.94  ESF  Removed clearing of error.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_DisplayActionCString(Int32 iState, Int32 iPlayer)
{
  char buf[256];
  switch (iState)
    {
    case STATE_REGISTER:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "Waiting for all clients to register...");
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "Attent que tous les clients aient fini de s'enregistrer...");
#endif
      break;
      
    case STATE_FORTIFY:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s to place an army (%d remaining)...", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s place une armée (reste %d)...", 
#endif
	      RISK_GetNameOfPlayer(iPlayer),
	      RISK_GetNumArmiesOfPlayer(iPlayer));
      XawListHighlight(wActionList, iActionState = ACTION_PLACE);
      if (UTIL_PlayerIsLocal(iPlayer))
        {
          XawListHighlight(wAttackList,
		           RISK_GetDiceModeOfPlayer(iCurrentPlayer));
          XawListHighlight(wMsgDestList,
		           RISK_GetMsgDstModeOfPlayer(iCurrentPlayer));
        }
      break;

      
    case STATE_PLACE:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s placing armies (%d remaining)...", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s place des armées (reste %d)...", 
#endif
	      RISK_GetNameOfPlayer(iPlayer),
	      RISK_GetNumArmiesOfPlayer(iPlayer));
      XawListHighlight(wActionList, iActionState = ACTION_PLACE);
      if (UTIL_PlayerIsLocal(iPlayer))
        {
          XawListHighlight(wAttackList,
		           RISK_GetDiceModeOfPlayer(iCurrentPlayer));
          XawListHighlight(wMsgDestList,
		           RISK_GetMsgDstModeOfPlayer(iCurrentPlayer));
        }
      break;

    case STATE_ATTACK:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s attacking...", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s attaque...", 
#endif
	      RISK_GetNameOfPlayer(iPlayer));
      iActionState = RISK_GetAttackModeOfPlayer(iCurrentPlayer);
      XawListHighlight(wActionList, 
		       RISK_GetAttackModeOfPlayer(iCurrentPlayer));
      if (UTIL_PlayerIsLocal(iPlayer))
        {
          XawListHighlight(wAttackList,
		           RISK_GetDiceModeOfPlayer(iCurrentPlayer));
          XawListHighlight(wMsgDestList,
		           RISK_GetMsgDstModeOfPlayer(iCurrentPlayer));
        }
      break;

    case STATE_MOVE:
      if (GAME_AttackFrom() >= 0)
	{
	  UTIL_DarkenCountry(GAME_AttackFrom());
	  GAME_SetAttackSrc(-1);
	}

#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s executing a free move...", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s déplace...", 
#endif
	      RISK_GetNameOfPlayer(iPlayer));
      XawListHighlight(wActionList, iActionState = ACTION_MOVE);
      if (UTIL_PlayerIsLocal(iPlayer))
        {
          XawListHighlight(wAttackList,
		           RISK_GetDiceModeOfPlayer(iCurrentPlayer));
          XawListHighlight(wMsgDestList,
		           RISK_GetMsgDstModeOfPlayer(iCurrentPlayer));
        }
      break;

    default:
      D_Assert(FALSE, "Shouldn't be here!");
    }

  UTIL_DisplayComment(buf);
}


/************************************************************************ 
 *  FUNCTION: UTIL_DisplayError
 *  HISTORY: 
 *     03.04.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_DisplayError(CString strError)
{
  Arg     pArgs[1];
  Int32   iCount;
  
  iCount=0;
  XtSetArg(pArgs[iCount], XtNlabel, strError); iCount++;
  XtSetValues(wErrorLabel, pArgs, iCount);
}


/************************************************************************ 
 *  FUNCTION: UTIL_ServerEnterState
 *  HISTORY: 
 *     03.05.94  ESF  Created.
 *     11.16.94  ESF  Added to send messages to other clients.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_ServerEnterState(Int32 iNewState)
{
  MsgEnterState   m;
  MsgNetMessage   msgNetMessage;
  char buf[256];

  switch (iNewState)
    {
    case STATE_REGISTER:
      break;
    case STATE_PLACE:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s is placing %s.", 
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      (RISK_GetNumArmiesOfPlayer(iCurrentPlayer) > 1 ? 
	       "armies" :
	       "an army"));
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s place %s armée%s.", 
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      (RISK_GetNumArmiesOfPlayer(iCurrentPlayer)>1)?"des":"une",
	      (RISK_GetNumArmiesOfPlayer(iCurrentPlayer)>1)?"s":"");
#endif
      break;
    case STATE_FORTIFY:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s is fortifying territories.", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s fortifie des territoires.", 
#endif
	      RISK_GetNameOfPlayer(iCurrentPlayer));
      break;
    case STATE_ATTACK:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s is attacking.", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s attaque.", 
#endif
	      RISK_GetNameOfPlayer(iCurrentPlayer));
      break;
    case STATE_MOVE:
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s is moving armie(s).", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s déplace des armée(s).", 
#endif
	      RISK_GetNameOfPlayer(iCurrentPlayer));
      break;
    default:
      D_Assert(0, "Bogus state!");
    }

  msgNetMessage.strMessage = buf;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			 MSG_NETMESSAGE, &msgNetMessage);

  /* Change states and let the server know about it */
  m.iState = iNewState;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_ENTERSTATE, &m);
}


/* Utility for the next function */
#define UTIL_CloseArmyDialog() \
 UTIL_DisplayError(""); \
 XtSetKeyboardFocus(wToplevel, wToplevel); \
 XtRemoveGrab(wArmiesShell); \
 XtUnrealizeWidget(wArmiesShell); 

/************************************************************************ 
 *  FUNCTION: UTIL_GetArmyNumber
 *  HISTORY: 
 *     03.16.94  ESF  Created.
 *     03.28.94  ESF  Fixed minor printing bug.
 *     04.01.94  ESF  Fixed bug in centering shell, had XtNy with an x.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 UTIL_GetArmyNumber(Int32 iMinArmies, Int32 iMaxArmies, Flag fLetCancel)
{
  Int32        x, y, iNumArmies;
  XEvent       xEvent;
  CString      strBuffer;
  char buf[256];

  /* Make sure that the range is valid */
  D_Assert(iMinArmies<=iMaxArmies, "Invalid range for army values!");

  UTIL_CenterShell(wArmiesShell, wToplevel, &x, &y);
  XtVaSetValues(wArmiesShell, 
		XtNallowShellResize, False,
		XtNx, x, 
		XtNy, y, 
		XtNborderWidth, 1,
		XtNtitle, "Frisk",
		NULL);

  /* Set the default number of armies as the maximum */
  snprintf(buf, sizeof(buf), "%d", iMaxArmies);
  XtVaSetValues(wArmiesText, XtNstring, buf, NULL);

  /* Don't display the cancel button if there is no chance of this */
  XtVaSetValues(wCancelArmiesButton, XtNsensitive, 
		fLetCancel ? True : False, NULL);

  /* Pop it up */
  XtMapWidget(wArmiesShell);
  XtAddGrab(wArmiesShell, True, True);
  XtSetKeyboardFocus(wToplevel, wArmiesShell); 
  XtSetKeyboardFocus(wArmiesShell, wArmiesText);

 keep_going:
  iQueryResult = QUERY_INPROGRESS;
  while (iQueryResult == QUERY_INPROGRESS) 
    {
      /* pass events */
      XNextEvent(hDisplay, &xEvent);
      XtDispatchEvent(&xEvent);
    }

  /* User must have selected one of the buttons */
  if (!fLetCancel && iQueryResult == QUERY_NO)
    goto keep_going;
  else if (iQueryResult == QUERY_NO)
    {
      UTIL_CloseArmyDialog();
      return(0);
    }
  
  /* Get number of armies */
  XtVaGetValues(wArmiesText, XtNstring, &strBuffer, NULL);
  iNumArmies = atoi(strBuffer);

  if (iNumArmies<iMinArmies || iNumArmies>iMaxArmies)
    {
      if (iNumArmies < iMinArmies)
#ifdef ENGLISH
	snprintf(buf, sizeof(buf), "You must move at least %d armie%s.",
#endif
#ifdef FRENCH
	snprintf(buf, sizeof(buf), "Il faut déplacer au moins %d armée%s.",
#endif
                iMinArmies, (iMinArmies>1)?"s":"");
      else
#ifdef ENGLISH
	snprintf(buf, sizeof(buf), "You can't move more than %d armie%s.",
#endif
#ifdef FRENCH
	snprintf(buf, sizeof(buf), "Impossible de déplacer plus de %d armée%s.",
#endif
                iMaxArmies, (iMaxArmies>1)?"s":"");
      UTIL_DisplayError(buf);
      
      goto keep_going;
    }

  UTIL_CloseArmyDialog();
  return(iNumArmies);
}


void UTIL_QueryYes(Widget w, XtPointer pData, XtPointer pCalldata) {
  UNUSED(w);
  UNUSED(pData);
  UNUSED(pCalldata);

  iQueryResult = QUERY_YES; 
}

void UTIL_QueryNo(Widget w, XtPointer pData, XtPointer pCalldata) {
  UNUSED(w);
  UNUSED(pData);
  UNUSED(pCalldata);

  iQueryResult = QUERY_NO; 
}

void UTIL_QueryCancel(Widget w, XtPointer pData, XtPointer pCalldata) { 
  UNUSED(w);
  UNUSED(pData);
  UNUSED(pCalldata);

  iQueryResult = QUERY_CANCEL; 
}


/************************************************************************ 
 *  FUNCTION: UTIL_CenterShell
 *  HISTORY: 
 *     03.16.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_CenterShell(Widget wCenter, Widget wBase, Int32 *x, Int32 *y)
{
  Dimension  dimWidth, dimHeight, dimTopWidth, dimTopHeight;
  Window     hDummyWindow;

  /* Make sure the two widgets are realized */
  if (!XtIsRealized(wCenter))
    XtRealizeWidget(wCenter);
  if (!XtIsRealized(wBase))
    XtRealizeWidget(wBase);

  /* Get the dimensions of the shells and center the popup. */
  XtVaGetValues(wCenter, 
		XtNwidth, &dimWidth, 
		XtNheight, &dimHeight, NULL);
  XtVaGetValues(wBase, 
		XtNwidth, &dimTopWidth, 
		XtNheight, &dimTopHeight, NULL);
  
  XTranslateCoordinates(hDisplay, XtWindow(wBase), 
			XDefaultRootWindow(hDisplay),
			(Int32)(dimTopWidth-dimWidth)/2, 
			(Int32)(dimTopHeight-dimHeight)/2, 
			x, y, &hDummyWindow);
}


/************************************************************************ 
 *  FUNCTION: UTIL_LightCountry
 *  HISTORY: 
 *     03.28.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_LightCountry(Int32 iCountry)
{
  UTIL_PrintArmies(iCountry,
		   RISK_GetNumArmiesOfCountry(iCountry),
		   WhitePixel(hDisplay, 0));
}


/************************************************************************ 
 *  FUNCTION: UTIL_DarkenCountry
 *  HISTORY: 
 *     03.28.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_DarkenCountry(Int32 iCountry)
{
  UTIL_PrintArmies(iCountry, 
		   RISK_GetNumArmiesOfCountry(iCountry),
		   BlackPixel(hDisplay, 0));
}


/************************************************************************ 
 *  FUNCTION: UTIL_PopupDialog
 *  HISTORY: 
 *     04.02.94  ESF  Created.
 *     05.12.94  ESF  Revamped.
 *     10.30.94  ESF  Added sanity check for hDisplay.
 *     01.25.95  ESF  Added code to center buttons.
 *     01.25.95  ESF  Changed the default title to "Frisk".
 *     04.30.95  ESF  Fixed a bug with resizing of the label.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 UTIL_PopupDialog(CString strTitle, CString strLabel, Int32 iNumOptions, 
		       CString strOption1, CString strOption2, 
		       CString strOption3)
{
  CString               pstrOptions[3];
  Int32                 i, x, y, iWidth;
  XEvent                xEvent;
  static XFontStruct   *pLabelFont=NULL;

  /* If this got called before the display is set up */
  if (!hDisplay)
    {
      printf("%s: %s\n", strTitle, strLabel);
      return 0;
    }

  /* Initialize these for later */
  pstrOptions[0] = strOption1;  
  pstrOptions[1] = strOption2;  
  pstrOptions[2] = strOption3;

  /* Get the font if necessary */
  if (pLabelFont==NULL)
    if ((pLabelFont=XLoadQueryFont(hDisplay, "*helvetica-b*-o-*14*"))==NULL)
      {
	printf("Can't find the font \"*helvetica-b*-o-*14*\"!!\n");
	UTIL_ExitProgram(-1);
      }

  /* Sanity check */
  if (iNumOptions>3 || iNumOptions<1)
    {
      (void)UTIL_PopupDialog("Warning", "UTIL: Wacked # of options", 
			     1, "Ok", NULL, NULL);
      return(-1);
    }

  /* Make sure the dialog is realized */
  if(!XtIsRealized(wDialogShell))
     XtRealizeWidget(wDialogShell);

  /* Set the title */
  XtVaSetValues(wDialogShell, 
		XtNtitle, strTitle == NULL ? "Frisk" : strTitle,
		XtNallowShellResize, True,
		NULL);

  /* Make sure that the label is big enough to hold the string, and
   * if the length of the button strings are larger, make it bigger.
   * Let there be a minimum width of 200...  There are a few magic 
   * numbers here, excuse them :)
   */
  
  iWidth = MAX(200,
	       MAX(XTextWidth(pLabelFont, strLabel, strlen(strLabel))+50,
		   XTextWidth(pLabelFont, strOption1, 
			      strOption1 ? strlen(strOption1) : 0)+
		   XTextWidth(pLabelFont, strOption2, 
			      strOption2 ? strlen(strOption2) : 0)+
		   XTextWidth(pLabelFont, strOption3, 
			      strOption3 ? strlen(strOption3) : 0)+
		   140));

  /* Set the button resources */
  for (i=0; i!=3; i++)
    if (iNumOptions>i)
      {
	XtMapWidget(wDialogButton[i]);
	XtVaSetValues(wDialogButton[i], 
		      XtNlabel, pstrOptions[i]==NULL ? "Null": pstrOptions[i], 
		      XtNfromHoriz, i>0 ? wDialogButton[i-1] : NULL,
		      NULL);
      }
    else
      {
	XtUnmapWidget(wDialogButton[i]);
	XtVaSetValues(wDialogButton[i],
		      XtNfromHoriz, 0, 
		      NULL); 
      }

  /* We need to unrealize it so that the changes will take place (?) */
  XtUnrealizeWidget(wDialogShell);

  /* Center and resize the shell, and then actually pop the dialog up */
  XtVaSetValues(wDialogShell, XtNwidth, iWidth, NULL);
  XtVaSetValues(wDialogLabel, 
		XtNlabel, strLabel,
		XtNwidth, iWidth,
		NULL);

  UTIL_CenterShell(wDialogShell, wToplevel, &x, &y);
  XtVaSetValues(wDialogShell, 
		XtNx, x, 
		XtNy, y,
		XtNborderWidth, 1,
		NULL);

  XtPopup(wDialogShell, XtGrabExclusive);
  
  iQueryResult = QUERY_INPROGRESS;
  while (iQueryResult == QUERY_INPROGRESS) 
    {
      /* pass events */
      XNextEvent(hDisplay, &xEvent);
      XtDispatchEvent(&xEvent);
    }

  /* User must have selected one of the buttons */
  XtPopdown(wDialogShell);
  return(iQueryResult);  
}


/************************************************************************ 
 *  FUNCTION: UTIL_PlayerIsLocal
 *  HISTORY: 
 *     04.11.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag UTIL_PlayerIsLocal(Int32 iPlayer)
{
  return (RISK_GetClientOfPlayer(iPlayer) == CLNT_GetThisClientID() ? TRUE : FALSE);
}


/************************************************************************ 
 *  FUNCTION: UTIL_NumPlayersAtClient
 *  HISTORY: 
 *     05.04.94  ESF  Created.
 *     05.13.95  ESF  Fixed bug, wasn't checking if player was allocated.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 UTIL_NumPlayersAtClient(Int32 iClient)
{
  Int32 i, iCount;

  /* It's an error to call this with an out of range number */
  D_Assert(iClient>=0 && iClient<MAX_CLIENTS, "Client is bogus!");

  for (i=iCount=0; i!=MAX_PLAYERS; i++)
    if (RISK_GetAllocationStateOfPlayer(i) == ALLOC_COMPLETE &&
	RISK_GetClientOfPlayer(i) == iClient)
      iCount++;

  return iCount;
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     05.06.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_RefreshMsgDest(Int32 iNumCStrings)
{
  /* Update the listbox */
  XtVaSetValues(wMsgDestList, 
		XtNlist, pstrMsgDstCString,
		XtNnumberStrings, iNumCStrings,
		NULL);
}


/************************************************************************ 
 *  FUNCTION: UTIL_ExitProgram
 *  HISTORY: 
 *     06.16.94  ESF  Created.
 *     08.03.94  ESF  Fixed to send server message before exiting.
 *     08.28.94  ESF  Changed to close sockets.
 *     10.02.94  ESF  Changed to not close sockets.
 *     25.08.95  JC   Changed to not send server message if closed.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_ExitProgram(Int32 iExitValue)
{
  if (CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()) != -1)
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_DEREGISTERCLIENT, NULL);
  MEM_TheEnd();
  exit(iExitValue);
}


/************************************************************************ 
 *  FUNCTION: UTIL_SetCursorShape
 *  HISTORY: 
 *     10.01.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_SetCursorShape(Int32 iShape)
{
  /* If the cursors haven't been created then create them */
  if (cursorPlay==0)
    cursorPlay = XCreateFontCursor(hDisplay, XC_crosshair);
  if (cursorWait==0)
    cursorWait = XCreateFontCursor(hDisplay, XC_watch);
  
  /* Switch to the desired cursor */
  switch (iShape)
    {
    case CURSOR_WAIT:
      XDefineCursor(hDisplay, hWindow, cursorWait);
      break;
      
    case CURSOR_PLAY:
      XDefineCursor(hDisplay, hWindow, cursorPlay);
      break;

    default:
      D_Assert(FALSE, "Bogus cursor being passed in.");
    }
}


/************************************************************************ 
 *  FUNCTION: UTIL_PlaceNotification
 *  HISTORY: 
 *     10.30.94  ESF  Created.
 *     11.06.94  ESF  Completed.
 *     01.15.95  ESF  Fixed memory leak.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_PlaceNotification(XtPointer msgMess, XtIntervalId *pId)
{
  MsgPlaceNotify  *msg = (MsgPlaceNotify *)msgMess;
  UTIL_SetCountryBrightness(msg->iCountry, pId == NULL ? TRUE : FALSE);
  
  /* If it's the second time we are being called then there will
   * be a pId and we can free the memory taken by it.
   */

  if (pId != NULL)
    MEM_Free(msg);
}


/************************************************************************ 
 *  FUNCTION: UTIL_AttackNotification
 *  HISTORY: 
 *     10.30.94  ESF  Created.
 *     11.16.94  ESF  Finished.
 *     01.15.95  ESF  Fixed memory leak.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_AttackNotification(XtPointer msgMess, XtIntervalId *pId)
{
  MsgAttackNotify  *msg = (MsgAttackNotify *)msgMess;
  UTIL_SetCountryBrightness(msg->iSrcCountry, pId == NULL ? TRUE : FALSE);
  UTIL_DrawNiceLine(msg->iSrcCountry, msg->iDstCountry);

  /* If it's the second time we are being called then there will
   * be a pId and we can free the memory taken by it.
   */

  if (pId != NULL)
    MEM_Free(msg);
}


/************************************************************************ 
 *  FUNCTION: UTIL_MoveNotification
 *  HISTORY: 
 *     10.30.94  ESF  Created.
 *     11.16.94  ESF  Finished.
 *     01.15.95  ESF  Fixed memory leak.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_MoveNotification(XtPointer msgMess, XtIntervalId *pId)
{
  MsgMoveNotify  *msg = (MsgMoveNotify *)msgMess;
  UTIL_SetCountryBrightness(msg->iSrcCountry, pId == NULL ? TRUE : FALSE);
  UTIL_DrawNiceLine(msg->iSrcCountry, msg->iDstCountry);

  /* If it's the second time we are being called then there will
   * be a pId and we can free the memory taken by it.
   */

  if (pId != NULL)
    MEM_Free(msg);
}

#define BORDER 2

/************************************************************************ 
 *  FUNCTION: UTIL_DrawNiceLine
 *  HISTORY: 
 *     11.06.94  ESF  Created.
 *     11.16.94  ESF  Finished.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_DrawNiceLine(Int32 iSrcCountry, Int32 iDstCountry)
{
  Int32  x1, y1, x2, y2, iHalfHeight, iHalfWidth;
  Int32  iWidth, iHeight, ixDist, iyDist;

  /* Get the two endpoints of the line */
  x1 = RISK_GetTextXOfCountry(iSrcCountry)-BORDER;
  y1 = RISK_GetTextYOfCountry(iSrcCountry)+BORDER;
  x2 = RISK_GetTextXOfCountry(iDstCountry)-BORDER;
  y2 = RISK_GetTextYOfCountry(iDstCountry)+BORDER;

  /* Get the width and height of the box containing the text. */
  iHeight = (pFont->max_bounds.ascent + pFont->max_bounds.descent)+2*BORDER;
  iWidth  = XTextWidth(pFont, MAX_STRING, MAX_DIGITS)+2*BORDER;

  /* Depending on where the source and destination of the line is, 
   * draw it coming and going from the middle of one of the sides, 
   * of from the corners.  Thus, the line can come and go from eight
   * different points, which should be enough to make it look good.
   */
  
  ixDist = (x1-x2<0) ? (x2-x1) : (x1-x2);
  iyDist = (y1-y2<0) ? (y2-y1) : (y1-y2);

  /* The compiler should do this (CSE), but I'm distrustful by nature */
  iHalfHeight = iHeight / 2;
  iHalfWidth  = iWidth / 2;

  if (ixDist > iyDist)
    {
      /* Use the middles of the sides */
      if (x2 > x1)
	x1+=iWidth, y1-=iHalfHeight, y2-=iHalfHeight;
      else
	x2+=iWidth, y1-=iHalfHeight, y2-=iHalfHeight;
    }
  else
    {
      /* Use the middles of the top and bottom */
      if (y2 > y1)
	x1+=iHalfWidth, x2+=iHalfWidth, y2-=iHeight;
      else
	x1+=iHalfWidth, x2+=iHalfWidth, y1-=iHeight;
    }

  if (!COLOR_IsTrueColor())
    XDrawLine(hDisplay, hWindow, hGC_XOR, x1, y1, x2, y2);
  XFlush(hDisplay);
}


/************************************************************************ 
 *  FUNCTION: UTIL_SetCountryBrightness
 *  HISTORY: 
 *     11.16.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void UTIL_SetCountryBrightness(Int32 iCountry, Boolean fLight)
{
  Int32 iLightRefCount;

  iLightRefCount = CLNT_GetLightCountOfCountry(iCountry);

  /* Light up or darken country, using reference counting. */
  if (fLight)
    {
      if (iLightRefCount == 0)
	UTIL_LightCountry(iCountry);
      CLNT_SetLightCountOfCountry(iCountry, iLightRefCount + 1);
    }
  else
    {
      if (iLightRefCount == 1)
	UTIL_DarkenCountry(iCountry);
      CLNT_SetLightCountOfCountry(iCountry, iLightRefCount - 1);
    }
}


/************************************************************************ 
 *  FUNCTION: UTIL_OpenFile
 *  HISTORY: 
 *     12.31.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
FILE *UTIL_OpenFile(CString strName, CString strOptions)
{
  FILE     *hFile;
  CString   strNewName = NULL;
  Int32     i;

  /* First just try to open the complete file that was passed */
  if ((hFile = fopen(strName, strOptions)) != NULL)
    return hFile;

  /* Construct an alternate name that is the same file 
   * but in the current directory.  Try to open this.
   */
  
  /* Alloc some memory, with room for '\0' and './' */
  strNewName = (CString)MEM_Alloc(strlen(strName)+1+2);
  
  /* Find the last occurance of '/' in the filename */
  for (i=strlen(strName)-1; i>=0 && strName[i]!='/'; i--)
    ; /* TwiddleThumbs() */
  
  /* Create the start of the new pathname */
  strcpy(strNewName, "./");
  
  /* Move the pointer along to the last '/', but don't run off the end */
  strName = strName + MIN(i+1, (Int32)strlen(strName));
  strcat(strNewName, strName);
  
  /* Try to open this file */
  hFile = fopen(strName, strOptions);

  /* Free the memory */
  MEM_Free(strNewName);

  /* Return the handle */
  return hFile;
}
