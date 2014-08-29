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
 *   $Id: callbacks.c,v 1.17 2000/01/17 21:53:06 tony Exp $
 *
 *   Used by client/AI
 *   Notes:
 *     should not use X, so ai can be compiled on X-less box
 *      is included by riskgame.c, maybe fix there?
 *      or this be the X part, and move out other messages?
 */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/List.h>
#include <X11/Xaw/Toggle.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "language.h"
#include "network.h"
#include "gui-vars.h"
#include "gui-func.h"
#include "utils.h"
#include "callbacks.h"
#include "version.h"
#include "riskgame.h"
#include "client.h"
#include "dice.h"
#include "cards.h"
#include "game.h"
#include "colormap.h"
#include "help.h"
#include "colorEdit.h"
#include "registerPlayers.h"
#include "debug.h"
#include "viewStats.h"


/* Game globals -- move elsewhere? -- make part of riskgame object state! */
static Flag	fPlayingRemotely=FALSE;
static Flag     fForceExchange=FALSE;

static Int32    iCountryToFortify=-1;
static Int32    piMsgDstPlayerID[MAX_PLAYERS+1];
static Flag     fWaitMission = FALSE;

static Flag	fHaveSeenFirstPlayer;

Int32           iIndexMD;

Int32           iActionState=ACTION_PLACE;
Int32           iReply;
CString         pstrMsgDstCString[MAX_PLAYERS+1];
Int32	        iFirstPlayer;
Int32           iState, iCurrentPlayer;
Flag            fGameStarted=FALSE;

