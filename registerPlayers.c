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
 *   $Id: registerPlayers.c,v 1.8 2000/01/10 22:47:41 tony Exp $
 *
 *
 */
/**
 \file "Player registration" dialog
 */

#include <X11/X.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Viewport.h>
#include <time.h>

#include "registerPlayers.h"
#include "addPlayer.h"
#include "colorEdit.h"
#include "gui-vars.h"
#include "riskgame.h"
#include "utils.h"
#include "debug.h"
#include "client.h"


/* Private functions */
void   REG_EditColor(Widget w, XEvent *pEvent, String *str, Cardinal *card);
void   REG_PerformSelect(Int32 iSlot, Flag fEraseOthers);
void   REG_RenderSlot(Int32 iSlot, Int32 iPlayer);
Flag   REG_IsSelected(Int32 iSlot);
Int32  REG_PlayerToSlot(Int32 iPlayer);
void   REG_DeletePlayer(void);
void   REG_AddPlayer(void);
void   REG_PlayerCreated(Int32 iPlayer);
void   REG_PlayerDestroyed(Int32 iPlayer);
void   REG_Done(void);
void   REG_Quit(void);

#define REG_SelectSlot(slot) \
  XtVaSetValues(wShowPlayerForm[slot], XtNborderColor, iSelectColor, NULL)
#define REG_UnselectSlot(slot) \
  XtVaSetValues(wShowPlayerForm[slot], XtNborderColor, iUnselectColor, NULL)

/* Action tables */
static XtActionsRec actionTable[] =
{
  { "regMouseClick", REG_MouseClick },
  { "regMouseShiftClick", REG_MouseShiftClick },
  { "regEditColor", REG_EditColor },
  { NULL, NULL }
};

static Int32  piPositionToPlayer[MAX_PLAYERS];
static Int32  iSelectColor, iUnselectColor;
static Cursor normalCursor, handCursor;

static Widget wRegisterShell, wRegisterForm, wPlayerViewport, wPlayerForm;
static Widget wShowPlayerColor[MAX_PLAYERS], wShowPlayerName[MAX_PLAYERS];
static Widget wShowPlayerSpecies[MAX_PLAYERS], wShowPlayerForm[MAX_PLAYERS];
static Widget wAddPlayerButton, wDeletePlayerButton;
static Widget wRegisterOKButton, wRegisterNOButton;


/************************************************************************ 
 *  FUNCTION: REG_BuildDialog
 *  HISTORY: 
 *     01.24.95  ESF  Created.
 *     01.29.95  ESF  Finished coding.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_BuildDialog(void)
{
  Int32 i;

  /* A cursor to use */
  handCursor   = XCreateFontCursor(hDisplay, XC_hand1);
  normalCursor = XCreateFontCursor(hDisplay, XC_left_ptr);
  iSelectColor = BlackPixel(hDisplay, 0);

  wRegisterShell = XtCreatePopupShell("wRegisterShell", 
				      transientShellWidgetClass,
				      wToplevel, pVisualArgs, iVisualCount);
  wRegisterForm = XtCreateManagedWidget("wRegisterForm", formWidgetClass, 
					wRegisterShell, NULL, 0);
  
  /* Where the players are displayed */
  wPlayerViewport = XtCreateManagedWidget("wPlayerViewport", 
					  viewportWidgetClass, 
					  wRegisterForm, NULL, 0);
  wPlayerForm = XtCreateManagedWidget("wPlayerForm",
				      boxWidgetClass, 
				      wPlayerViewport, NULL, 0);
  
  /* Each player */
  for (i=0; i!=MAX_PLAYERS; i++)
    {
      wShowPlayerForm[i] = XtVaCreateManagedWidget("wShowPlayerForm",
						   formWidgetClass, 
						   wPlayerForm,
						   NULL);
      if (i)
	XtVaSetValues(wShowPlayerForm[i], XtNfromVert, 
		      wShowPlayerForm[i-1], NULL);
      wShowPlayerColor[i] = XtVaCreateManagedWidget("wShowPlayerColor",
						    labelWidgetClass, 
						    wShowPlayerForm[i],
						    NULL);
      wShowPlayerName[i] = XtVaCreateManagedWidget("wShowPlayerName",
						   labelWidgetClass,
						   wShowPlayerForm[i], 
						   XtNfromHoriz, 
						   wShowPlayerColor[i],
						   NULL);
      wShowPlayerSpecies[i] = XtVaCreateManagedWidget("wShowPlayerSpecies",
						      labelWidgetClass, 
						      wShowPlayerForm[i], 
						      XtNfromHoriz, 
						      wShowPlayerName[i],
						      NULL);
    }
  
  /* The buttons */
  wAddPlayerButton = XtCreateManagedWidget("wAddPlayerButton", 
					   commandWidgetClass,
					   wRegisterForm, NULL, 0);
  XtAddCallback(wAddPlayerButton, XtNcallback, 
		(XtCallbackProc)REG_AddPlayer, NULL);
  wDeletePlayerButton = XtCreateManagedWidget("wDeletePlayerButton", 
					      commandWidgetClass,
					      wRegisterForm, NULL, 0);
  XtAddCallback(wDeletePlayerButton, XtNcallback, 
		(XtCallbackProc)REG_DeletePlayer, NULL);
  
  /* The OK button */
  wRegisterOKButton = XtCreateManagedWidget("wRegisterOKButton", 
					    commandWidgetClass,
					    wRegisterForm, NULL, 0);
  XtAddCallback(wRegisterOKButton, XtNcallback, 
		(XtCallbackProc)REG_Done, NULL);

  /* The NO button */
  wRegisterNOButton = XtCreateManagedWidget("wRegisterNOButton", 
					    commandWidgetClass,
					    wRegisterForm, NULL, 0);
  XtAddCallback(wRegisterNOButton, XtNcallback, 
		(XtCallbackProc)REG_Quit, NULL);

  /* Add actions */
  XtAppAddActions(appContext, actionTable, XtNumber(actionTable));

  /* Reset the mapping */
  for (i=0; i!=MAX_PLAYERS; i++)
    piPositionToPlayer[i] = -1;
}


