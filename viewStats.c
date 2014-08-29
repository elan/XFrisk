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
 *   $Id: viewStats.c,v 1.12 2000/01/16 18:47:02 tony Exp $
 */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/List.h>

#include "viewStats.h"
#include "riskgame.h"
#include "callbacks.h"
#include "gui-vars.h"
#include "utils.h"
#include "types.h"
#include "debug.h"

/* Defines */
#define GRAPH_WIDTH   471 
#define GRAPH_HEIGHT  150

/* dX for graph 3 ok-ish*/
#define GRAPH_STEP 3

/* Private globals */
static Widget wStatShell, wStatForm;
static Widget wStatNumberForm, wStatGraphForm;
static Widget wStatColorLabel, wStatNameLabel, wStatArmiesLabel;
static Widget wStatCountriesLabel, wStatCardsLabel, wStatWLDLabel;
static Widget wStatViewport, wStatPlayerForm;
static Widget wPlayerStatForm[MAX_PLAYERS], wStatPlayerColor[MAX_PLAYERS];
static Widget wStatPlayerName[MAX_PLAYERS], wStatPlayerArmies[MAX_PLAYERS];
static Widget wStatPlayerCountries[MAX_PLAYERS], wStatPlayerCards[MAX_PLAYERS];
static Widget wStatPlayerWLD[MAX_PLAYERS], wStatGraphLabel, wStatGraph;
static Widget wStatCloseButton;

static Int32   piSlotToPlayer[MAX_PLAYERS];
static Int32   iBackgroundColor;
static Int32   iXPos = 0, fFirstTime = TRUE;
static Pixmap  pixmap;
static Flag    fDialogIsUp = FALSE;
static Int32   piPlayerWin[MAX_PLAYERS], piPlayerLose[MAX_PLAYERS];
static Int32   piPlayerDraw[MAX_PLAYERS];
static Int32   piTotalArmies[MAX_PLAYERS];
static Int32   piLastValue[MAX_PLAYERS];

/* Private functions */
Int32 STAT_SlotToPlayer(Int32 iSlot);

void  STAT_InitThings(void);

void  STAT_UpdateGraph(void);
void  STAT_UpdateArmies(Int32 iPlayer, Int32 iArmies);
void  STAT_UpdateCountries(Int32 iPlayer, Int32 iCountries);
void  STAT_UpdateCards(Int32 iPlayer, Int32 iCards);
void  STAT_UpdateWinLoseDraw(Int32 iPlayer);

void  STAT_PlayerCreated(Int32 iPlayer);
void  STAT_PlayerDestroyed(Int32 iPlayer);


/************************************************************************ 
 *  FUNCTION: STAT_AddPlayerStats
 *  HISTORY: 
 *     12.25.95  TdH  Created.
 *  PURPOSE: Add info lines about players
 *  NOTES: 
 ************************************************************************/

void STAT_AddPlayerStats(void) {
    Int32 i;

  /* Each player */
  for (i=0; i!=MAX_PLAYERS; i++)
    {
      wPlayerStatForm[i] = XtVaCreateManagedWidget("wShowPlayerForm",
                                                   formWidgetClass,
                                                   wStatPlayerForm,
                                                   NULL);
      if (i)
	XtVaSetValues(wPlayerStatForm[i], XtNfromVert, 
		      wPlayerStatForm[i-1], NULL);
      wStatPlayerColor[i] = XtVaCreateManagedWidget("wStatPlayerColor",
						    labelWidgetClass, 
						    wPlayerStatForm[i],
						    XtNresize, False,
						    NULL);
      wStatPlayerName[i] = XtVaCreateManagedWidget("wStatPlayerName",
						   labelWidgetClass,
						   wPlayerStatForm[i], 
						   XtNfromHoriz, 
						   wStatPlayerColor[i],
						   XtNresize, False,
						   NULL);
      wStatPlayerArmies[i] = XtVaCreateManagedWidget("wStatPlayerArmies",
						     labelWidgetClass, 
						     wPlayerStatForm[i], 
						     XtNfromHoriz, 
						     wStatPlayerName[i],
						     XtNresize, False,
						     NULL);
      wStatPlayerCountries[i] = XtVaCreateManagedWidget("wStatPlayerCountries",
							labelWidgetClass, 
							wPlayerStatForm[i], 
							XtNfromHoriz, 
							wStatPlayerArmies[i],
							XtNresize, False,
							NULL);
      wStatPlayerCards[i] = XtVaCreateManagedWidget("wStatPlayerCards",
						    labelWidgetClass, 
						    wPlayerStatForm[i], 
						    XtNfromHoriz, 
						    wStatPlayerCountries[i],
						    XtNresize, False,
						    NULL);
      wStatPlayerWLD[i] = XtVaCreateManagedWidget("wStatPlayerWLD",
						  labelWidgetClass, 
						  wPlayerStatForm[i], 
						  XtNfromHoriz, 
						  wStatPlayerCards[i],
						  XtNresize, False,
                                                  NULL);
    }
}