/************************************************************************ 
 *  FUNCTION: CBK_XIncomingMessage
 *  HISTORY: 
 *     03.17.94  ESF  Created.
 *     06.24.94  ESF  Fixed memory leak bug.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_XIncomingMessage(XtPointer pClientData, int *iSource, XtInputId *id)
{
  Int32     iMessType;
  void     *pvMess;

  UNUSED(pClientData);
  UNUSED(id);
  
  (void)RISK_ReceiveMessage(*iSource, &iMessType, &pvMess);
  CBK_IncomingMessage(iMessType, pvMess);
  NET_DeleteMessage(iMessType, pvMess);
}


/************************************************************************ 
 *  FUNCTION: CBK_IncomingMessage
 *  HISTORY: 
 *     01.26.94  ESF  Created.
 *     01.27.94  ESF  Coded MSG_MESSAGEPACKET case.
 *     01.29.94  ESF  Changed MSG_MESSAGEPACKET to scroll correctly.
 *     02.05.94  ESF  Added MSG_REGISTERPLAYER handling.
 *     03.02.94  ESF  Added more army placing glue.
 *     03.04.94  ESF  Factored out code, cleaned up.
 *     03.05.94  ESF  Added color-coded player turn indicator call.
 *     03.06.94  ESF  Fixed initialization of iCurrentPlayer bug.
 *     03.16.94  ESF  Added continent bonuses.
 *     03.17.94  ESF  Factored out X code so that this could be more general.
 *     03.17.94  ESF  Fixed bug, player indicator broken for remote case.
 *     03.17.94  ESF  Added fPlayingRemotely setting.
 *     03.18.94  ESF  Completed MSG_UPDATEARMIES code.
 *     03.28.94  ESF  Added code for MSG_ENDOFGAME and MSG_DEADPLAYER.
 *     03.29.94  ESF  Added code for MSG_CARDPACKET.
 *     03.29.94  ESF  Fixed bug in handling on MSG_UPDATEARMIES.
 *     03.29.94  ESF  Added handling for MSG_REPLYPACKET.
 *     04.11.94  ESF  Added handling for MSG_CARDPACKET.
 *     05.12.94  ESF  Added handling for MSG_DELETEMSGDST.
 *     05.17.94  ESF  Added handling for MSG_NETMESSAGE.
 *     05.19.94  ESF  Added verbose message when exit occurs.
 *     08.28.94  ESF  Added handling for MSG_POPUPREGISTERBOX.
 *     09.01.94  ESF  Added handling for MSG_NETPOPUP.
 *     10.29.94  ESF  Added handling for MSG_DICEROLL.
 *     10.29.94  ESF  Added handling for MSG_MOVENOTIFY.
 *     10.29.94  ESF  Added handling for MSG_ATTACKNOTIFY.
 *     10.29.94  ESF  Added handling for MSG_PLACEROLL.
 *     01.15.95  ESF  Removed handling for MSG_DELETEMSGDST.
 *     24.08.95  JC   Added handling for MSG_MISSION.
 *     25.08.95  JC   new game if MSG_ENDOFGAME and winner is -1.
 *     25.08.95  JC   Start the game only after a fortification.
 *     28.08.95  JC   Added handling for MSG_ENDOFMISSION, MSG_VICTORY.
 *                    Now MSG_ENDOFGAME => new game
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_IncomingMessage(Int32 iMessType, void *pvMess)
{
  switch(iMessType)
    {
    case MSG_NOMESSAGE:
      /* NoOp */
      break;

    case MSG_NETMESSAGE:
      UTIL_DisplayComment(((MsgNetMessage *)(pvMess))->strMessage);
      break;

    case MSG_PLACENOTIFY:
      {
	/* Copy the message, since we are using it for a while */
	MsgPlaceNotify *msgMess = 
	  (MsgPlaceNotify *)MEM_Alloc(sizeof(MsgPlaceNotify));
	memcpy((void *)msgMess, (void *)pvMess, sizeof(MsgPlaceNotify));

	/* Do the notification, and then add a timeout for erasing it */
	UTIL_PlaceNotification((XtPointer)msgMess, NULL);
	XtAppAddTimeOut(appContext, NOTIFY_TIME, 
			UTIL_PlaceNotification, msgMess);
      }
	break;

    case MSG_MOVENOTIFY:
      {
	/* Copy the message, since we are using it for a while */
	MsgMoveNotify *msgMess = 
	  (MsgMoveNotify *)MEM_Alloc(sizeof(MsgMoveNotify));
	memcpy((void *)msgMess, (void *)pvMess, sizeof(MsgMoveNotify));

	/* Do the notification, and then add a timeout for erasing it */
	UTIL_MoveNotification((XtPointer)msgMess, NULL);
	XtAppAddTimeOut(appContext, NOTIFY_TIME, 
			UTIL_MoveNotification, msgMess);
      }
      break;

    case MSG_ATTACKNOTIFY:
      {
	/* Copy the message, since we are using it for a while */
	MsgAttackNotify *msgMess = 
	  (MsgAttackNotify *)MEM_Alloc(sizeof(MsgAttackNotify));
	memcpy((void *)msgMess, (void *)pvMess, sizeof(MsgAttackNotify));

	/* Do the notification, and then add a timeout for erasing it */
	UTIL_AttackNotification((XtPointer)msgMess, NULL);
	XtAppAddTimeOut(appContext, NOTIFY_TIME, 
			UTIL_AttackNotification, msgMess);
      }
      break;

    case MSG_DICEROLL:
      {
	MsgDiceRoll *pmsgDiceRoll = (MsgDiceRoll *)pvMess;

	/* Set the dice colors to be the correct ones */
	COLOR_CopyColor(COLOR_PlayerToColor(iCurrentPlayer), 
			COLOR_DieToColor(0));
	COLOR_CopyColor(COLOR_PlayerToColor(pmsgDiceRoll->iDefendingPlayer),
			COLOR_DieToColor(1));
	
	/* Erase what was there, and then display the dice */
	DICE_Hide();
	DICE_DrawDice(pmsgDiceRoll->pAttackDice, pmsgDiceRoll->pDefendDice);
      }
      break;

    case MSG_NETPOPUP:
      {
#ifdef ENGLISH
        (void)UTIL_PopupDialog("Message from Server", 
			       ((MsgNetPopup *)pvMess)->strMessage, 1, 
			       "Ok", NULL, NULL);
#endif
#ifdef FRENCH
        (void)UTIL_PopupDialog("Message du Serveur", 
			       ((MsgNetPopup *)pvMess)->strMessage, 1, 
			       "Oui", NULL, NULL);
#endif
      }
      break;

    case MSG_POPUPREGISTERBOX:
      {
	/* Clear any text... */
	UTIL_DisplayError("");
	UTIL_DisplayComment("");

	REG_PopupDialog();
      }
      break;

    case MSG_FORCEEXCHANGECARDS:
      {
        if (iState != STATE_REGISTER)
          {
            if (((MsgForceExchangeCards *)pvMess)->iPlayer >= 0)
              {
#ifdef ENGLISH
                UTIL_DisplayComment("You have too many cards"
                                    " and must exchange a set.");
#endif
#ifdef FRENCH
                UTIL_DisplayComment("Vous avez trop de cartes"
                                    " et devez échanger");
#endif
	        fForceExchange = fCanExchange = TRUE;
	        CBK_ShowCards((Widget)NULL, (XtPointer)NULL, (XtPointer)NULL);

                iState = STATE_PLACE;
                UTIL_ServerEnterState(iState);
              }
          }
      }
      break;

    case MSG_REPLYPACKET:
      iReply = ((MsgReplyPacket *)pvMess)->iReply;
      break;
      
    case MSG_ENDOFMISSION:
      {
	MsgEndOfMission *pmsgEndOfMission = (MsgEndOfMission *)pvMess;
	Int32 iWinner = pmsgEndOfMission->iWinner;

	D_Assert(iWinner >= 0 && iWinner < MAX_PLAYERS, "Bogus Winner!");

	/* Print out a final message */
	GAME_MissionAccomplied(iWinner, pmsgEndOfMission->iTyp,
	                                pmsgEndOfMission->iNum1,
	                                pmsgEndOfMission->iNum2);
      }
      break;

    case MSG_VICTORY:
      {
	Int32 iWinner = ((MsgVictory *)pvMess)->iWinner;

	D_Assert(iWinner >= 0 && iWinner < MAX_PLAYERS, "Bogus Winner!");

	/* Print out a final message */
	GAME_Victory(iWinner);
      }
      break;

    case MSG_ENDOFGAME:
      {
	/* Set the cursor so that the client doesn't have
	 * the icon of the hourglass by any chance, because
	 * it doesn't really make sense.
	 */

	UTIL_SetCursorShape(CURSOR_PLAY);

	/* Print out a final message */
	fHaveSeenFirstPlayer = FALSE;
	GAME_GameOverMan();
      }
      break;

    case MSG_MESSAGEPACKET:
      {
	MsgMessagePacket *pMess = (MsgMessagePacket *)pvMess;
        UTIL_DisplayMessage(pMess->iFrom, pMess->iTo, pMess->strMessage);
      }
      break;
      
    case MSG_TURNNOTIFY:
      { 
#ifdef ASSERTIONS
	Int32           i;
#endif
	MsgTurnNotify  *pTurn = (MsgTurnNotify *)pvMess;

	iCurrentPlayer = pTurn->iPlayer;
	if(!fHaveSeenFirstPlayer) {
	  fHaveSeenFirstPlayer = TRUE;
	  iFirstPlayer = iCurrentPlayer;
	}

	/* Set the color of the player color-coded indicator */
        GUI_SetColorOfCurrentPlayer(COLOR_PlayerToColor(iCurrentPlayer));

	if (pTurn->iClient == CLNT_GetThisClientID())
	  {
	    fPlayingRemotely = FALSE;

	    /* Set the cursor to indicate play */
	    UTIL_SetCursorShape(CURSOR_PLAY);
	    
	    /* See if everyone has finished fortifying their territories.
	     * If they have not, then there is a serious problem.
	     */

	    if (!fGameStarted && (iState == STATE_FORTIFY) && 
		(RISK_GetNumArmiesOfPlayer(iCurrentPlayer) == 0))
	      {
		fGameStarted = TRUE;
		iCountryToFortify = -1;

#ifdef ASSERTIONS
		/* Sanity check */
		for (i=0;
		     i!=RISK_GetNumLivePlayers(); 
		     i++)
		  {
		    Int32 iPlayer = RISK_GetNthLivePlayer(i);
		    Int32 iFirstAttacker;
		    Int32 j;

		    D_Assert(RISK_GetNumArmiesOfPlayer(iPlayer)>=0, 
			     "Bogus number of armies.");

		    D_Assert(RISK_GetNumArmiesOfPlayer(iPlayer) == 0,
			     "This player has armies and shouldn't!");

		    /* I not entirely sure this correct. But it's at least a
		     * little better than before. People who have already had
		     * their first turn are allowed to have cards. --Pac */
		    for(j=0;j!=RISK_GetNumLivePlayers();++j)
		      if(RISK_GetNthLivePlayer(j)==iFirstPlayer)
			break;
		    D_Assert(j!=RISK_GetNumLivePlayers(),
			     "first player isn't alive?");
		    /* negative % positive = negative, unfortunately */
		    iFirstAttacker=j-NUM_COUNTRIES%RISK_GetNumLivePlayers();
		    while(iFirstAttacker<0)
		      iFirstAttacker+=RISK_GetNumLivePlayers();
		    iFirstAttacker=RISK_GetNthLivePlayer(iFirstAttacker);
		    D_Assert(iPlayer < iCurrentPlayer
			     || iPlayer >= iFirstAttacker
			     || RISK_GetNumCardsOfPlayer(iPlayer) == 0,
			     "This player has cards and shouldn't!");
		  }
#endif
	      }
	    
	    if (fGameStarted)
	      {
		/* Force the player to exchange if he or she possesses 
		 * 5 cards or more.
		 */

		if (RISK_GetNumCardsOfPlayer(iCurrentPlayer) >= 5)
		  {
#ifdef ENGLISH
		    UTIL_DisplayComment("You have more than five cards"
					" and must exchange a set.");
#endif
#ifdef FRENCH
		    UTIL_DisplayComment("Vous avez plus de cinq cartes"
					" et devez échanger");
#endif
		    fForceExchange = TRUE;
		    CBK_ShowCards((Widget)NULL, 
				  (XtPointer)NULL, (XtPointer)NULL);
		  }

		/* Go into placing state, give the player as many armies as
		 * he or she deserves by dividing the total number of 
		 * countries owned divided by 3, for a minimum of three
		 * armies.
		 */

		iState = STATE_PLACE;
		UTIL_ServerEnterState(iState);

		/* All of the armies should have been used up */
		D_Assert(RISK_GetNumArmiesOfPlayer(iCurrentPlayer) == 0,
			 "Number of armies should be 0!");

		GAME_SetTurnArmiesOfPlayer(iCurrentPlayer);
	      }
	    else if (iState == STATE_FORTIFY)
	      {
		UTIL_ServerEnterState(iState);
		if (iCountryToFortify != -1)
		  {
		    GAME_PlaceClick(iCountryToFortify, PLACE_ONE);
		    iCountryToFortify = -1;
		    UTIL_DisplayError("");
		  }
	      }
	    UTIL_DisplayActionCString(iState, iCurrentPlayer);
	  }
	else
	  {
	    char buf[256];
	    fPlayingRemotely = TRUE;
	    
	    /* Set the cursor to indicate waiting */
	    UTIL_SetCursorShape(CURSOR_WAIT);

#ifdef ENGLISH
	    snprintf(buf, sizeof(buf), "%s is playing remotely...",
#endif
#ifdef FRENCH
	    snprintf(buf, sizeof(buf), "%s joue à distance...",
#endif
		    RISK_GetNameOfPlayer(iCurrentPlayer));
	    UTIL_DisplayComment(buf);
	  }
      }
      break;
      
    case MSG_EXIT:
      /* Popup telling what happened */
#ifdef ENGLISH
      (void)UTIL_PopupDialog("Exit Notification", 
			     "Terminating game!  Goodbye...",
			     1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
      (void)UTIL_PopupDialog("Notification d'abandon", 
			     "Jeu terminé!  Au revoir...",
			     1, "Ok", NULL, NULL);
#endif
      close(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()));
      CLNT_SetCommLinkOfClient(CLNT_GetThisClientID(), -1);
      UTIL_ExitProgram(0);
      break;
    
    case MSG_MISSION:
      {
        Int32 iPlayer;

        if (!fWaitMission)
          {
            if (UTIL_PlayerIsLocal(iCurrentPlayer))
                iPlayer = iCurrentPlayer;
            else if (UTIL_NumPlayersAtClient(CLNT_GetThisClientID()) == 1)
                iPlayer = RISK_GetNthPlayerAtClient(CLNT_GetThisClientID(), 0);
            else
                iPlayer = -1;
            if (iPlayer != -1)
              {
                GAME_ShowMission(iPlayer);
#ifdef ENGLISH
                (void)UTIL_PopupDialog("Mission Notification", 
			               "The mission is visible in the "
			               "message zone",
			               1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
                (void)UTIL_PopupDialog("Notification de mission", 
			               "La mission est visible dans la "
			               "zone de message",
			               1, "Oui", NULL, NULL);
#endif
              }
            else
                /* Popup telling what happened */
#ifdef ENGLISH
                (void)UTIL_PopupDialog("Mission Notification", 
			               "To view your mission, click on\n"
			               " mission's button",
			               1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
                (void)UTIL_PopupDialog("Notification de mission", 
			               "Pour voir la mission, il faut \n"
			               "utiliser le bouton mission",
			               1, "Oui", NULL, NULL);
#endif
          }
      }
      break;

    default: {
      char buf[256];
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "CALLBACKS: Unhandled message (%d)", 
	       iMessType);
      (void)UTIL_PopupDialog("Fatal Error", buf, 1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "CALLBACKS: message non géré(%d)", 
	      iMessType);
      (void)UTIL_PopupDialog("Erreur fatale", buf, 1, "Ok", NULL, NULL);
#endif
      RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
		       MSG_DEREGISTERCLIENT, NULL);
      UTIL_ExitProgram(-1);
      }
   }
}