/************************************************************************ 
 *  FUNCTION: REG_Done
 *  HISTORY: 
 *     01.29.94  ESF  Created.
 *     05.19.94  ESF  Added warning popup.
 *     01.29.95  ESF  Moved to registerPlayer.c
 *     02.14.95  ESF  Added unselection of slots.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_Done(void)
{
  extern Int32 iState;

  Int32 i, rep;

  /* If there aren't enough players (i.e. <2), then confirm that
   * the client wants to end the registration stage.
   */

  if (RISK_GetNumPlayers()<2)
    {
#ifdef ENGLISH
      rep = UTIL_PopupDialog("Warning", "Not enough players to begin game. "
			     " Continue?", 3, "Yes", "No", "Quit");
#endif
#ifdef FRENCH
      rep = UTIL_PopupDialog("Attention", "Pas assez de joueurs pour commencer. "
			     " Continuer?", 3, "Oui", "Non", "Quitter");
#endif
      switch (rep)
        {
        case QUERY_NO:
          return;
        case QUERY_CANCEL:
          UTIL_ExitProgram(0);
        }
    }

  iState = STATE_FORTIFY;
  /* Let the server know that this client is ready to play */
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_STARTGAME, NULL);
  XtPopdown(wRegisterShell);

  /* Unselect all of the slots */
  for (i=0; i!=MAX_PLAYERS; i++)
    REG_UnselectSlot(i);
}