/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.17.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_BuildDialog(void) 
{
  Int32 i;

  wStatShell = XtCreatePopupShell("wStatShell", 
				  topLevelShellWidgetClass,
				  wToplevel, pVisualArgs, iVisualCount);

  /* The forms */
  wStatForm = XtCreateManagedWidget("wStatForm", 
				    formWidgetClass, wStatShell, 
				    NULL, 0);
  wStatNumberForm = XtCreateManagedWidget("wStatNumberForm", 
					  formWidgetClass, wStatForm,
					  NULL, 0);
  wStatGraphForm = XtCreateManagedWidget("wStatGraphForm", 
					 formWidgetClass, wStatForm, 
					 NULL, 0);
  wStatCloseButton = XtCreateManagedWidget("wStatCloseButton", 
					   commandWidgetClass, wStatForm, 
					   NULL, 0);
  XtAddCallback(wStatCloseButton, XtNcallback, 
		(XtCallbackProc)STAT_Close, NULL);
  
  /* The labels */
  wStatColorLabel = XtCreateManagedWidget("wStatColorLabel", labelWidgetClass,
					  wStatNumberForm, NULL, 0);
  wStatNameLabel = XtCreateManagedWidget("wStatNameLabel", labelWidgetClass,
					  wStatNumberForm, NULL, 0);
  wStatArmiesLabel = XtCreateManagedWidget("wStatArmiesLabel", 
					   labelWidgetClass,
					   wStatNumberForm, NULL, 0);
  wStatCountriesLabel = XtCreateManagedWidget("wStatCountriesLabel", 
					      labelWidgetClass,
					      wStatNumberForm, NULL, 0);
  wStatCardsLabel = XtCreateManagedWidget("wStatCardsLabel", labelWidgetClass,
                                          wStatNumberForm, NULL, 0);
  wStatWLDLabel = XtCreateManagedWidget("wStatWLDLabel", labelWidgetClass,
					wStatNumberForm, NULL, 0);

  /* The statistics "spreadsheet" */
  wStatViewport = XtCreateManagedWidget("wStatViewport", viewportWidgetClass,
					wStatNumberForm, NULL, 0);
  wStatPlayerForm = XtCreateManagedWidget("wStatPlayerForm", boxWidgetClass,
					  wStatViewport, NULL, 0);

  STAT_AddPlayerStats();/* info about each player */

  /* The statistics graph */
  wStatGraphLabel = XtVaCreateManagedWidget("wStatGraphLabel",
					    labelWidgetClass, 
					    wStatGraphForm, NULL);
  wStatGraph = XtVaCreateManagedWidget("wStatGraph", labelWidgetClass, 
				       wStatGraphForm, NULL);

  /* Create the graph pixmap */
  pixmap = XCreatePixmap(hDisplay, 
			 RootWindowOfScreen(XtScreen(wStatGraph)),
			 GRAPH_WIDTH, GRAPH_HEIGHT,
			 DefaultDepth(hDisplay, DefaultScreen(hDisplay)));

  /* Set it to be the background pixmap of the widget
   */
  XtVaSetValues(wStatGraph, XtNbitmap, pixmap, NULL);

  STAT_InitThings();

  /* In addition, reset the mapping -- this happens but once
   * this is why it's not in STAT_InitThings()*/
  for (i=0; i!=MAX_PLAYERS; i++)
    piSlotToPlayer[i] = -1;
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     04.01.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_InitThings(void)
{
  Int32 i;

  /* Init the data & slots */
  for (i=0; i!=MAX_PLAYERS; i++)
    {
      piPlayerWin[i] = piPlayerLose[i] = piPlayerDraw[i] = 0;
      piTotalArmies[i] = 0;
      if (fDialogIsUp)
	{
	  STAT_UpdateWinLoseDraw(i);
	  STAT_UpdateArmies(i, piTotalArmies[i]);
	}
    }

  /* Fill the pixmap with the color background of the ocean.
   * Tdh: and ocean color is borked. easiest fix would be a global SEACOLOR
   */
/*  XSetForeground(hDisplay, hGC, COLOR_QueryColor(COLOR_CountryToColor(NUM_COUNTRIES)));*/
  XSetForeground(hDisplay, hGC, 0);
  XFillRectangle(hDisplay, pixmap, hGC, 0, 0, GRAPH_WIDTH, GRAPH_HEIGHT);

  /* If the dialog is up, then blit the new stuff to it */
  if (fDialogIsUp)
    XCopyArea(hDisplay, pixmap, XtWindow(wStatGraph), hGC, 0, 0,
	      GRAPH_WIDTH, GRAPH_HEIGHT, 0, 0);
  iXPos = 0;

  /* This will trigger the first points on the graph to be drawn */
  fFirstTime = TRUE;
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.17.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_PopupDialog(void) 
{
  Int32 x, y, i, iPlayer;
  
  if (fDialogIsUp)
    {
      fDialogIsUp = FALSE;
      XtPopdown(wStatShell);
      return;
    }

  /* Center the new shell */
  UTIL_CenterShell(wStatShell, wToplevel, &x, &y);
  XtVaSetValues(wStatShell, 
		XtNallowShellResize, False,
		XtNx, x, 
		XtNy, y, 
		XtNborderWidth, 1,
#ifdef ENGLISH
		XtNtitle, "Game Statistics",
#endif
#ifdef FRENCH
		XtNtitle, "Statistiques de la partie",
#endif
		NULL);

  /* It's going to be up in a second. */
  fDialogIsUp = TRUE;

  for (i=0; i<RISK_GetNumPlayers(); i++)
    {
      iPlayer = RISK_GetNthPlayer(i);
      
      STAT_UpdateArmies(iPlayer, piTotalArmies[iPlayer]);
      STAT_UpdateCountries(iPlayer, RISK_GetNumCountriesOfPlayer(iPlayer));
      STAT_UpdateCards(iPlayer, RISK_GetNumCardsOfPlayer(iPlayer));
      STAT_UpdateWinLoseDraw(iPlayer);
    }

  /* Pop it up... */
  XtPopup(wStatShell, XtGrabNone);

  /* Get the background color for erasing slots */
  XtVaGetValues(wPlayerStatForm[0], XtNborderColor, &iBackgroundColor, NULL);
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.17.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_Close(void) 
{
  STAT_PopupDialog();
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.17.95  ESF  Created.
 *     28.08.95  JC   MSG_ENDOFGAME -> MSG_ENDOFMISSION or MSG_VICTORY.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_Callback(Int32 iMessType, void *pvMess) 
{
  /*
   * GameBegin              --> Init. things
   * EndTurn                --> Territories graph 
   * DiceRoll               --> Win/Lose/Draw
   * Object(iNumCountries)  --> Update
   * Object(iNumCards)      --> Update
   * Object(iNumArmies)     --> Update
   * PlayerCreated          --> Add player
   * PlayerDestroyed        --> Delete player
   */

  if (iMessType == MSG_STARTGAME)
    {
      /* Reset things that need resetting between games */
      STAT_InitThings();
    }
  else if (iMessType == MSG_DICEROLL)
    {
      /* Go through the dice roll and update the 
       * win/lose/draw statistics for the players.
       */

      MsgDiceRoll  *pMess = (MsgDiceRoll *)pvMess;
      Int32         i, iOtherPlayer = pMess->iDefendingPlayer;
      
      for (i=0; i!=3; i++)
	if (pMess->pAttackDice[i] != -1 &&  pMess->pDefendDice[i] != -1)
	  {
	    if (pMess->pAttackDice[i] > pMess->pDefendDice[i])
	      { piPlayerWin[iCurrentPlayer]++; piPlayerLose[iOtherPlayer]++; }
	    else if (pMess->pAttackDice[i] == pMess->pDefendDice[i])
	      { piPlayerDraw[iCurrentPlayer]++; piPlayerDraw[iOtherPlayer]++; }
	    else if (pMess->pAttackDice[i] < pMess->pDefendDice[i])
	      { piPlayerLose[iCurrentPlayer]++; piPlayerWin[iOtherPlayer]++; }
	  }

      /* Win/Lose/Draw */
      if (fDialogIsUp)
	{
	  STAT_UpdateWinLoseDraw(iCurrentPlayer);
	  STAT_UpdateWinLoseDraw(iOtherPlayer);
	}
    }

  else if ((iMessType == MSG_ENDOFMISSION) || (iMessType == MSG_VICTORY))
    {
      /* The last position of the board, since ENDTURN won't come along. */
      STAT_UpdateGraph();
    }

  else if (iMessType == MSG_TURNNOTIFY)
    {
      /* Draws the initial points on the graph */
      if (fFirstTime)
	{
	  fFirstTime = FALSE;
	  STAT_UpdateGraph();
	}
    }

  else if (iMessType == MSG_ENDTURN)
    {
      /* Territories graph, but only if not in fortifying mode... */
      if (iState != STATE_FORTIFY)
	STAT_UpdateGraph();
    }

  else if (iMessType == MSG_OBJINTUPDATE && 
	   ((MsgObjIntUpdate *)pvMess)->iField == PLR_ALLOCATION &&
	   ((MsgObjIntUpdate *)pvMess)->iNewValue == ALLOC_COMPLETE)
    {
      /* PlayerCreated */
      STAT_PlayerCreated(((MsgObjIntUpdate *)pvMess)->iIndex1);
    }
  
  else if (iMessType == MSG_OBJINTUPDATE && 
	   ((MsgObjIntUpdate *)pvMess)->iField == PLR_ALLOCATION &&
	   ((MsgObjIntUpdate *)pvMess)->iNewValue == ALLOC_NONE)
    {
      /* PlayerDestroyed */
      STAT_PlayerDestroyed(((MsgObjIntUpdate *)pvMess)->iIndex1);
    }

  else if (iMessType == MSG_OBJINTUPDATE && 
	   ((MsgObjIntUpdate *)pvMess)->iField == PLR_NUMCOUNTRIES)
    {
      Int32 iPlayer = ((MsgObjIntUpdate *)pvMess)->iIndex1;
      
      /* Object(iNumCountries) */
      if (iPlayer>=0)
	STAT_UpdateCountries(iPlayer, ((MsgObjIntUpdate *)pvMess)->iNewValue);
    }

  else if (iMessType == MSG_OBJINTUPDATE && 
	   ((MsgObjIntUpdate *)pvMess)->iField == PLR_NUMCARDS)
    {
      Int32 iPlayer = ((MsgObjIntUpdate *)pvMess)->iIndex1;

      /* Object(iNumCountries) */
      if (iPlayer>=0)
	STAT_UpdateCards(iPlayer, ((MsgObjIntUpdate *)pvMess)->iNewValue);
    }

  else if (iMessType == MSG_OBJINTUPDATE &&
	   ((MsgObjIntUpdate *)pvMess)->iField == CNT_NUMARMIES)
    {
      Int32 iCountry  = ((MsgObjIntUpdate *)pvMess)->iIndex1;
      Int32 iPlayer   = RISK_GetOwnerOfCountry(iCountry);
      Int32 iNewValue = ((MsgObjIntUpdate *)pvMess)->iNewValue; 
      Int32 iOldValue = RISK_GetNumArmiesOfCountry(iCountry);
      
      if (iPlayer >=0)
	{
	  /* Update the number of total armies the player has */
	  piTotalArmies[iPlayer] += (iNewValue - iOldValue);
	  STAT_UpdateArmies(iPlayer, piTotalArmies[iPlayer]);
	}
    }
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_UpdateGraph(void) {
  Int32 iYPos, i;

  /* Assertion: iXPos is the new X position. */

  /* Check to see if we are end the right end of the graph */
  if (iXPos == GRAPH_WIDTH) {
      /* Pick the two-thirds point */
      Int32 iNewStartX = GRAPH_WIDTH/3 * 2;

      /* Scroll the graph over a third to the left */
      XCopyArea(hDisplay, pixmap, pixmap, hGC, 
		iNewStartX/2, 0, 
		GRAPH_WIDTH-iNewStartX/2, GRAPH_HEIGHT,
		0, 0);
      /* Clean up the right third now */
      XSetForeground(hDisplay, hGC, COLOR_QueryColor(COLOR_CountryToColor(NUM_COUNTRIES)));
      XFillRectangle(hDisplay, pixmap, hGC, iNewStartX, 0, 
		     GRAPH_WIDTH-iNewStartX, GRAPH_HEIGHT);
      iXPos = iNewStartX;
    }

  /* Assertion: iXPos is the next X place to draw the next point */

  /* Put the number of countries of each player on the graph */
  for (i=0; i!=MAX_PLAYERS; i++)  {
      /* Pick the color of player i */
      XSetForeground(hDisplay, hGC, COLOR_QueryColor(COLOR_PlayerToColor(i)));

      /* Calculate the y point */
      iYPos = GRAPH_HEIGHT - 
	RISK_GetNumCountriesOfPlayer(i) * GRAPH_HEIGHT / NUM_COUNTRIES;
      
      if (iXPos == 0)  {
	  /* First point, just draw a point */
	  XDrawPoint(hDisplay, pixmap, hGC, iXPos, iYPos);
	}
      else   {
	  /* Draw a line from the last point to the current one */
	  XDrawLine(hDisplay, pixmap, hGC, 
		    iXPos-GRAPH_STEP, piLastValue[i],
                    iXPos, iYPos);
	  XDrawLine(hDisplay, pixmap, hGC, 
		    iXPos-GRAPH_STEP, piLastValue[i]+1,
                    iXPos, iYPos+1);
	}
      
      /* Copy portion to screen so it shows <==> the window's up */
      if (fDialogIsUp)
	XCopyArea(hDisplay, pixmap, XtWindow(wStatGraph), hGC, 
		  iXPos-GRAPH_STEP, 0,
		  2, GRAPH_HEIGHT,
		  iXPos-GRAPH_STEP, 0);
      
      /* Update the last y-value for the player i */
      piLastValue[i] = iYPos;
    }

  /* Go to the next X position */
  iXPos+=GRAPH_STEP;
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_UpdateArmies(Int32 iPlayer, Int32 iArmies)
{
  Int32 iSlot = STAT_PlayerToSlot(iPlayer);
  char buf[256];
  if (iSlot != -1)
    {
      snprintf(buf, sizeof(buf), "%d", iArmies);
      XtVaSetValues(wStatPlayerArmies[iSlot], XtNlabel, buf, NULL);
    }
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_UpdateCountries(Int32 iPlayer, Int32 iCountries)
{
  Int32 iSlot = STAT_PlayerToSlot(iPlayer);
  char buf[256];
  if (iSlot != -1)
    {
      snprintf(buf, sizeof(buf), "%d", iCountries);
      XtVaSetValues(wStatPlayerCountries[iSlot], XtNlabel, buf, NULL);
    }
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_UpdateCards(Int32 iPlayer, Int32 iCards)
{
  Int32 iSlot = STAT_PlayerToSlot(iPlayer);
  char buf[256];
  if (iSlot != -1)
    {
      snprintf(buf, sizeof(buf), "%d", iCards);
      XtVaSetValues(wStatPlayerCards[iSlot], XtNlabel, buf, NULL);
    }
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_UpdateWinLoseDraw(Int32 iPlayer)
{
  Int32 iSlot = STAT_PlayerToSlot(iPlayer);
  char buf[256];
  if (iSlot != -1)
    {
      snprintf(buf, sizeof(buf), "%d/%d/%d", 
	      piPlayerWin[iPlayer], 
	      piPlayerLose[iPlayer], 
	      piPlayerDraw[iPlayer]);
      
      XtVaSetValues(wStatPlayerWLD[iSlot], XtNlabel, buf, NULL);
    }
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_PlayerCreated(Int32 iPlayer)
{
  Int32   iSlot;

  /* Search for open slot */
  for (iSlot=0; iSlot < MAX_PLAYERS; iSlot++)
    if (piSlotToPlayer[iSlot] == -1)
      break;
  
  /* There MUST be a slot */
  D_Assert(iSlot >= MAX_PLAYERS, "Whoa, dude!  Bogus player somewhere?!");
  
  /* Actually dump the data to the slot */
  STAT_RenderSlot(iSlot, iPlayer);
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_PlayerDestroyed(Int32 iPlayer)
{
  Int32  iSlot, i;

  /* Which slot is the player in? */
  iSlot = STAT_PlayerToSlot(iPlayer);
  D_Assert(iSlot != -1, "Player should be in table!");

  /* Move all the other entries up */
  for (i=iSlot; 
       (i<MAX_PLAYERS-1) &&
       (piSlotToPlayer[i]!=-1) &&
       (piSlotToPlayer[i+1]!=-1);
       i++)
    {
      /* Move slot i+1 to i */
      iPlayer = piSlotToPlayer[i+1];
      piSlotToPlayer[i] = iPlayer;
      
      /* Fill the destination slot with the new player */
      STAT_RenderSlot(i, iPlayer);
    }

  /* Erase the last one */
  STAT_RenderSlot(i, -1);
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void STAT_RenderSlot(Int32 iSlot, Int32 iPlayer)
{
  D_Assert(iSlot != -1, "Bogus slot!");

  /* Either render the slot with a player or delete it */
  if (iPlayer == -1)
    {
      /* Reset the slot */
      piSlotToPlayer[iSlot] = -1;
      
      /* Fill in the color */
      XtVaSetValues(wStatPlayerColor[iSlot], 
		    XtNbackground, iBackgroundColor, 
		    XtNborderColor, iBackgroundColor,
		    NULL);
      
      /* The name */
      XtVaSetValues(wStatPlayerName[iSlot], XtNlabel, "", NULL);
      
      /* The armies */
      XtVaSetValues(wStatPlayerArmies[iSlot], XtNlabel, "", NULL);

      /* The countries */
      XtVaSetValues(wStatPlayerCountries[iSlot], XtNlabel, "", NULL);

      /* The cards */
      XtVaSetValues(wStatPlayerCards[iSlot], XtNlabel, "", NULL);

      /* The Win/Lose/Draw */
      XtVaSetValues(wStatPlayerWLD[iSlot], XtNlabel, "", NULL);
    }
  else
    {
      /* Set the mapping */
      piSlotToPlayer[iSlot] = iPlayer;
      
      /* Fill in the color and change the cursor */
      XtVaSetValues(wStatPlayerColor[iSlot], 
		    XtNbackground, COLOR_QueryColor(COLOR_PlayerToColor(iPlayer)), 
		    XtNborderColor, BlackPixel(hDisplay, 0),
		    NULL);
      
      /* The name */
      XtVaSetValues(wStatPlayerName[iSlot], XtNlabel, 
		    RISK_GetNameOfPlayer(iPlayer), NULL);
      
      /* The countries */
      STAT_UpdateCountries(iPlayer, RISK_GetNumCountriesOfPlayer(iPlayer));

      /* The armies */
      STAT_UpdateArmies(iPlayer, piTotalArmies[iPlayer]);

      /* The cards */
      STAT_UpdateCards(iPlayer, RISK_GetNumCardsOfPlayer(iPlayer));

      /* The Win/Lose/Draw */
      STAT_UpdateWinLoseDraw(iPlayer);
    }
}  


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 STAT_PlayerToSlot(Int32 iPlayer)
{
  Int32 iSlot;
  
  /* Which slot is the player in? */
  for (iSlot=0; iSlot!=MAX_PLAYERS; iSlot++)
    if (piSlotToPlayer[iSlot] == iPlayer)
      return iSlot;
  
  return -1;
}