/************************************************************************ 
 *  FUNCTION: CBK_RefreshMap
 *  HISTORY: 
 *     01.27.94  ESF  Created.
 *     01.28.94  ESF  Changed to work with pixmap.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_RefreshMap(Widget w, XtPointer pData, XtPointer pCalldata)
{
  XExposeEvent *pExpose = (XExposeEvent *)pCalldata;
  UNUSED(w);
  UNUSED(pData);

  XCopyArea(hDisplay, pixMapImage, hWindow, hGC, 
	    pExpose->x, pExpose->y, pExpose->width, pExpose->height, 
	    pExpose->x, pExpose->y);
}


#define DOUBLE_CLICK_TIME  300

/************************************************************************ 
 *  FUNCTION: CBK_MapClick
 *  HISTORY: 
 *     01.27.94  ESF  Created 
 *     02.03.94  ESF  Don't allocate colors in order, use mapping.
 *     03.03.94  ESF  Added some game glue.
 *     03.07.94  ESF  Made a lean, mean, fighting function.  Look in game.c.
 *     03.17.94  ESF  Fixed remote case.
 *     05.10.94  ESF  Fixed to do nothing unless game has started.
 *     10.02.94  ESF  Added PLACE_ALL option.
 *     06.09.95  JC   Remember on click in fortify mode.
 *     12.29.96  BPK  Added multiple armies placement in fortification
 *     01.17.00  ThH  Now GAME_PlaceClick decides about number placed during fortify
 *  PURPOSE: 
 *  NOTES: Handle click on map
 ************************************************************************/