/************************************************************************ 
 *  FUNCTION: REG_Quit
 *  HISTORY: 
 *     23.08.94  JC   Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_Quit(void)
{
  UTIL_ExitProgram(0);
}


/************************************************************************ 
 *  FUNCTION: REG_DeletePlayer
 *  HISTORY: 
 *     02.13.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_DeletePlayer(void)
{
  Int32             i;
  MsgMessagePacket  mess;

  /* Get all of the players that are selected and delete 
   * them if the client is allowed to.
   */

  for (i=0; i!=MAX_PLAYERS; i++)
    if (REG_IsSelected(i))
      {
	/* To delete a player you have to be the client of the player,
	 * or the player has to be an AI player (anyone can delete one).
	 */

	if (RISK_GetClientOfPlayer(piPositionToPlayer[i]) == CLNT_GetThisClientID() ||
	    RISK_GetSpeciesOfPlayer(piPositionToPlayer[i]) != SPECIES_HUMAN)
	  {
	    /* Send a message to the rest of the clients */
	    char buf[256];
#ifdef ENGLISH
	    snprintf(buf, sizeof(buf), "%s (%s) has left the game.", 
		    RISK_GetNameOfPlayer(piPositionToPlayer[i]), "Human");
#endif
#ifdef FRENCH
	    snprintf(buf, sizeof(buf), "%s (%s) has left the game.", 
		    RISK_GetNameOfPlayer(piPositionToPlayer[i]), "Human");
#endif
	    mess.strMessage = buf;
	    mess.iTo        = DST_ALLPLAYERS;
	    mess.iFrom      = FROM_SERVER;
	    (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
				   MSG_MESSAGEPACKET, &mess);
	    
	    /* Actually destroy the player */
	    CLNT_FreePlayer(piPositionToPlayer[i]);
	    
	    /* Unselect the slot */
	    REG_UnselectSlot(i);
	    
	    /* Because the other players will move down to occupy
	     * the position of the deleted player, we must start
	     * looking from the slot that we just deleted.
	     */
	    
	    i--;
	  }
	else
	  {
	    char buf[256];
#ifdef ENGLISH
	    snprintf(buf, sizeof(buf), "%s belongs to another client.",
#endif
#ifdef FRENCH
	    snprintf(buf, sizeof(buf), "%s belongs to another client.",
#endif
		    RISK_GetNameOfPlayer(piPositionToPlayer[i]));
	    UTIL_PopupDialog("Error", buf, 1, "Ok", NULL, NULL);
	  }
      }
}


/**
 \b  History:
 \tag     01.29.95  ESF  Created.
 \b Notes:
 \par  maybe init color from here?
*/
void REG_AddPlayer(void)
{
  PLAYER_PopupDialog();
}


/************************************************************************ 
 *  FUNCTION: REG_PopupDialog
 *  HISTORY: 
 *     01.29.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_PopupDialog(void)
{
  Int32 x, y, i, iPlayer;

  /* Center the new shell */
  UTIL_CenterShell(wRegisterShell, wToplevel, &x, &y);
  XtVaSetValues(wRegisterShell, 
		XtNallowShellResize, False,
		XtNx, x, 
		XtNy, y, 
		XtNborderWidth, 1,
#ifdef ENGLISH
		XtNtitle, "Player Registration",
#endif
#ifdef FRENCH
		XtNtitle, "Enregistrement des joueurs",
#endif
		NULL);

  /* Reset the mapping */
  for (i=0; i!=MAX_PLAYERS; i++)
    piPositionToPlayer[i] = -1;

  /* Pop it up... */
  XtPopup(wRegisterShell, XtGrabExclusive);

  /* Add existant players */
  for (i=0; i<RISK_GetNumPlayers(); i++)
    {
      iPlayer = RISK_GetNthPlayer(i);
      REG_PlayerCreated(iPlayer);
    }
  
  /* Init the random number generator */
  srand(time(NULL));

  /* Get the background color for unselecting slots */
  XtVaGetValues(wShowPlayerForm[0], XtNborderColor, &iUnselectColor, NULL);
}


/************************************************************************ 
 *  FUNCTION: REG_MouseShiftClick
 *  HISTORY: 
 *     02.12.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_MouseShiftClick(Widget w, XEvent *pEvent, String *str, Cardinal *card)
{
  Int32 iSlot;
  UNUSED(pEvent);
  UNUSED(str);
  UNUSED(card);
  
  /* Find out which slot was clicked on */
  for (iSlot=0; iSlot!=MAX_PLAYERS; iSlot++)
    if (w == wShowPlayerColor[iSlot] ||
	w == wShowPlayerName[iSlot] ||
	w == wShowPlayerForm[iSlot])
      break;
  
  /* If the background of the listbox was clicked on, count
   * this as a click to a hypothetical MAX_PLAYER+1 slot --
   * this will result in the click erasing other selections.
   */

  if (w == wPlayerForm)
    iSlot = MAX_PLAYERS;

  /* So the selection */
  REG_PerformSelect(iSlot, FALSE);
}


/************************************************************************ 
 *  FUNCTION: REG_MouseClick
 *  HISTORY: 
 *     01.29.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_MouseClick(Widget w, XEvent *pEvent, String *str, Cardinal *card)
{
  Int32 iSlot;
  UNUSED(pEvent);
  UNUSED(str);
  UNUSED(card);
  
  /* Find out which slot was clicked on */
  for (iSlot=0; iSlot!=MAX_PLAYERS; iSlot++)
    if (w == wShowPlayerColor[iSlot] ||
	w == wShowPlayerName[iSlot] ||
	w == wShowPlayerForm[iSlot])
      break;

  /* If the background of the listbox was clicked on, count
   * this as a click to a hypothetical MAX_PLAYER+1 slot --
   * this will result in the click erasing other selections.
   */

  if (w == wPlayerForm)
    iSlot = MAX_PLAYERS;
  
  /* So the selection */
  REG_PerformSelect(iSlot, TRUE);
}


/************************************************************************ 
 *  FUNCTION: REG_EditColor
 *  HISTORY: 
 *     01.29.95  ESF  Created.
 *     02.12.95  ESF  Filled in.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_EditColor(Widget w, XEvent *pEvent, String *str, Cardinal *card)
{
  Int32    iColor, iSlot, iPlayer;
  CString  strNewColor;
  UNUSED(pEvent);
  UNUSED(str);
  UNUSED(card);

  /* Get the player. */
  for (iSlot=0; iSlot!=MAX_PLAYERS; iSlot++)
    if (w == wShowPlayerColor[iSlot])
      break;
  D_Assert(iSlot!=MAX_PLAYERS+1, "Wierd shift-click!");
  iPlayer = piPositionToPlayer[iSlot];

  /* Don't do anything if there isn't a valid player here */
  if (iPlayer == -1)
    return;

  /* Get the color. */
  iColor = COLOR_PlayerToColor(iPlayer);

  /* Actually go and edit the color */
  strNewColor = COLEDIT_EditColor(iColor, FALSE);
  
  if (strNewColor != NULL)
    {
      /* And also the actual color for the player */
      COLOR_StoreNamedColor(strNewColor, iPlayer);

      /* Refresh player info slot*/
      REG_RenderSlot(iSlot, iPlayer);
    }
}


/************************************************************************ 
 *  FUNCTION: REG_PlayerCreated
 *  HISTORY: 
 *     02.12.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_PlayerCreated(Int32 iPlayer)
{
  Int32   iSlot;

  /* Search for open slot */
  for (iSlot=0; iSlot!=MAX_PLAYERS; iSlot++)
    if (piPositionToPlayer[iSlot] == -1)
      break;

  /* There MUST be a slot */
  D_Assert(iSlot!=MAX_PLAYERS+1, "Whoa, dude!  Bogus player somewhere?!");

  /* Actually dump the data to the slot */
  REG_RenderSlot(iSlot, iPlayer);
}


/************************************************************************ 
 *  FUNCTION: REG_PlayerDestroyed
 *  HISTORY: 
 *     02.12.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_PlayerDestroyed(Int32 iPlayer)
{
  Int32  iSlot, i;

  /* Which slot is the player in? */
  iSlot = REG_PlayerToSlot(iPlayer);

  /* Move all the other entries up */
  for (i=iSlot; 
       i<MAX_PLAYERS-1 && 
       piPositionToPlayer[i]!=-1 &&
       piPositionToPlayer[i+1]!=-1; 
       i++)
    {
      /* Move slot i+1 to i */
      iPlayer = piPositionToPlayer[i+1];
      piPositionToPlayer[i] = iPlayer;

      /* Fill the destination slot with the new player */
      REG_RenderSlot(i, iPlayer);

      /* Copy the state of selection for the destination slot. */
      if (REG_IsSelected(i+1))
	REG_SelectSlot(i);
      else
	REG_UnselectSlot(i);
    }

  /* Erase the last one */
  REG_RenderSlot(i, -1);
}


/************************************************************************ 
 *  FUNCTION: REG_PerformSelect
 *  HISTORY: 
 *     02.12.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_PerformSelect(Int32 iSlot, Flag fEraseOthers)
{
  /* Don't do anything if there isn't a valid player here */
  if (iSlot>=0 && iSlot<MAX_PLAYERS && piPositionToPlayer[iSlot] != -1)
    {  
      /* If already selected, leave selected if not shift-click */
      if (REG_IsSelected(iSlot) && fEraseOthers == FALSE)
	REG_UnselectSlot(iSlot);
      else /* Otherwise, select it */
	REG_SelectSlot(iSlot);
    }

  /* Depending on whether it was a shift-click or a click, erase others */
  if (fEraseOthers)
    {
      Int32 i;
      
      for (i=0; i!=MAX_PLAYERS; i++)
	if (i!=iSlot)
	  REG_UnselectSlot(i);
    }
}