void CBK_MapClick(Widget w, XEvent *pEvent, String *str, Cardinal *card)
{
  Int32  x, y, iCountry;
  char buf[256];
  UNUSED(w);
  UNUSED(str);
  UNUSED(card);

  if (iState == STATE_REGISTER)
    return;

  /* Find out the coordinates and time of the mouse click */
  x = pEvent->xbutton.x;
  y = pEvent->xbutton.y;

  /* Find out what country was clicked on */
  iCountry = COLOR_ColorToCountry(XGetPixel(pMapImage, x, y));

  if (fPlayingRemotely)
    {
      if (    (UTIL_NumPlayersAtClient(CLNT_GetThisClientID()) == 1)
           && (iState == STATE_FORTIFY) && (iCountry < NUM_COUNTRIES))
        {
          if (iCountryToFortify == -1)
            {
              iCountryToFortify = iCountry;
              snprintf(buf, sizeof(buf), "%s %s",
                       RISK_GetNameOfCountry(iCountryToFortify),
                       TXT_BE_FORTIFIED);
              UTIL_DisplayError(buf);
              return;
            }
        }
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "Click as you may, it's %s's turn...", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "Jouez avec le bouton, mais c'est le tour de %s...", 
#endif
	      RISK_GetNameOfPlayer(iCurrentPlayer));
      UTIL_DisplayError(buf);
      return;
    }

  /* Erase previous errors */
  UTIL_DisplayError("");

  switch (iState)
    {

    case STATE_FORTIFY:
      /*
       * Changed so we can place multiple armies during fortification
       * -- BPK
       * -- TdH GAME_PlaceClick takes care of it now
       * instead fall through */

    case STATE_PLACE:
      if (pEvent->xbutton.button == Button1)
 	GAME_PlaceClick(iCountry, PLACE_ONE);
      else if (pEvent->xbutton.button == Button2)
	GAME_PlaceClick(iCountry, PLACE_ALL);
      else if (pEvent->xbutton.button == Button3)
 	GAME_PlaceClick(iCountry, PLACE_MULTIPLE);
      break;

    case STATE_ATTACK:
      GAME_AttackClick(iCountry);
      break;
      
    case STATE_MOVE:
      GAME_MoveClick(iCountry);
      break;

    default:
      D_Assert(FALSE, "Shouldn't be here!");
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_Quit
 *  HISTORY: 
 *     01.29.94  ESF  Created 
 *     04.02.94  ESF  Added confirmation.
 *     08.28.94  ESF  Fixed to implement cleaner exit.
 *     15.01.00  MSH  Fixed to include Cancel button in English dialog
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_Quit(Widget w, XtPointer pData, XtPointer call_data)
{
  Int32 rep;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

#ifdef ENGLISH
  rep = UTIL_PopupDialog("Quit", "Quit Frisk or new game?", 3, "Quit", "New", "Cancel");
#endif
#ifdef FRENCH
  rep = UTIL_PopupDialog("Quit", "Quitter Frisk ou nouvelle partie?", 3, "Quitter", "Nouvelle", "Annuler");
#endif
  switch (rep)
    {
    case QUERY_YES:
      {
        UTIL_ExitProgram(0);
      }
    case QUERY_NO:
      {
          iState = STATE_REGISTER;
          (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			         MSG_ENDOFGAME, NULL);
      }
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_ShowCards
 *  HISTORY: 
 *     02.19.94  ESF  Created.
 *     03.17.94  ESF  Erase the error display.
 *     05.04.94  ESF  Fixed so that only the owner can see his or her cards.
 *     05.10.94  ESF  Fixed to do nothing unless game has started.
 *     05.12.94  ESF  Fixed to display correct cards.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_ShowCards(Widget w, XtPointer pData, XtPointer call_data)
{
  Int32 i, iPlayer;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

#ifdef CARD_DEBUG
  static Int32 j;
#endif

  if (iState == STATE_REGISTER)
    return;

  UTIL_DisplayError("");

  /* If the player clicking is the current player, show cards, otherwise,
   * if there's more than one player at the client, issue an error, unless
   * there's only one, in which case show his or her cards.
   */

  if (UTIL_PlayerIsLocal(iCurrentPlayer))
    iPlayer = iCurrentPlayer;
  else if (UTIL_NumPlayersAtClient(CLNT_GetThisClientID()) == 1)
    iPlayer = RISK_GetNthPlayerAtClient(CLNT_GetThisClientID(), 0);
  else
    {
      iPlayer = -1;
#ifdef ENGLISH
      (void)UTIL_PopupDialog("Error", "Wait until your turn!", 
			     1, "Ok", "Cancel", NULL);
#endif
#ifdef FRENCH
      (void)UTIL_PopupDialog("Erreur", "Il faut attendre son tour!", 
			     1, "Ok", "Annule", NULL);
#endif
    }

  /* If it's valid, display cards */
  if (iPlayer >= 0)
    {
      XtPopup(wCardShell, XtGrabExclusive);

#ifdef CARD_DEBUG      
      for (i=0; i!=MAX_CARDS; i++)
	{
	  CARD_RenderCard(j, i);
	  j = (++j) % NUM_CARDS;
	}
#else
      for (i=0; i!=RISK_GetNumCardsOfPlayer(iPlayer); i++)
	CARDS_RenderCard(RISK_GetCardOfPlayer(iPlayer, i), i);
#endif
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_CancelCards
 *  HISTORY: 
 *     02.19.94  ESF  Created.
 *     03.17.94  ESF  Added clearing of selections and unmappings.
 *     04.01.94  ESF  Fixed clearing of error when player cancels.
 *     04.11.94  ESF  Added not popping down the shell when !fForceExchange.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_CancelCards(Widget w, XtPointer pData, XtPointer call_data)
{
  Int32 i;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (fForceExchange == TRUE)
#ifdef ENGLISH
    (void)UTIL_PopupDialog("Error", "You must exchange cards!", 1, 
			   "Ok", NULL, NULL);
#endif
#ifdef FRENCH
    (void)UTIL_PopupDialog("Erreur", "Il faut échanger des cartes!", 1, 
			   "Ok", NULL, NULL);
#endif
  else
    {
      UTIL_DisplayError("");
      XtPopdown(wCardShell);
      
      /* Unmap the cards and clear the selections */
      for (i=0; i!=MAX_CARDS; i++)
	{
	  XtUnmanageChild(wCardToggle[i]);
	  XtVaSetValues(wCardToggle[i], XtNstate, False, NULL);
	}
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_ShowMission
 *  HISTORY: 
 *     16.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_ShowMission(Widget w, XtPointer pData, XtPointer call_data)
{
  Int32 iPlayer;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (iState == STATE_REGISTER)
    return;

  UTIL_DisplayError("");

  /* If the player clicking is the current player, show cards, otherwise,
   * if there's more than one player at the client, issue an error, unless
   * there's only one, in which case show his or her cards.
   */

  if (UTIL_PlayerIsLocal(iCurrentPlayer))
    iPlayer = iCurrentPlayer;
  else if (UTIL_NumPlayersAtClient(CLNT_GetThisClientID()) == 1)
    iPlayer = RISK_GetNthPlayerAtClient(CLNT_GetThisClientID(), 0);
  else
    {
      iPlayer = -1;
#ifdef ENGLISH
      (void)UTIL_PopupDialog("Error", "Wait until your turn!", 
			     1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
      (void)UTIL_PopupDialog("Erreur", "Il faut attendre son tour!", 
			     1, "Ok", NULL, NULL);
#endif
    }

  /* If it's valid, display mission */
  if (iPlayer >= 0)
    {
      if (RISK_GetMissionTypeOfPlayer(iPlayer) == NO_MISSION)
        {
#ifdef ENGLISH
          if(UTIL_PopupDialog("Mission", "Play with a mission?", 
			         2, "Ok", "Cancel", NULL) == QUERY_NO)
#endif
#ifdef FRENCH
          if(UTIL_PopupDialog("Mission", "Jouer avec une mission?", 
			         2, "Oui", "Annule", NULL) == QUERY_NO)
#endif
              return;
          fWaitMission = TRUE;
          (void)RISK_SendSyncMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			             MSG_MISSION, NULL,
			             MSG_MISSION, CBK_IncomingMessage);
          fWaitMission = FALSE;
        }
      GAME_ShowMission(iPlayer);
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_Attack
 *  HISTORY: 
 *     03.03.94  ESF  Stubbed.
 *     03.05.94  ESF  Coded.
 *     05.07.94  ESF  Modified to work with Distributed RiskGame Object.
 *     05.10.94  ESF  Fixed to do nothing unless game has started.
 *     05.19.94  ESF  Fixed bug, not refering to DiceMode.
 *     10.15.94  ESF  Added flexibility.
 *     01.17.95  ESF  Free pItem.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_Attack(Widget w, XtPointer pData, XtPointer call_data)
{
  XawListReturnStruct *pItem;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (iState == STATE_REGISTER)
    return;

  /* Don't do anything if current player is not local and there is not
   * one player at this client.
   */

  if (UTIL_PlayerIsLocal(iCurrentPlayer) == FALSE)
    return;
    
  /* I know this is silly, but hey... (We should get the data from pData?) */
  pItem = XawListShowCurrent(wAttackList);
  RISK_SetDiceModeOfPlayer(iCurrentPlayer, pItem->list_index);
  
  /* Free up memory */
  XtFree((void *)pItem);
}


/************************************************************************ 
 *  FUNCTION: CBK_Action
 *  HISTORY: 
 *     03.03.94  ESF  Stubbed.
 *     03.05.94  ESF  Coded.
 *     03.29.94  ESF  Added player attack mode history.
 *     05.07.94  ESF  Modified to work with Distributed RiskGame Object.
 *     05.10.94  ESF  Fixed to do nothing unless game has started.
 *     11.19.94  ESF  Added a few UTIL_DisplayError("")'s, where needed.
 *     01.17.95  ESF  Free pItem.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_Action(Widget w, XtPointer pData, XtPointer call_data)
{
  XawListReturnStruct *pItem;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (iState == STATE_REGISTER)
    return;

  /* Don't do anything if current player is not local */
  if (!UTIL_PlayerIsLocal(iCurrentPlayer))
    return;

  /* I know this is silly, but hey... (We should get the data from pData?) */
  pItem = XawListShowCurrent(wActionList);

  switch (pItem->list_index)
    {
    /*****************/
    case ACTION_PLACE:
    /*****************/
      if (iState == STATE_PLACE || iState == STATE_FORTIFY)
	{
	  UTIL_DisplayError("");
	  iActionState = pItem->list_index;
	}
      else /* Invalid */
	{
#ifdef ENGLISH
	  UTIL_DisplayError("You've already placed your armies.");
#endif
#ifdef FRENCH
	  UTIL_DisplayError("Toutes vos armées sont déjà placée.");
#endif
	  XawListHighlight(wActionList, ACTION_ATTACK);
	}

      break;

    /*******************/
    case ACTION_ATTACK:
    case ACTION_DOORDIE:
    /*******************/
      if (iState == STATE_ATTACK)
	{
	  UTIL_DisplayError("");
	  iActionState = pItem->list_index;

	  /* Keep track of the type of attack selected */
	  RISK_SetAttackModeOfPlayer(iCurrentPlayer, pItem->list_index);
	}	
      else if (iState == STATE_PLACE || iState == STATE_FORTIFY)
	{
#ifdef ENGLISH
	  UTIL_DisplayError("You must finish placing your armies.");
#endif
#ifdef FRENCH
	  UTIL_DisplayError("Il faut finir de placer vos armées.");
#endif
	  XawListHighlight(wActionList, ACTION_PLACE);
	}
      else if (iState == STATE_MOVE) /* Must not have moved yet */
	{
	  /* Keep track of the type of attack selected */
	  RISK_SetAttackModeOfPlayer(iCurrentPlayer, pItem->list_index);

	  iState = STATE_ATTACK;
	  UTIL_DisplayError("");
	  UTIL_ServerEnterState(iState);
	  UTIL_DisplayActionCString(iState, iCurrentPlayer);
	}

      break;
      
    /****************/
    case ACTION_MOVE:
    /****************/
      if (iState == STATE_MOVE)
	{
	  UTIL_DisplayError("");
	  iActionState = pItem->list_index;
	}
      else if (iState == STATE_FORTIFY || iState == STATE_PLACE)
	{
#ifdef ENGLISH
	  UTIL_DisplayError("You must finish placing your armies.");
#endif
#ifdef FRENCH
	  UTIL_DisplayError("Il faut finir de placer vos armées.");
#endif
	  XawListHighlight(wActionList, ACTION_PLACE);
	}
      else /* STATE_ATTACK */
	{
	  iState = STATE_MOVE;
	  UTIL_DisplayError("");
	  UTIL_ServerEnterState(iState);
	  UTIL_DisplayActionCString(iState, iCurrentPlayer);
	}
      break;

    default:
      D_Assert(FALSE, "Shouldn't be here -- bogus state!");
    }

  /* Free up memory */
  XtFree((void *)pItem);
}


/************************************************************************ 
 *  FUNCTION: CBK_MsgDest
 *  HISTORY: 
 *     03.03.94  ESF  Stubbed.
 *     05.07.94  ESF  Created and modified to work with Distributed Object.
 *     05.19.94  ESF  Fixed a bug, wasn't refering to MsgDestList.
 *     01.17.95  ESF  Free pItem.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_MsgDest(Widget w, XtPointer pData, XtPointer call_data)
{
  XawListReturnStruct *pItem;
  Int32 iPlayer;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (UTIL_PlayerIsLocal(iCurrentPlayer))
    iPlayer = iCurrentPlayer;
  else if (UTIL_NumPlayersAtClient(CLNT_GetThisClientID()) == 1)
    iPlayer = RISK_GetNthPlayerAtClient(CLNT_GetThisClientID(), 0);
  else
      iPlayer = -1;
  if (iPlayer == -1)
    return;
  
  /* I know this is silly, but hey... (We should get the data from pData?) */
  pItem = XawListShowCurrent(wMsgDestList);
  RISK_SetMsgDstModeOfPlayer(iPlayer, pItem->list_index);

  /* Free up memory */
  XtFree((void *)pItem);
}


/************************************************************************ 
 *  FUNCTION: CBK_SendMessage
 *  HISTORY: 
 *     05.03.94  ESF  Created.
 *     05.04.94  ESF  Enhanced.
 *     10.02.94  ESF  Moved here and fixed a few (serious) bugs.
 *     01.09.95  ESF  Changed to name sender if current player on client.
 *     01.17.95  ESF  Free pItem.
 *     29.08.95  JC   strFrom -> iFrom, iTo now contain the iPlayer.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_SendMessage(void)
{
  CString                strMess;
  XawListReturnStruct   *pItem;
  MsgMessagePacket       mess;

  /* Get the message and destination from widgets */
  XtVaGetValues(wSendMsgText, XtNstring, &strMess, NULL);
  pItem = XawListShowCurrent(wMsgDestList);

  /* Who's sending the message? */
  if (UTIL_NumPlayersAtClient(CLNT_GetThisClientID()) == 1)
    mess.iFrom = RISK_GetNthPlayerAtClient(CLNT_GetThisClientID(), 0);
  else if (RISK_GetClientOfPlayer(iCurrentPlayer) == CLNT_GetThisClientID())
    mess.iFrom = iCurrentPlayer;
  else
    mess.iFrom = FROM_UNKNOW;

  /* Who do we send the message to? */
  if (pItem->list_index == -1 || pItem->list_index == 0)
    mess.iTo = DST_ALLPLAYERS;
  else
    {
      mess.iTo = piMsgDstPlayerID[pItem->list_index];
      D_Assert(mess.iTo!=-1, "Bogus player!");
    }

  mess.strMessage = strMess;

  /* Display the message locally if necessary */
  if (mess.iTo != DST_ALLPLAYERS && 
      RISK_GetClientOfPlayer(mess.iTo) != CLNT_GetThisClientID()) {
    UTIL_DisplayMessage(mess.iFrom, mess.iTo, mess.strMessage);
  }
  /* Send the message */
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_MESSAGEPACKET, &mess);
  
  /* Erase the old message */
  XtVaSetValues(wSendMsgText, XtNstring, "", NULL);

  /* Free up memory */
  XtFree((void *)pItem);
}


/************************************************************************ 
 *  FUNCTION: CBK_CancelAttack
 *  HISTORY: 
 *     03.03.94  ESF  Stubbed.
 *     03.04.94  ESF  Coded.
 *     05.10.94  ESF  Fixed to do nothing unless game has started.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_CancelAttack(Widget w, XtPointer pData, XtPointer call_data)
{
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (iState == STATE_REGISTER)
    return;

  if (fPlayingRemotely)
    {
#ifdef ENGLISH
      UTIL_DisplayError("Nice try, but it isn't your turn...");
#endif
#ifdef FRENCH
      UTIL_DisplayError("Bien essayé, mais ce n'est pas votre tour...");
#endif
      return;
    }

  UTIL_DisplayError("");

  if (iState == STATE_ATTACK)
    {
      if (GAME_AttackFrom() >=0)
	UTIL_DarkenCountry(GAME_AttackFrom());
      GAME_SetAttackSrc(-1);
    }
  else if (iState == STATE_MOVE)
    {
      if (iMoveSrc >=0)
	UTIL_DarkenCountry(iMoveSrc);
      iMoveSrc = -1;
    }

  UTIL_DisplayActionCString(iState, iCurrentPlayer);
}


/************************************************************************ 
 *  FUNCTION: CBK_Repeat
 *  HISTORY: 
 *     03.03.94  ESF  Stubbed.
 *     03.06.94  ESF  Coded.
 *     03.07.94  ESF  Fixed state bug.
 *     03.08.94  ESF  Added DOORDIE recognition.
 *     03.16.94  ESF  Fixed bug in DOORDIE.
 *     04.02.94  ESF  Added server notification caching for do-or-die.
 *     05.10.94  ESF  Fixed to do nothing unless game has started.
 *     05.19.94  ESF  Added verbose message across network.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_Repeat(Widget w, XtPointer pData, XtPointer call_data)
{
  MsgNetMessage mess;
  char buf[256];
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (iState == STATE_REGISTER)
    return;

  if (fPlayingRemotely)
    {
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "I'm afraid that's up to %s to decide...", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), 
	       "Je suis effrayé par ce que vous décidez pour %s...", 
#endif
	      RISK_GetNameOfPlayer(iCurrentPlayer));
      UTIL_DisplayError(buf);
      return;
    }

  if (iState != STATE_ATTACK)
#ifdef ENGLISH
    UTIL_DisplayError("You can't do any attacking at the moment.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Il n'y a rien à faire pour l'instant.");
#endif
  else if (iActionState == ACTION_ATTACK || iActionState == ACTION_DOORDIE)
    {
      if (iLastAttackSrc == -1 || iLastAttackDst == -1)
	{
#ifdef ENGLISH
	  UTIL_DisplayError("You can't repeat an uncomplete attack.");
#endif
#ifdef FRENCH
	  UTIL_DisplayError("Impossible de répéter une attaque incomplète.");
#endif
	  return;
	}

      /* send a verbose message */
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s is attacking from %s to %s",
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      RISK_GetNameOfCountry(iLastAttackSrc),
	      RISK_GetNameOfCountry(iLastAttackDst));
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s attaque %s depuis %s",
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      RISK_GetNameOfCountry(iLastAttackDst),
	      RISK_GetNameOfCountry(iLastAttackSrc));
#endif
      mess.strMessage = buf;
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_NETMESSAGE, &mess);
      
      /* Repeat the attack */
      GAME_Attack(iLastAttackSrc, iLastAttackDst);
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_Help
 *  HISTORY: 
 *     04.01.94  ESF  Coded.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_Help(Widget w, XtPointer pData, XtPointer call_data)
{
  Int32 x, y;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  /* Center popup */
  UTIL_CenterShell(wHelpShell, wToplevel, &x, &y);
  XtVaSetValues(wHelpShell, 
		XtNallowShellResize, False,
		XtNx, x, 
		XtNy, y, 
		XtNborderWidth, 1,
#ifdef ENGLISH
		XtNtitle, "Frisk Help",
#endif
#ifdef FRENCH
		XtNtitle, "Aide de frisk",
#endif
		NULL);

  XtPopup(wHelpShell, XtGrabNone);
}


/************************************************************************ 
 *  FUNCTION: CBK_HelpSelectTopic
 *  HISTORY: 
 *     04.01.94  ESF  Coded.
 *     01.17.95  ESF  Free pItem.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_HelpSelectTopic(Widget w, XtPointer pData, XtPointer call_data)
{
  XawListReturnStruct *pItem;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);
  
  /* I know this is sill, but hey... (We should get the data from pData) */
  pItem = XawListShowCurrent(wHelpTopicList);
  HELP_IndexPopupHelp(pItem->list_index);

  /* Free up memory */
  XtFree((void *)pItem);
}


/************************************************************************ 
 *  FUNCTION: CBK_HelpOk
 *  HISTORY: 
 *     04.01.94  ESF  Coded.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_HelpOk(Widget w, XtPointer pData, XtPointer call_data)
{
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);
  XtPopdown(wHelpShell);
}


/************************************************************************ 
 *  FUNCTION: CBK_ExchangeCards
 *  HISTORY: 
 *     03.03.94  ESF  Stubbed.
 *     03.29.94  ESF  Coded.
 *     05.03.94  ESF  Fixed for when the player has many cards.
 *     02.21.95  ESF  Fixed a bug -- only current player may exchange.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_ExchangeCards(Widget w, XtPointer pData, XtPointer call_data)
{
  Int32   i, iNumCardsSelected;
  Flag    iCardState; 
  Int32   piCards[3], piCardValues[3], piCardTypes[3];
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);
  
  /* If it's not the current player, then don't let the exchange
   * take place.  This fixes a nasty bug reported by many.
   */

  if (RISK_GetClientOfPlayer(iCurrentPlayer) != CLNT_GetThisClientID())
    UTIL_DisplayError(TXT_ONLY_CURRENT);
  /* Get the cards that have been pressed, check for validity */
  for (i=iNumCardsSelected=0; i!=MAX_CARDS; i++)
  {
      XtVaGetValues(wCardToggle[i], XtNstate, &iCardState, NULL);
      if (iCardState == True)
      {
          if (iNumCardsSelected<3)
              piCards[iNumCardsSelected] = i;
          iNumCardsSelected++;
      }
  }

  /* Clear the selections of the cards */
  for (i=0; i!=MAX_CARDS; i++)
    XtVaSetValues(wCardToggle[i], XtNstate, False, NULL);

  /* Valid number of cards selected */
  if (fCanExchange == FALSE)
    {
#ifdef ENGLISH
      UTIL_DisplayError("You can only exchange cards at the beginning of "
			"your turn.");
#endif
#ifdef FRENCH
      UTIL_DisplayError("Les cartes ne sont échangeables qu'au début de "
			"votre tour.");
#endif
      return;
    }
  else if (iNumCardsSelected!=3)
    {
#ifdef ENGLISH
      UTIL_DisplayError("You must select three cards to exchange.");
#endif
#ifdef FRENCH
      UTIL_DisplayError("Il faut sélectionner trois cartes pour pouvoir "
                        "échanger.");
#endif
      return;
    }
  
  /* Substitute the indices for the card values */
  for (i=0; i!=3; i++)
    {
      piCardValues[i] = RISK_GetCardOfPlayer(iCurrentPlayer, piCards[i]);

      /* Set the type of the card */
      if (piCardValues[i]<NUM_COUNTRIES)
	piCardTypes[i] = piCardValues[i] % 3;
      else
	piCardTypes[i] = -1;  /* Joker */
    }
  
  /* Now see if the cards form a valid triple */
  if ((piCardTypes[0]==piCardTypes[1] && 
       piCardTypes[1]==piCardTypes[2] && 
       piCardTypes[0]==piCardTypes[2]) ||
      (piCardTypes[0]!=piCardTypes[1] && 
       piCardTypes[1]!=piCardTypes[2] && 
       piCardTypes[0]!=piCardTypes[2]) ||
      (piCardTypes[0]==-1 || piCardTypes[1]==-1 || piCardTypes[2]==-1))
    {
      /* Actually perform the exchange */
      GAME_ExchangeCards(piCards);

      /* If this is the only exchange, then pop down the window */
      if (RISK_GetNumCardsOfPlayer(iCurrentPlayer) <= 5)
	{
	  /* We're through exchanging sets of cards */
	  fForceExchange = FALSE;

	  XtPopdown(wCardShell);
	  
	  /* Unmap the cards and clear the selections */
	  for (i=0; i!=MAX_CARDS; i++)
	    XtUnmanageChild(wCardToggle[i]);
	}
      else
	{
	  /* Redisplay cards after erasing and unselecting the old ones */
	  for (i=0; i!=MAX_CARDS; i++)
	    {
	      XtUnmanageChild(wCardToggle[i]);
	      XtVaSetValues(wCardToggle[i], XtNstate, False, NULL);
	    }
	  
	  for (i=0; i!=RISK_GetNumCardsOfPlayer(iCurrentPlayer); i++)
	    CARDS_RenderCard(RISK_GetCardOfPlayer(iCurrentPlayer, i), i);
	}

      /* Display the appropriate message for the state */
      UTIL_DisplayActionCString(iState, iCurrentPlayer);
    }
  else
#ifdef ENGLISH
    UTIL_DisplayError("You must pick three cards of the same type or one of "
		      "each type.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Vous devez choisir trois cartes du même type ou une de "
		      "chaque type.");
#endif
}