/************************************************************************ 
 *  FUNCTION: REG_RenderSlot
 *  HISTORY: 
 *     02.12.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_RenderSlot(Int32 iSlot, Int32 iPlayer)
{
  /* Either render the slot with a player or delete it */
  if (iPlayer == -1)
    {
      /* Reset the slot */
      piPositionToPlayer[iSlot] = -1;

      /* Unselect the slot */
      REG_UnselectSlot(iSlot);
      
      /* Fill in the color and change the cursor */
      XtVaSetValues(wShowPlayerColor[iSlot], 
		    XtNbackground, iUnselectColor, 
		    XtNborderColor, iUnselectColor,
		    XtNcursor, normalCursor, /* BUG -- not right */
		    NULL);
      
      /* The name */
      XtVaSetValues(wShowPlayerName[iSlot], XtNlabel, "", NULL);
      
      /* The species */
      XtVaSetValues(wShowPlayerSpecies[iSlot], XtNlabel, "", NULL);
    }
  else
    {
      /* Set the mapping */
      piPositionToPlayer[iSlot] = iPlayer;
      
      /* Fill in the color and change the cursor */
      XtVaSetValues(wShowPlayerColor[iSlot], 
		    XtNbackground, 
		    COLOR_QueryColor(COLOR_PlayerToColor(iPlayer)), 
		    XtNborderColor, iSelectColor,
		    XtNcursor, handCursor,
		    NULL);
      
      /* The name */
      XtVaSetValues(wShowPlayerName[iSlot], XtNlabel, 
		    RISK_GetNameOfPlayer(iPlayer), NULL);
      
      /* The species */
      XtVaSetValues(wShowPlayerSpecies[iSlot], XtNlabel, 
		    RISK_GetNameOfSpecies(RISK_GetSpeciesOfPlayer(iPlayer)), 
		    NULL);
    }
}


/************************************************************************ 
 *  FUNCTION: REG_IsSelected
 *  HISTORY: 
 *     02.13.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag REG_IsSelected(Int32 iSlot)
{
  Int32 iBorderColor;

  /* If out of bounds then call it unselected */
  if (iSlot<0 || iSlot>=MAX_PLAYERS)
    return FALSE;

  XtVaGetValues(wShowPlayerForm[iSlot], XtNborderColor, &iBorderColor, NULL);
  return (iBorderColor == iSelectColor);
}


/************************************************************************ 
 *  FUNCTION: REG_PlayerToSlot
 *  HISTORY: 
 *     02.13.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 REG_PlayerToSlot(Int32 iPlayer)
{
  Int32 iSlot;

  /* Which slot is the player in? */
  for (iSlot=0; iSlot!=MAX_PLAYERS; iSlot++)
    if (piPositionToPlayer[iSlot] == iPlayer)
      break;
  D_Assert(iSlot<MAX_PLAYERS, "Bogus player to slot!");

  return iSlot;
}


/************************************************************************ 
 *  FUNCTION: REG_Callback
 *  HISTORY: 
 *     02.26.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void REG_Callback(Int32 iMessType, void *pvMess)
{
  /*
   * PlayerCreated          --> Add player
   * PlayerDestroyed        --> Delete player
   */

  if (iMessType == MSG_OBJINTUPDATE && 
      ((MsgObjIntUpdate *)pvMess)->iField == PLR_ALLOCATION &&
      ((MsgObjIntUpdate *)pvMess)->iNewValue == ALLOC_COMPLETE)
    {
      /* PlayerCreated */
      REG_PlayerCreated(((MsgObjIntUpdate *)pvMess)->iIndex1);
    }
  else if (iMessType == MSG_OBJINTUPDATE && 
	   ((MsgObjIntUpdate *)pvMess)->iField == PLR_ALLOCATION &&
	   ((MsgObjIntUpdate *)pvMess)->iNewValue == ALLOC_NONE)
    {
      /* PlayerDestroyed */
      REG_PlayerDestroyed(((MsgObjIntUpdate *)pvMess)->iIndex1);
    }
}