/************************************************************************ 
 *  FUNCTION: CBK_EndTurn
 *  HISTORY: 
 *     03.05.94  ESF  Created.
 *     05.10.94  ESF  Fixed to do nothing unless game has started.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_EndTurn(Widget w, XtPointer pData, XtPointer call_data)
{
  char buf[256];
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  if (iState == STATE_REGISTER)
    return;

  if (fPlayingRemotely)
    {
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "Come on, let %s finish playing...", 
	      RISK_GetNameOfPlayer(iCurrentPlayer));
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "Come on, let %s finish playing...", 
	      RISK_GetNameOfPlayer(iCurrentPlayer));
#endif
      UTIL_DisplayError(buf);
      return;
    }
  
  if (iState == STATE_PLACE || iState == STATE_FORTIFY)
#ifdef ENGLISH
    UTIL_DisplayError("You must finish placing your armies.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Vous devez finir de placer vos armées.");
#endif
  else
    {
      UTIL_DisplayError("");
      GAME_EndTurn();
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_RefreshDice
 *  HISTORY: 
 *     04.11.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_RefreshDice(Widget w, XtPointer pData, XtPointer call_data)
{
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  DICE_Refresh();
}


/************************************************************************ 
 *  FUNCTION: CBK_About
 *  HISTORY: 
 *     10.02.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_About(Widget w, XtPointer pData, XtPointer call_data)
{
  char buf[256];
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  /* Print version information */
#ifdef ENGLISH
  snprintf(buf, sizeof(buf), "This is Frisk version %s, compiled %s at %s", 
	  VERSION, __DATE__, __TIME__);
#endif
#ifdef FRENCH
  snprintf(buf, sizeof(buf), 
	   "Ceci est la version %s de Frisk, compilée le %s à %s", 
	  VERSION, __DATE__, __TIME__);
#endif
  UTIL_DisplayComment(buf);
}


/************************************************************************ 
 *  FUNCTION: CBK_Replicate
 *  HISTORY: 
 *     02.26.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_Replicate(Int32 iMessType, void *pvMess, Int32 iType, Int32 iSrc)
{
  UNUSED(iSrc);

  /*
   * Do the physical replication (send to the server so it will 
   * broadcast it), but only if this callback is being called 
   * for an outgoing message (i.e. a replication).  Otherwise, 
   * a message just came in.
   */

  if (iType == MESS_OUTGOING)
    {
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			     iMessType, pvMess);
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_Callback
 *  HISTORY: 
 *     05.03.94  ESF  Created.
 *     08.18.94  ESF  Changed so that this does replication.
 *     01.09.95  ESF  Changed to not handle player death messages.
 *     01.17.95  ESF  Added to handle MsgDst changes entirely here.
 *     02.26.95  ESF  Modified to look more like a controller in MVC.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_Callback(Int32 iMessType, void *pvMess)
{
  /* Based on the message from the Model, perform some sort of action */
  switch (iMessType)
    {
    case MSG_OBJINTUPDATE:
      {
	MsgObjIntUpdate *pMess = (MsgObjIntUpdate *)pvMess;
	switch(pMess->iField)
	  {
	  case CNT_OWNER:
	    {
	      /* Recolor the country */
	      if (pMess->iNewValue >= 0)
		  COLOR_ColorCountry(pMess->iIndex1, pMess->iNewValue);
	    }
	    break;
	    
	  case CNT_NUMARMIES:
	    {
	      /* Redraw the number of armies on the country.  Since the
	       * country may be lit up because of the notification, we
	       * take pains not to mess with it.  Check to see if it was
	       * lit up, and if so, leave it that way.
	       */

	      UTIL_PrintArmies(pMess->iIndex1, pMess->iNewValue,
			       CLNT_GetLightCountOfCountry(pMess->iIndex1)>0 ?
			       WhitePixel(hDisplay, 0) : 
			       BlackPixel(hDisplay, 0));
	    }
	    break;

  	  case PLR_ALLOCATION:
  	  {
  	    /* If a player was killed then remove it from the MsgDst list */
  	    if (pMess->iNewValue == ALLOC_NONE)
  	      {
  		Int32 iDeadPlayer = pMess->iIndex1;
  		Int32 i, iIndex;
  		Flag  fDone = FALSE;

  		/* Search for the entry to delete it */
  		for (i=1; i!=iIndexMD && !fDone; i++)
  		  if (piMsgDstPlayerID[i] == iDeadPlayer)
  		    fDone = TRUE;
  
  		D_Assert(fDone, "Something's wierd!");
  
  		/* Cause we want to recover somewhat if not debugging */
  		if (!fDone)
  		  break;
  
  		/* We found the entry.  Delete it and move the others down */
  		iIndex = (i-1);
  		
  		MEM_Free(pstrMsgDstCString[iIndex]);
  		piMsgDstPlayerID[iIndex] = -1;
  		iIndexMD--;
  		
  		for (i=iIndex; i<iIndexMD; i++)
  		  {
  		    pstrMsgDstCString[i] = pstrMsgDstCString[i+1];
  		    piMsgDstPlayerID[i] = piMsgDstPlayerID[i+1];
  		  }
  
  		UTIL_RefreshMsgDest(iIndexMD);
  	      }
	    /* If the player was created, add it to the message box */
	    else if (pMess->iNewValue == ALLOC_COMPLETE)
	      {
		/* A new player, add this to the list, update listbox */
		char buf[256];
		pstrMsgDstCString[iIndexMD] = 
		  (CString)MEM_Alloc(strlen(
				      RISK_GetNameOfPlayer(pMess->iIndex1))+1);
		piMsgDstPlayerID[iIndexMD] = pMess->iIndex1;
		strcpy(pstrMsgDstCString[iIndexMD], 
		       RISK_GetNameOfPlayer(pMess->iIndex1));
		UTIL_RefreshMsgDest(++iIndexMD);
		
		/* Also create a message that tells of the new player */
#ifdef ENGLISH
		snprintf(buf, sizeof(buf), "%s (%s) has joined.", 
#endif
#ifdef FRENCH
		snprintf(buf, sizeof(buf), "%s (%s) est arrivé.", 
#endif
			RISK_GetNameOfPlayer(pMess->iIndex1), 
			RISK_GetNameOfSpecies(
				    RISK_GetSpeciesOfPlayer(pMess->iIndex1)));
		UTIL_DisplayMessage(FROM_SERVER, DST_OTHER, buf);
	      }
  	  }
  	  break;
	    
	  default:
	    /* A message that we don't care about */
	    ; 
	  }
	break;
      }
      
    case MSG_OBJSTRUPDATE:
      {
	MsgObjStrUpdate *pMess = (MsgObjStrUpdate *)pvMess;
	switch (pMess->iField)
	  {
	  case PLR_COLORSTRING:
	    {
	      /* Allocate the color */
	      COLOR_StoreNamedColor(pMess->strNewValue, pMess->iIndex1);
	    }
	    break;
	    
	  default:
	    /* A message that we don't care about */
	    ;
	  }
      }
      break;
      
    default:
      /* Nothing... */
      ;
    }
}
