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
 *   $Id: game.c,v 1.13 2000/01/18 20:07:26 morphy Exp $
 *
 *   $Log: game.c,v $
 *   Revision 1.13  2000/01/18 20:07:26  morphy
 *   Made middle button (drop all armies) obey DROP_ARMIES limitation
 *
 *   Revision 1.12  2000/01/18 19:56:43  morphy
 *   Prevention of dropping all armies works now
 *
 *   Revision 1.11  2000/01/18 09:25:47  tony
 *   oops, little braino in "dropping armies" code
 *
 *   Revision 1.10  2000/01/17 21:09:03  tony
 *   small fix on dropping armies during fortification, optional number
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "registerPlayers.h"
#include "callbacks.h"
#include "utils.h"
#include "dice.h"
#include "game.h"
#include "debug.h"
#include "riskgame.h"
#include "types.h"
#include "colormap.h"
#include "client.h"


/* Globals */
static MsgNetMessage mess;

static Int32         iAttackDst=-1;
static Int32         iMoveDst=-1;

/*iAttackSrc is used by callbacks.c */
static Int32         iAttackSrc=-1;

Int32         iLastAttackSrc=-1, iLastAttackDst=-1;
Int32         iMoveSrc=-1;

Flag          fGetsCard=FALSE;
Flag          fCanExchange=TRUE;

/* max number of armies to allow when not using DROP_ALL */
#define DROP_ARMIES 3

/************************************************************************ 
 *  FUNCTION: GAME_AttackFrom
 *  HISTORY:  291099  Tdh
 *  PURPOSE:  Return value of iAttackSrc
 *  NOTES: to make iAttackSrc private to game.c
 */
Int32 GAME_AttackFrom() {
    return iAttackSrc;
}

/************************************************************************ 
 *  FUNCTION: GAME_SetAttackSrc
 *  HISTORY:  291099  Tdh
 *  PURPOSE:  Set value of iAttackSrc
 *  NOTES:    to make iAttackSrc private to game.c
 */

void GAME_SetAttackSrc(Int32 src) {
    iAttackSrc = src;
}


/************************************************************************ 
 *  FUNCTION: GAME_CanAttack
 *  HISTORY: 
 *     03.04.94  ESF  Created.
 *  PURPOSE: Check if this attack is possible
 *  NOTES: 
 ************************************************************************/
Flag GAME_CanAttack(Int32 AttackSrc, Int32 iAttackDst)
{
  Int32 i;

  for (i=0; i!=6 && RISK_GetAdjCountryOfCountry(AttackSrc, i)!=-1; i++)
    if (RISK_GetAdjCountryOfCountry(AttackSrc, i) == iAttackDst)
      return TRUE;
  
  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_SetTurnArmiesOfPlayer
 *  HISTORY: 
 *     04.23.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_SetTurnArmiesOfPlayer(Int32 iCurrentPlayer)
{
  Int32 iNumArmies=0;

  /* Add the continent bonus */
  iNumArmies += GAME_GetContinentBonus(iCurrentPlayer);

  /* Add the standard "at-least-three-and-at-most-one-per-three-countries" */
  iNumArmies += MAX(RISK_GetNumCountriesOfPlayer(iCurrentPlayer)/3, 3);

  RISK_SetNumArmiesOfPlayer(iCurrentPlayer, iNumArmies);
}


/************************************************************************ 
 *  FUNCTION: GAME_MoveArmies
 *  HISTORY: 
 *     03.04.94  ESF  Created.
 *     03.18.94  ESF  Added server update.
 *     05.19.94  ESF  Added verbose message across network.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_MoveArmies(Int32 iSrcCountry, Int32 iDstCountry, Int32 iNumArmies)
{
  MsgMoveNotify msgMoveNotify;
  char buf[256];

  /* Notify the server */
  msgMoveNotify.iSrcCountry = iSrcCountry;
  msgMoveNotify.iDstCountry = iDstCountry;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_MOVENOTIFY, &msgMoveNotify);

  /* send a verbose message */
#ifdef ENGLISH
  snprintf(buf, sizeof(buf), "%s moved %d armies from %s to %s",
#endif
#ifdef FRENCH
  snprintf(buf, sizeof(buf), "%s déplace %d armée(s) de %s à %s",
#endif
	  RISK_GetNameOfPlayer(iCurrentPlayer),
	  iNumArmies,
	  RISK_GetNameOfCountry(iSrcCountry),
	  RISK_GetNameOfCountry(iDstCountry));
  mess.strMessage = buf;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_NETMESSAGE, &mess);

  /* Update date structures */
  RISK_SetNumArmiesOfCountry(iDstCountry, 
		   RISK_GetNumArmiesOfCountry(iDstCountry) + iNumArmies);
  RISK_SetNumArmiesOfCountry(iSrcCountry, 
		   RISK_GetNumArmiesOfCountry(iSrcCountry) - iNumArmies);
}


/************************************************************************ 
 *  FUNCTION: GAME_PlaceArmies
 *  HISTORY: 
 *     03.04.94  ESF  Stubbed.
 *     05.19.94  ESF  Coded and added verbose message across network.
 *     10.02.94  ESF  Added support for PLACE_ALL.
 *     04.30.95  ESF  Moved the dialog code from to PlaceClick.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_PlaceArmies(Int32 iCountry, Int32 iArmies)
{
  MsgPlaceNotify  msgPlaceNotify;
  char buf[256];

  /* Send a message to the server about this */
  msgPlaceNotify.iCountry = iCountry;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_PLACENOTIFY, &msgPlaceNotify);

  /* Send a verbose message */
#ifdef ENGLISH
  snprintf(buf, sizeof(buf), "%s placing %d %s on %s",
#endif
#ifdef FRENCH
  snprintf(buf, sizeof(buf), "%s place %d %s en %s",
#endif
	  RISK_GetNameOfPlayer(iCurrentPlayer),
	  iArmies, 
	  iArmies == 1 ? "army" : "armies",
	  RISK_GetNameOfCountry(iCountry));
  mess.strMessage = buf;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_NETMESSAGE, &mess);

  /* Once we've placed, can't exchange cards anymore */
  fCanExchange = FALSE;
  
  /* Update the data structures */
  RISK_SetNumArmiesOfPlayer(iCurrentPlayer, 
			    RISK_GetNumArmiesOfPlayer(iCurrentPlayer)
			    - iArmies);
  RISK_SetNumArmiesOfCountry(iCountry,
			     RISK_GetNumArmiesOfCountry(iCountry)+iArmies);
  
  /* If we're out of armies, jump into attack mode */
  if (!RISK_GetNumArmiesOfPlayer(iCurrentPlayer) && iState == STATE_PLACE)
    {
      iState = STATE_ATTACK;
      UTIL_ServerEnterState(iState);
      UTIL_DisplayActionCString(iState, iCurrentPlayer);
    }
  
  UTIL_DisplayActionCString(iState, iCurrentPlayer);
}


/************************************************************************ 
 *  FUNCTION: GAME_Attack
 *  HISTORY: 
 *     04.23.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_Attack(Int32 iSrcCountry, Int32 iDstCountry)
{
  Int32 iActionState = RISK_GetAttackModeOfPlayer(iCurrentPlayer);
  Int32 iDice        = RISK_GetDiceModeOfPlayer(iCurrentPlayer);

  /* Keep these so as to be able to repeat attack */
  iLastAttackSrc = iSrcCountry;	  
  iLastAttackDst = iDstCountry;
  
  if (iActionState == ACTION_ATTACK)
    {
      if (GAME_ValidAttackSrc(iSrcCountry, TRUE) &&
	  GAME_ValidAttackDst(iSrcCountry, iDstCountry, TRUE) &&
	  GAME_ValidAttackDice(iDice, iSrcCountry))
	{
	  GAME_DoAttack(iSrcCountry, iDstCountry, TRUE);
	  UTIL_DisplayActionCString(iState, iCurrentPlayer);
	}
    }
  else if (iActionState == ACTION_DOORDIE)
    {
      Flag  fNotify = TRUE;
      
      while (GAME_ValidAttackSrc(iSrcCountry, FALSE) &&
	     GAME_ValidAttackDst(iSrcCountry, iDstCountry, FALSE) &&
	     GAME_ValidAttackDice(iDice, iSrcCountry))
	{
	  GAME_DoAttack(iSrcCountry, iDstCountry, fNotify);
	  
	  /* Only True the first time around */
	  fNotify = FALSE;
	}
    }
}


/************************************************************************ 
 *  FUNCTION: GAME_DoAttack
 *  HISTORY: 
 *     03.04.94  ESF  Created.
 *     03.06.94  ESF  Added missing pieces.
 *     03.07.94  ESF  Fixed owner update bug & army subtraction bug.
 *     03.18.94  ESF  Added server notification.
 *     03.28.94  ESF  Added message when country is taken.
 *     03.29.94  ESF  Added more informative moving message.
 *     04.02.94  ESF  Fixed bug, darken country if wrong number of die.
 *     04.02.94  ESF  Only notify server if fCacheNotify == FALSE.
 *     05.04.94  ESF  Fixed bug, changed location of call to GAME_PlayerDied().
 *     05.04.94  ESF  Fixed bug, when end of game occurs.
 *     05.19.94  ESF  Added verbose notification when player takes country.
 *     05.19.94  ESF  Moved calculation of dice correctness to ValidDice.
 *     11.06.94  ESF  Cached results, so that Do-or-die runs quicker.
 *     11.06.94  ESF  Added attack notification.
 *     01.17.95  ESF  Changed fCacheNotify to fNotify.
 *     07.09.95  JC   Fixed a bug with ENDOFMISSION and FORCEEXCHANGECARDS.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_DoAttack(Int32 iSrcCountry, Int32 iDstCountry, Flag fNotify)
{
  MsgAttackNotify  msgAttackNotify;
  Int32            iAttackDie, iDefendDie, iArmiesWon, iArmiesMove;
  Int32            iSrc, iDst, iPlayerAttackDie;
  char buf[256];

  /* Cached values */
  static Int32 iCachedSrcCountry=-1;
  static Int32 iCachedDstCountry=-1;

  iPlayerAttackDie = RISK_GetDiceModeOfPlayer(iCurrentPlayer);
  
  /* If the dice have been selected automatically, calculate them */
  if (iPlayerAttackDie != ATTACK_AUTO)
    iAttackDie = iPlayerAttackDie+1;
  else
    iAttackDie = MIN(RISK_GetNumArmiesOfCountry(iSrcCountry)-1, 3);
  
  iDefendDie = MIN(RISK_GetNumArmiesOfCountry(iDstCountry), 2);
  
  /* Set the color of the dice if the attack has changed */
  if (iCachedSrcCountry != iSrcCountry)
      COLOR_CopyColor(COLOR_PlayerToColor(RISK_GetOwnerOfCountry(iSrcCountry)),
		      COLOR_DieToColor(0));
  if (iCachedDstCountry != iDstCountry)
      COLOR_CopyColor(COLOR_PlayerToColor(RISK_GetOwnerOfCountry(iDstCountry)),
		      COLOR_DieToColor(1));

  /* Get the number of armies that the attacker won */
  iArmiesWon = DICE_Attack(iAttackDie, iDefendDie, 
			   RISK_GetOwnerOfCountry(iSrcCountry), 
			   RISK_GetOwnerOfCountry(iDstCountry));
  
  RISK_SetNumArmiesOfCountry(iDstCountry, 
	      RISK_GetNumArmiesOfCountry(iDstCountry) - iArmiesWon);
  RISK_SetNumArmiesOfCountry(iSrcCountry,
	      RISK_GetNumArmiesOfCountry(iSrcCountry) - 
	      (MIN(iDefendDie, iAttackDie) - iArmiesWon));

  /* Country was taken */
  if (!RISK_GetNumArmiesOfCountry(iDstCountry))
    {
      /* The attacking player gets a card */
      fGetsCard = TRUE;

      /* Send a verbose message */
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s captured %s from %s",
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s capture %s à partir de %s",
#endif
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      RISK_GetNameOfCountry(iDstCountry),
	      RISK_GetNameOfCountry(iSrcCountry));
      mess.strMessage = buf;
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_NETMESSAGE, &mess);
      
      /* Display informative message telling of victory */
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s moving armies from %s to %s.", 
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      RISK_GetNameOfCountry(iSrcCountry),
	      RISK_GetNameOfCountry(iDstCountry));
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s conquière %s à partir de %s.", 
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      RISK_GetNameOfCountry(iDstCountry),
	      RISK_GetNameOfCountry(iSrcCountry));
#endif
      UTIL_DisplayComment(buf);

      /* Update data structures */
      iSrc = RISK_GetOwnerOfCountry(iSrcCountry);
      RISK_SetNumCountriesOfPlayer(iSrc, RISK_GetNumCountriesOfPlayer(iSrc)+1);
      iDst = RISK_GetOwnerOfCountry(iDstCountry);
      RISK_SetNumCountriesOfPlayer(iDst, RISK_GetNumCountriesOfPlayer(iDst)-1);

      /* New owner */
      RISK_SetOwnerOfCountry(iDstCountry, RISK_GetOwnerOfCountry(iSrcCountry));

      /* See if this victory signalled a _MSG_ENDOFMISSION condition */
      if (GAME_IsMissionAccomplied(iCurrentPlayer, iDst))
        {
          MsgEndOfMission msg;

          msg.iWinner = iCurrentPlayer;
          msg.iTyp  = RISK_GetMissionTypeOfPlayer(iCurrentPlayer);
          msg.iNum1 = RISK_GetMissionContinent1OfPlayer(iCurrentPlayer);
          msg.iNum2 = RISK_GetMissionContinent2OfPlayer(iCurrentPlayer);
          RISK_SetMissionTypeOfPlayer(iCurrentPlayer, NO_MISSION);
          (void)RISK_SendSyncMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			             MSG_ENDOFMISSION, &msg,
			             MSG_ENDOFMISSION, CBK_IncomingMessage);
          /* If the winner stop the game ... */
          if (iState == STATE_REGISTER)
              return;
        }

      /* Move the desired number of armies if it isn't end of game */
      if (RISK_GetNumCountriesOfPlayer(iDst) != 0 ||
	  RISK_GetNumLivePlayers() != 2)
	{
	  iArmiesMove = UTIL_GetArmyNumber(iAttackDie, 
				     RISK_GetNumArmiesOfCountry(iSrcCountry)-1,
				     FALSE);
	  RISK_SetNumArmiesOfCountry(iSrcCountry, 
				     RISK_GetNumArmiesOfCountry(iSrcCountry) - 
				     iArmiesMove);
	  RISK_SetNumArmiesOfCountry(iDstCountry, 
				     RISK_GetNumArmiesOfCountry(iDstCountry) + 
				     iArmiesMove);
	}

      /* See if this victory signalled a _DEADPLAYER or _ENDOFGAME condition */
      if (RISK_GetNumCountriesOfPlayer(iDst) == 0)
	GAME_PlayerDied(iDst);
    }
  else
    {
      /* If a new attack and not do-or-die, notify the server */
      if (!(iCachedSrcCountry == iSrcCountry && 
	    iCachedDstCountry == iDstCountry))
	{
	  /* Send a verbose message */
#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "%s is attacking from %s to %s",
		  RISK_GetNameOfPlayer(iCurrentPlayer),
		  RISK_GetNameOfCountry(iSrcCountry),
		  RISK_GetNameOfCountry(iDstCountry));
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "%s attaque %s à partir de %s",
		  RISK_GetNameOfPlayer(iCurrentPlayer),
		  RISK_GetNameOfCountry(iDstCountry),
		  RISK_GetNameOfCountry(iSrcCountry));
#endif
	  mess.strMessage = buf;
	  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
				 MSG_NETMESSAGE, &mess);
	  UTIL_DisplayComment(buf);
	}
    }
     
  /* If we are told to notify, then notify */
  if (fNotify == TRUE)
    {
      msgAttackNotify.iSrcCountry = iSrcCountry;
      msgAttackNotify.iDstCountry = iDstCountry;
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_ATTACKNOTIFY, &msgAttackNotify);
    }

  /* Update cached values */
  iCachedSrcCountry = iSrcCountry;
  iCachedDstCountry = iDstCountry;
}


/************************************************************************ 
 *  FUNCTION: GAME_IsEnemyAdjacent
 *  HISTORY: 
 *     03.05.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_IsEnemyAdjacent(Int32 iCountry)
{
  Int32 i, iPlayer;

  iPlayer = RISK_GetOwnerOfCountry(iCountry);
  for (i=0; i!=6 && RISK_GetAdjCountryOfCountry(iCountry, i)!=-1; i++)
    if (RISK_GetOwnerOfCountry(RISK_GetAdjCountryOfCountry(iCountry, i)) != 
	iPlayer)
      return TRUE;

  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_IsFrontierAdjacent
 *  HISTORY: 
 *     03.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_IsFrontierAdjacent(Int32 srcCountry, Int32 destCountry)
{
  Int32 iPlayer, iContinent, iCountry, i;

  iPlayer = RISK_GetOwnerOfCountry(srcCountry);
  iContinent = RISK_GetContinentOfCountry(srcCountry);
  if (RISK_GetContinentOfCountry(destCountry) != iContinent)
      return GAME_IsEnemyAdjacent(destCountry);
  for (i=0; i!=6 && RISK_GetAdjCountryOfCountry(destCountry, i)!=-1; i++)
    {
      iCountry = RISK_GetAdjCountryOfCountry(destCountry, i);
      if (RISK_GetContinentOfCountry(iCountry) != iContinent)
          return TRUE;
    }

  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: FindEnemyAdjacent
 *  HISTORY: 
 *     11.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 FindEnemyAdjacent(Int32 iCountry, Int32 distance)
{
  Int32 i, min, res, iPlayer, dest;

  iPlayer = RISK_GetOwnerOfCountry(iCountry);
  min = 100000;
  for (i=0; i!=6 && RISK_GetAdjCountryOfCountry(iCountry, i)!=-1; i++)
    {
      dest = RISK_GetAdjCountryOfCountry(iCountry, i);
      if (RISK_GetOwnerOfCountry(dest) == iPlayer)
        {
          if (distance <= 3)
            {
              res = FindEnemyAdjacent(dest, distance + 1);
              if (res < min)
                  min = res;
            }
        }
      else
          min = 0;
    }

  return (min + 1);
}


/************************************************************************ 
 *  FUNCTION: GAME_FindEnemyAdjacent
 *  HISTORY: 
 *     11.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 GAME_FindEnemyAdjacent(Int32 iCountry)
{
  Int32 i, min, res, iPlayer, dest, destCountry;

  iPlayer = RISK_GetOwnerOfCountry(iCountry);
  destCountry = -1;
  min = 100000;
  for (i=0; i!=6 && RISK_GetAdjCountryOfCountry(iCountry, i)!=-1; i++)
    {
      dest = RISK_GetAdjCountryOfCountry(iCountry, i);
      if (RISK_GetOwnerOfCountry(dest) == iPlayer)
        {
          res = FindEnemyAdjacent(dest, 0);
          if (res < min)
            {
              min = res;
              destCountry = dest;
            }
        }
      else
          min = 0;
    }

  return (destCountry);
}


/************************************************************************ 
 *  FUNCTION: GAME_AttackClick
 *  HISTORY: 
 *     03.06.94  ESF  Created.
 *     04.02.94  ESF  Added notification of server after Do-or-Die.
 *     05.19.94  ESF  Added verbose message across network.
 *     05.19.94  ESF  Fixed for fixed attack dice do-or-die.
 *     07.14.94  ESF  Fixed bug, bot checking attack dice on dest country.
 *     10.15.94  ESF  Added printing of "Attacking from foo to bar" locally.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_AttackClick(Int32 iCountry)
{
  char buf[256];
  if (iAttackSrc < 0)
    {
      if (GAME_ValidAttackSrc(iCountry, TRUE) &&
	  GAME_ValidAttackDice(RISK_GetDiceModeOfPlayer(iCurrentPlayer),
			       iCountry))
	{
	  iAttackSrc = iCountry;

#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "%s is attacking from %s to" 
		  " <click on territory>",
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "%s attaque à partir de %s ",
#endif
		  RISK_GetNameOfPlayer(iCurrentPlayer),
		  RISK_GetNameOfCountry(iAttackSrc));
	  UTIL_DisplayComment(buf);
	  UTIL_LightCountry(iAttackSrc);
	}
    }
  else if (GAME_ValidAttackDst(iAttackSrc, iCountry, TRUE) &&
	   GAME_ValidAttackDice(RISK_GetDiceModeOfPlayer(iCurrentPlayer),
				iAttackSrc))
    {
      iAttackDst = iCountry;
      GAME_Attack(iAttackSrc, iAttackDst);

      iAttackSrc = iAttackDst = -1;
      UTIL_DisplayActionCString(iState, iCurrentPlayer);
    }
}


/************************************************************************ 
 *  FUNCTION: GAME_ValidAttackDice
 *  HISTORY: 
 *     05.19.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_ValidAttackDice(Int32 iAttackDice, Int32 iCountry)
{
  char buf[256];
  /* If the dice have been selected manually, check them */
  if (iAttackDice != ATTACK_AUTO)
    {
      if (RISK_GetNumArmiesOfCountry(iCountry) <= iAttackDice+1)
	{
#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "You can't attack with %d dice, dummy.",
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "Attaque impossible avec %d dé(s).",
#endif
		  iAttackDice+1);
	  UTIL_DarkenCountry(iCountry);
	  UTIL_DisplayError(buf);
	  return FALSE;
	}
    }
  return TRUE;
}


/************************************************************************ 
 *  FUNCTION: GAME_ValidAttackSrc
 *  HISTORY: 
 *     03.06.94  ESF  Created.
 *     03.07.94  ESF  Added verbose option
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_ValidAttackSrc(Int32 iAttackSrc, Flag fVerbose)
{
  char buf[256];
  if (iAttackSrc < 0)
    {
      if (fVerbose)
#ifdef ENGLISH
	UTIL_DisplayError("Where the hell are you trying to attack from?");
#endif
#ifdef FRENCH
	UTIL_DisplayError("D'où voulez-vous attaquer?");
#endif
    }
  else if (iAttackSrc >= NUM_COUNTRIES)
    {
      if (fVerbose)
#ifdef ENGLISH
	UTIL_DisplayError("You can't attack from the ocean!");
#endif
#ifdef FRENCH
	UTIL_DisplayError("Attaque impossible à partir de l'océan!");
#endif
    }
  else if (RISK_GetOwnerOfCountry(iAttackSrc) != iCurrentPlayer)
    {
      if (fVerbose)
	{
#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "You can't attack from %s -- you don't own it.",
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "Attaque impossible à partir de %s.",
#endif
		  RISK_GetNameOfCountry(iAttackSrc));
	  UTIL_DisplayError(buf);
	}
    }
  else if (!GAME_IsEnemyAdjacent(iAttackSrc))
    {
      if (fVerbose)
	{
#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "There are no enemy territories you can attack from %s.", 
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "Pas de territoire à attaquer à partir de %s.",
#endif
		  RISK_GetNameOfCountry(iAttackSrc));
	  UTIL_DisplayError(buf);
	}
    }
  else if (RISK_GetNumArmiesOfCountry(iAttackSrc) == 1)
    {
      if (fVerbose)
#ifdef ENGLISH
	UTIL_DisplayError("You can't attack with 1 army.");
#endif
#ifdef FRENCH
	UTIL_DisplayError("Attaque impossible avec une seule armée.");
#endif
    }
  else
    return TRUE;

  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_ValidAttackDst
 *  HISTORY: 
 *     03.06.94  ESF  Created.
 *     03.07.94  ESF  Added verbose option
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_ValidAttackDst(Int32 iAttackSrc, Int32 iAttackDst, Flag fVerbose)
{
  char buf[256];
  if (iAttackDst < 0)
    {
      if (fVerbose)
#ifdef ENGLISH
	UTIL_DisplayError("Where the hell are you trying to attack from?");
#endif
#ifdef FRENCH
	UTIL_DisplayError("Où voulez-vous attaquer?");
#endif
    }
  else if (iAttackDst >= NUM_COUNTRIES)
    {
      if (fVerbose)
#ifdef ENGLISH
	UTIL_DisplayError("You can't attack the ocean!");
#endif
#ifdef FRENCH
	UTIL_DisplayError("Impossible d'attaquer l'océan!");
#endif
    }
  else 
    {
      if (!GAME_CanAttack(iAttackSrc, iAttackDst))
	{
	  if (fVerbose)
	    {
#ifdef ENGLISH
	      snprintf(buf, sizeof(buf), "You can't attack %s from %s!",
#endif
#ifdef FRENCH
	      snprintf(buf, sizeof(buf), "Attaque impossible de %s à partir de %s!",
#endif
		      RISK_GetNameOfCountry(iAttackDst),
		      RISK_GetNameOfCountry(iAttackSrc));
	      UTIL_DisplayError(buf);
	    }
	}
      else if (RISK_GetOwnerOfCountry(iAttackDst) == iCurrentPlayer)
	{
	  if (fVerbose)
	    {
#ifdef ENGLISH
	      snprintf(buf, sizeof(buf), "You can't attack %s -- you own it.",
#endif
#ifdef FRENCH
	      snprintf(buf, sizeof(buf), "Attaque impossible de son propre territoire (%s).",
#endif
		      RISK_GetNameOfCountry(iAttackDst));
	      UTIL_DisplayError(buf);
	    }
	}
      else
	return TRUE;
    }
  
  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_PlaceClick
 *  HISTORY: 
 *     03.07.94  ESF  Created.
 *     03.16.94  ESF  Added support for placing multiple armies.
 *     05.19.94  ESF  Factored out code to GAME_PlaceArmies.
 *     04.30.95  ESF  Brought the dialog code from PlaceArmies here.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_PlaceClick(Int32 iCountry, Int32 iPlaceType)
{
    Int32 maxN;
  /* Is the country picked the player's? */
  if (GAME_ValidPlaceDst(iCountry))  {
      Int32 iArmies;

      maxN = RISK_GetNumArmiesOfPlayer(iCurrentPlayer);
#ifndef DROPALL
      if ( iState == STATE_FORTIFY )
	maxN = (maxN > DROP_ARMIES) ? DROP_ARMIES : maxN;
#endif

      /* Depending on the type of placement, popup a dialog or not */
      if (iPlaceType == PLACE_MULTIPLE)
      {
	  iArmies = UTIL_GetArmyNumber(1, maxN, TRUE);
	  if (!iArmies)
	    return;
	}
      else if (iPlaceType == PLACE_ALL)
	iArmies = maxN;
      else
	iArmies = 1;

      GAME_PlaceArmies(iCountry, iArmies);

      /* If we're fortifying, we're done. */
      if (iState == STATE_FORTIFY)
	GAME_EndTurn();
    }
}


/************************************************************************ 
 *  FUNCTION: GAME_ValidPlaceDst
 *  HISTORY: 
 *     03.07.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_ValidPlaceDst(Int32 iPlaceDst)
{
  if (iPlaceDst < 0)
#ifdef ENGLISH
    UTIL_DisplayError("What the hell country is that!");
#endif
#ifdef FRENCH
    UTIL_DisplayError("What the hell country is that!");
#endif
  else if (iPlaceDst >= NUM_COUNTRIES)
#ifdef ENGLISH
    UTIL_DisplayError("Ahhh...That's the ocean, buddy...");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Ahhh...C'est l'océan, plouf...");
#endif
  else if (RISK_GetOwnerOfCountry(iPlaceDst) != iCurrentPlayer)
#ifdef ENGLISH
    UTIL_DisplayError("You cannot place armies on opponent's territories.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Donner des armées à un autre joueur?");
#endif
  else
    return TRUE;

  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_MoveClick
 *  HISTORY: 
 *     03.07.94  ESF  Created.
 *     03.29.94  ESF  Added a message when iMoveDst is verified.
 *     03.29.94  ESF  Fixed lack of UTIL_DisplayComment() bug.
 *     01.09.95  JC   End turn only if iState asn't moved to STATE_REGISTER.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_MoveClick(Int32 iCountry)
{
  Int32 iNumArmies;
  char buf[256];
  
  if (iMoveSrc < 0)
    {
      if (GAME_ValidMoveSrc(iCountry))
	{
	  iMoveSrc = iCountry;

#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "%s moving armies from %s to" 
		  " <click on territory>", 
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "%s déplace des armées de %s en" 
		  " <click on territory>",
#endif
		  RISK_GetNameOfPlayer(iCurrentPlayer),
		  RISK_GetNameOfCountry(iMoveSrc));
	  UTIL_DisplayComment(buf);
	  UTIL_LightCountry(iMoveSrc);
	}
    }
  else if (GAME_ValidMoveDst(iMoveSrc, iCountry))
    {
      iMoveDst = iCountry;

#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s moving armies from %s to %s.", 
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s déplace des armées de %s en %s.", 
#endif
	      RISK_GetNameOfPlayer(iCurrentPlayer),
	      RISK_GetNameOfCountry(iMoveSrc),
	      RISK_GetNameOfCountry(iMoveDst));
      UTIL_DisplayComment(buf);

      iNumArmies = UTIL_GetArmyNumber(1,
				      RISK_GetNumArmiesOfCountry(iMoveSrc)-1,
				      TRUE);
      if (iNumArmies>0)
	{
	  GAME_MoveArmies(iMoveSrc, iMoveDst, iNumArmies);

          if (iState != STATE_REGISTER)
	      /* Turn over, man! */
	      GAME_EndTurn();
	}
      else
	{
	  UTIL_DarkenCountry(iMoveSrc);
	  iMoveSrc = iMoveDst = -1;
	}
    }
}


/************************************************************************ 
 *  FUNCTION: GAME_ValidMoveSrc
 *  HISTORY: 
 *     03.07.94  ESF  Created.
 *     03.29.94  ESF  fixed a small bug, inserted "else".
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_ValidMoveSrc(Int32 iMoveSrc)
{
  if (iMoveSrc < 0)
#ifdef ENGLISH
    UTIL_DisplayError("Where the hell are you trying to move from?");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Where the hell are you trying to move from?");
#endif
  else if (iMoveSrc >= NUM_COUNTRIES)
#ifdef ENGLISH
    UTIL_DisplayError("You can't move armies from the ocean.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Impossible de déplacer des armées à partir de l'océan.");
#endif
  else if (RISK_GetOwnerOfCountry(iMoveSrc) != iCurrentPlayer)
#ifdef ENGLISH
    UTIL_DisplayError("You cannot move armies from an opponent's"
			" territories.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Impossible de prendre les armées d'un autre joueur.");
#endif
  else if (RISK_GetNumArmiesOfCountry(iMoveSrc) == 1)
#ifdef ENGLISH
    UTIL_DisplayError("You cannot move from territories that"
		      " have one army to begin with.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Impossible de déplacer la seule armée d'un territoire.");
#endif
  else
    return TRUE;

  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_ValidMoveDst
 *  HISTORY: 
 *     03.07.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES:  used to get iMoveDst passed, which is a global static in game.c
 ************************************************************************/
Flag GAME_ValidMoveDst(Int32 iMoveSrc, Int32 iMoveDst)
{
  char buf[256];
  if (iMoveDst < 0)
#ifdef ENGLISH
    UTIL_DisplayError("Where the hell are you trying to move from?");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Where the hell are you trying to move from?");
#endif
  else if (iMoveDst >= NUM_COUNTRIES)
#ifdef ENGLISH
    UTIL_DisplayError("You can't move armies into the ocean -- they'd drown.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Impossible de placer des armées dans l'océan.");
#endif
  else if (RISK_GetOwnerOfCountry(iMoveDst) != iCurrentPlayer)
#ifdef ENGLISH
    UTIL_DisplayError("You cannot move armies to an opponent's"
		      " territories.");
#endif
#ifdef FRENCH
    UTIL_DisplayError("Impossible de placer des armées sur un territoire ennemi.");
#endif
  else if (!GAME_CanAttack(iMoveSrc, iMoveDst))
    {
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "Armies can't get to %s from %s",
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "Déplacement impossible de %s en %s",
#endif
	      RISK_GetNameOfCountry(iMoveDst),
	      RISK_GetNameOfCountry(iMoveSrc));
      UTIL_DisplayError(buf);
    }
  else
    return TRUE;

  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_GetContinentBonus
 *  HISTORY: 
 *     03.16.94  ESF  Created.
 *     01.09.95  ESF  Changed an if..else to an assertion.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 GAME_GetContinentBonus(Int32 iPlayer)
{
  Int32  piCount[NUM_CONTINENTS];
  Int32  i, iArmies=0;

  /* Init. */
  for (i=0; i!=NUM_CONTINENTS; i++)
    piCount[i] = 0;

  /* Count up how many countries the player has in each of the continents */
  for (i=0; i!=NUM_COUNTRIES; i++)
    if (RISK_GetOwnerOfCountry(i) == iPlayer)
      {
	/* Sanity check */
	D_Assert(RISK_GetContinentOfCountry(i) >= 0 &&
		 RISK_GetContinentOfCountry(i) < NUM_CONTINENTS,
		 "Country has messed up number of armies.");
	
	piCount[RISK_GetContinentOfCountry(i)]++;
      }

  /* If the player has the total number of countries, give him or her bonus */
  for (i=0; i!=NUM_CONTINENTS; i++)
    if (RISK_GetNumCountriesOfContinent(i) == piCount[i])
      iArmies += RISK_GetValueOfContinent(i);

  return(iArmies);
}


/************************************************************************ 
 *  FUNCTION: GAME_PlayerDied
 *  HISTORY: 
 *     03.28.94  ESF  Created.
 *     03.29.94  ESF  Fixed off-by-one bug in end of game detection.
 *     04.11.94  ESF  Added iKillerPlayer parameter.
 *     05.04.94  ESF  Fixed bugs and exhanced.
 *     05.12.94  ESF  Fixed bug, reset dead player's cards to 0.
 *     09.31.94  ESF  Changed because of new ENDOFGAME contents.
 *     10.02.94  ESF  Changed to set number of live players == 0.
 *     10.30.94  ESF  Fixed bug, shouldn't be setting iNumLivePlayers to 0.
 *     01.09.95  ESF  Changed to be more verbose.
 *     30.08.95  JC   Talk the server: "force exchange of cards?".
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_PlayerDied(Int32 iDeadPlayer)
{
  MsgMessagePacket msg;
  char buf[256];

  /* Notify everybody */
#ifdef ENGLISH
  snprintf(buf, sizeof(buf), "%s was destroyed by %s (who gained %d card%s).",
#endif
#ifdef FRENCH
  snprintf(buf, sizeof(buf), "%s est détruit par %s (qui gagne %d carte%s).",
#endif
	  RISK_GetNameOfPlayer(iDeadPlayer),
	  RISK_GetNameOfPlayer(iCurrentPlayer),
	  RISK_GetNumCardsOfPlayer(iDeadPlayer),
	  (RISK_GetNumCardsOfPlayer(iDeadPlayer)>1)?"s":"");
  msg.strMessage = buf;
  msg.iFrom = FROM_SERVER;
  msg.iTo = DST_ALLPLAYERS;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			 MSG_MESSAGEPACKET, &msg);

  /* Kill the player  */
  RISK_SetStateOfPlayer(iDeadPlayer, PLAYER_DEAD);

  /* Is there only one player left?  If so, end of game... */
  if (RISK_GetNumLivePlayers() == 1)
    {
      MsgVictory  msgVictory;
      
      msgVictory.iWinner = iCurrentPlayer; 
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_VICTORY, &msgVictory);
    }
  else
    {
      MsgForceExchangeCards msg;
      Int32 i, n, m;

      /* Give the killer player all of the cards */
      n = RISK_GetNumCardsOfPlayer(iCurrentPlayer); 
      m = RISK_GetNumCardsOfPlayer(iDeadPlayer); 
      
      /* Let the player know how many cards he or she got. */
      if (m>0)
	{
#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "%s got %d card%s from %s.", 
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "%s prend %d carte%s de %s.", 
#endif
		  RISK_GetNameOfPlayer(iCurrentPlayer),
		  m, m==1?"":"s", RISK_GetNameOfPlayer(iDeadPlayer));
	  mess.strMessage = buf;
	  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
				 MSG_NETMESSAGE, &mess);
	}

      for (i=0; i!=m; i++)
	RISK_SetCardOfPlayer(iCurrentPlayer, n+i, 
			     RISK_GetCardOfPlayer(iDeadPlayer, i));
      RISK_SetNumCardsOfPlayer(iCurrentPlayer, n+m);
      RISK_SetNumCardsOfPlayer(iDeadPlayer, 0);

      if (m>0)
	{
          /* Force the player to exchange if he or she possesses 
           * too many cards.
           */
          msg.iPlayer = iCurrentPlayer;
          (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
                                 MSG_FORCEEXCHANGECARDS, &msg);
	}
    }
}


/************************************************************************ 
 *  FUNCTION: GAME_MissionToStr
 *  HISTORY: 
 *     29.08.95  JC  Created (copy of GAME_ShowMission).
 *  PURPOSE: Put in strScratch a description of the mission
 *  NOTES: 
 ************************************************************************/
void GAME_MissionToStr(Int32 iPlayer, Int16 iTyp, Int32 iNum1, Int32 iNum2, Flag fVict, char *buf, size_t bufsize)
{
  UNUSED(iPlayer);

#ifdef FRENCH
  switch (iTyp)
    {
    case NO_MISSION:
      snprintf(buf, bufsize, "Plus de mission -> Conquerir le monde");
      break;
    case CONQUIER_WORLD:
      if (fVict)
          snprintf(buf, bufsize, "Mission accomplie: le monde est conquit");
      else
          snprintf(buf, bufsize, "Mission: Conquerir le monde");
      break;
    case CONQUIER_Nb_COUNTRY:
      if (fVict)
          snprintf(buf, bufsize, "Mission accomplie: %d territoires conquis",
                              iNum1);
      else
          snprintf(buf, bufsize, "Mission: Conquerir %d territoires",
                              iNum1);
      break;
    case CONQUIER_TWO_CONTINENTS:
      if (fVict)
          snprintf(buf, bufsize, "Mission accomplie: Conquerir %s et %s",
              RISK_GetNameOfContinent(iNum1),
              RISK_GetNameOfContinent(iNum2));
      else
          snprintf(buf, bufsize, "Mission: Conquerir %s et %s",
              RISK_GetNameOfContinent(iNum1),
              RISK_GetNameOfContinent(iNum2));
      break;
    case KILL_A_PLAYER:
      if (fVict || iNum2)
          snprintf(buf, bufsize, "Mission accomplie: %s est mort",
              RISK_GetNameOfPlayer(iNum1));
      else if (RISK_GetNumCountriesOfPlayer(iNum1) <= 0)
          snprintf(buf, bufsize, "Mission impossible: Tuer %s qui est déjà mort",
              RISK_GetNameOfPlayer(iNum1));
      else
          snprintf(buf, bufsize, "Mission: Tuer %s",
              RISK_GetNameOfPlayer(iNum1));
      break;
    }
#endif
#ifdef ENGLISH
  switch (iTyp)
    {
    case NO_MISSION:
      snprintf(buf, bufsize, "Additional mission -> Conquer the world");
      break;
    case CONQUIER_WORLD:
      if (fVict)
          snprintf(buf, bufsize, "Mission accomplished: the world is conquered");
      else
          snprintf(buf, bufsize, "Mission: Conquer the world");
      break;
    case CONQUIER_Nb_COUNTRY:
      if (fVict)
          snprintf(buf, bufsize, "Mission accomplished: %d countries conquered",
                              iNum1);
      else
          snprintf(buf, bufsize, "Mission: Conquer %d countries",
                              iNum1);
      break;
    case CONQUIER_TWO_CONTINENTS:
      if (fVict)
          snprintf(buf, bufsize, "Mission accomplished: Conquer %s and %s",
              RISK_GetNameOfContinent(iNum1),
              RISK_GetNameOfContinent(iNum2));
      else
          snprintf(buf, bufsize, "Mission: Conquer %s and %s",
              RISK_GetNameOfContinent(iNum1),
              RISK_GetNameOfContinent(iNum2));
      break;
    case KILL_A_PLAYER:
      if (fVict || iNum2)
          snprintf(buf, bufsize, "Mission accomplished: %s is dead",
              RISK_GetNameOfPlayer(iNum1));
      else if (RISK_GetNumCountriesOfPlayer(iNum1) <= 0)
          snprintf(buf, bufsize, "Mission impossible: kill %s who's already dead",
              RISK_GetNameOfPlayer(iNum1));
      else
          snprintf(buf, bufsize, "Mission: Kill %s",
              RISK_GetNameOfPlayer(iNum1));
      break;
    }
#endif
}


/************************************************************************ 
 *  FUNCTION: GAME_MissionAccomplied
 *  HISTORY: 
 *     25.08.95  JC   Created from different pieces.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_MissionAccomplied(Int32 iWinner, Int16 iTyp, Int32 iNum1, Int32 iNum2)
{
  Char  str[256];
  Int32 iChoice;
  char buf[256];

  GAME_MissionToStr(iWinner, iTyp, iNum1, iNum2, TRUE, buf, sizeof(buf));
  if (RISK_GetClientOfPlayer(iWinner) != CLNT_GetThisClientID())
    {
      /* Player that has won is non-local */
#ifdef ENGLISH
      snprintf(str, sizeof(str), 
	       "%s has accomplied his mission!\n%s\n",
	       RISK_GetNameOfPlayer(iCurrentPlayer),
	       buf);
      (void)UTIL_PopupDialog("End of Mission", str, 2, "Cool!", 
                             "So What?", NULL);
#endif
#ifdef FRENCH
      snprintf(str, sizeof(str), 
	       "%s a terminé sa mission!\n%s\n",
	       RISK_GetNameOfPlayer(iCurrentPlayer),
	       buf);
      (void)UTIL_PopupDialog("Mission terminée", str, 2, "Cool!", 
                             "Et alors?", NULL);
#endif
      return;
    }

  /* Winning player is local, congratulate him or her */
#ifdef ENGLISH
  (void)UTIL_PopupDialog(RISK_GetNameOfPlayer(iCurrentPlayer), 
                         "You WON!!!", 2, "Cool!", 
                         "So What?", NULL);
#endif
#ifdef FRENCH
  (void)UTIL_PopupDialog(RISK_GetNameOfPlayer(iCurrentPlayer), 
                         "Victoire!!!", 2, "Cool!", 
                         "Et alors?", NULL);
#endif

  /* See if the player wants to continue or play again */
#ifdef ENGLISH
  if (RISK_GetNumLivePlayers () > 1)
      iChoice = UTIL_PopupDialog("Game over", "Play again?",
                                 3, "Yes", "No", "Continue");
  else
      iChoice = UTIL_PopupDialog("Game over", "Play again?",
                                 2, "Yes", "No", NULL);
#endif
#ifdef FRENCH
  if (RISK_GetNumLivePlayers () > 1)
      iChoice = UTIL_PopupDialog("Partie terminée", "Jouer une autre partie?",
                                 3, "Oui", "Non", "Continuer");
  else
      iChoice = UTIL_PopupDialog("Partie terminée", "Jouer une autre partie?",
                                 2, "Oui", "Non", NULL);
#endif
  if (iChoice == QUERY_NO)
    {
      /* Leave the game */
      UTIL_ExitProgram(0);
    }
  if (iChoice == QUERY_CANCEL)
      return;

  iState = STATE_REGISTER;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_ENDOFGAME, NULL);
}


/************************************************************************ 
 *  FUNCTION: GAME_Victory
 *  HISTORY: 
 *     28.08.95  JC   Created from different pieces.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_Victory(Int32 iWinner)
{
  Int32 iChoice;
  char buf[256];

  if (RISK_GetClientOfPlayer(iWinner) != CLNT_GetThisClientID())
    {
      /* Player that has won is non-local */
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "%s has won!", RISK_GetNameOfPlayer(iCurrentPlayer));
      (void)UTIL_PopupDialog("End of Game", buf, 2, "Cool!", 
                             "So What?", NULL);
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "%s a gagné!", RISK_GetNameOfPlayer(iCurrentPlayer));
      (void)UTIL_PopupDialog("Fin du jeu", buf, 2, "Cool!", 
                             "Et alors?", NULL);
#endif
      return;
    }

  /* Winning player is local, congratulate him or her */
#ifdef ENGLISH
  (void)UTIL_PopupDialog(RISK_GetNameOfPlayer(iCurrentPlayer), 
                         "You WON!!!", 2, "Cool!", 
                         "So What?", NULL);
#endif
#ifdef FRENCH
  (void)UTIL_PopupDialog(RISK_GetNameOfPlayer(iCurrentPlayer), 
                         "Victoire!!!", 2, "Cool!", 
                         "Et alors?", NULL);
#endif

  /* See if the player wants to continue or play again */
#ifdef ENGLISH
  iChoice = UTIL_PopupDialog("Game over", "Play again?",
                             2, "Yes", "No", NULL);
#endif
#ifdef FRENCH
  iChoice = UTIL_PopupDialog("Partie terminée", "Jouer une autre partie?",
                             2, "Oui", "Non", NULL);
#endif
  if (iChoice == QUERY_NO)
    {
      /* Leave the game */
      UTIL_ExitProgram(0);
    }

  iState = STATE_REGISTER;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_ENDOFGAME, NULL);
}


/************************************************************************ 
 *  FUNCTION: GAME_GameOverMan
 *  HISTORY: 
 *     03.28.94  ESF  Created.
 *     05.10.94  ESF  Completed.
 *     05.12.94  ESF  Added more functionality.
 *     01.05.95  DFE  Fixed "Free Card" bug -- gave 1st person free card.
 *     02.13.95  ESF  Updated to reflect new registration interface.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_GameOverMan(void)
{
  /* There will be another game */
  fGameStarted = FALSE;
  
  /* Since EndTurn isn't called, need to reset this so that first player
   * doesn't get a free card next game.
   */
  fGetsCard    = FALSE;

  /* The state will be one of registering */
  if (iState != STATE_REGISTER)
    {
      iState = STATE_REGISTER;
      UTIL_ServerEnterState(iState);
    }

  UTIL_DisplayError("");
  UTIL_DisplayComment("");

  /* Set the colors to be of the initial setup */
  COLOR_SetWorldColors();

  /* Erase the dice roll */
  DICE_Hide();

  /* Get the new players */
  REG_PopupDialog();
}


/************************************************************************ 
 *  FUNCTION: GAME_ExchangeCards
 *  HISTORY: 
 *     03.29.94  ESF  Created.
 *     04.23.95  ESF  Fixed (terrible) card take-away bug.
 *     05.13.95  ESF  Fixed (horrible) country bonus bug -- thanks Alan!
 *     22.08.95  JC   Fixed a bug with computerized player
 *                    (piCards not ordered).
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_ExchangeCards(Int32 *piCards)
{
  Int32             i, j, k, piOldCards[3];
  MsgExchangeCards  msgCards;
  MsgNetMessage     msgNetMessage;
  char buf[256];

  /* only 3 cards because we're handling selected cards */
  /* Save a copy and transform array indices into card indices */
  for (i=0; i!=3; i++)
    {
      piOldCards[i] = piCards[i];
      piCards[i] = RISK_GetCardOfPlayer(iCurrentPlayer, piCards[i]);
    }
  
  /* Send the cards to the server -- this will generate an _REPLYPACKET */
  for (i=0; i!=3; i++)
    msgCards.piCards[i] = piCards[i];
  (void)RISK_SendSyncMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_EXCHANGECARDS, &msgCards,
			     MSG_REPLYPACKET, CBK_IncomingMessage);

  /* Update the date structures (bonus + countries on cards) */
  RISK_SetNumArmiesOfPlayer(iCurrentPlayer, 
			    RISK_GetNumArmiesOfPlayer(iCurrentPlayer)+iReply);

  /* See if any of the cards match countries that the player has */
  for (i=0; i!=3; i++)
    if (piCards[i] < NUM_COUNTRIES && /* Not a joker! */
	RISK_GetOwnerOfCountry(piCards[i]) == iCurrentPlayer)
      RISK_SetNumArmiesOfCountry(piCards[i], 
				 RISK_GetNumArmiesOfCountry(piCards[i])+2);

  /* Take the cards away from the player */
  for (i=0; i!=3; i++)
    {
      for (j=piOldCards[i]; j<MAX_CARDS-1; j++)
	RISK_SetCardOfPlayer(iCurrentPlayer, j,
			     RISK_GetCardOfPlayer(iCurrentPlayer, j+1));

      /* Now that we've moved all the cards down, we have to move the
       * pointers to the selected cards down as well.  This was a bug.
       */

      for (k=i+1; k<3; k++)
        if (piOldCards[k]>piOldCards[i])
	  piOldCards[k] = piOldCards[k]-1;
    }

  RISK_SetNumCardsOfPlayer(iCurrentPlayer, 
			   RISK_GetNumCardsOfPlayer(iCurrentPlayer)-3);

  /* Display a message */
#ifdef ENGLISH
  snprintf(buf, sizeof(buf), "%s got %d armies from exchanging cards.",
#endif
#ifdef FRENCH
  snprintf(buf, sizeof(buf), "%s gagne %d armées en échangeant des cartes.",
#endif
	  RISK_GetNameOfPlayer(iCurrentPlayer),
	  iReply);
  UTIL_DisplayError(buf);
  msgNetMessage.strMessage = buf;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), MSG_NETMESSAGE,
			 &msgNetMessage);
}


/************************************************************************ 
 *  FUNCTION: GAME_EndTurn
 *  HISTORY: 
 *     03.17.94  ESF  Created.
 *     03.28.94  ESF  Added card code, do we need Sync on the call?
 *     03.28.94  ESF  Moved iAttack/iDefend stuff here.
 *     03.29.94  ESF  Added iMove stuff.
 *     04.11.94  ESF  Added clear dice call.
 *     04.11.94  ESF  Move message call to end.
 *     02.20.95  ESF  Moved to game.c from utils.c
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_EndTurn(void)
{
  /* Get the player a card if he or she needs one */
  if (fGetsCard)
    {
      MsgRequestCard msg;
      
      fGetsCard = FALSE;
      msg.iPlayer = iCurrentPlayer;
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_REQUESTCARD, &msg);
    }

  /* Darken the country if the player was attacking or moving */
  if (iAttackSrc >= 0)
    UTIL_DarkenCountry(iAttackSrc);
  if (iMoveSrc >= 0)
    UTIL_DarkenCountry(iMoveSrc);    
  
  /* Clear the dice box */
  DICE_Hide();

  /* Init. variables for next turn */
  fCanExchange = TRUE;
  iAttackSrc = iAttackDst = iLastAttackSrc = iLastAttackDst = -1;
  iMoveSrc = iMoveDst = -1;

  /* Notify the server of the end-of-turn condition and wait for a response */
  (void)RISK_SendSyncMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			     MSG_ENDTURN, NULL,  
			     MSG_TURNNOTIFY, CBK_IncomingMessage);
}


/************************************************************************ 
 *  FUNCTION: GAME_IsMissionAccomplied
 *  HISTORY: 
 *     16.08.95  JC  Created.
 *     23.08.95  JC  added iKilledPlayer.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag GAME_IsMissionAccomplied(Int32 iPlayer, Int32 iKilledPlayer)
{
  Int32 piCount [NUM_CONTINENTS];
  Int32 i, cont, n1, n2;

  switch (RISK_GetMissionTypeOfPlayer(iPlayer))
    {
    case CONQUIER_WORLD:
      if (RISK_GetNumCountriesOfPlayer(iPlayer) >= NUM_COUNTRIES)
          return TRUE;
      break;
    case CONQUIER_Nb_COUNTRY:
      n1 = RISK_GetMissionNumberOfPlayer(iPlayer);
      if (RISK_GetNumCountriesOfPlayer(iPlayer) >= n1)
          return TRUE;
      break;
    case CONQUIER_TWO_CONTINENTS:
      for (cont=0; cont<NUM_CONTINENTS; cont++)
          piCount[cont] = 0;
      for (i=0; i<NUM_COUNTRIES; i++)
          if (RISK_GetOwnerOfCountry(i) == iPlayer)
              piCount[RISK_GetContinentOfCountry(i)]++;
      n1 = RISK_GetMissionContinent1OfPlayer(iPlayer);
      n2 = RISK_GetMissionContinent2OfPlayer(iPlayer);
      if (    (piCount[n1] == RISK_GetNumCountriesOfContinent(n1))
           && (piCount[n2] == RISK_GetNumCountriesOfContinent(n2)))
          return TRUE;
      break;
    case KILL_A_PLAYER:
      n1 = RISK_GetMissionPlayerToKillOfPlayer(iPlayer);
      if (    (RISK_GetNumCountriesOfPlayer(n1) <= 0)
           && (n1 == iKilledPlayer))
        {
          RISK_SetMissionPlayerIsKilledOfPlayer(iPlayer, TRUE);
          return TRUE;
        }
      else if (RISK_GetMissionIsPlayerKilledOfPlayer(iPlayer))
          return TRUE;
      break;
    }
  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: GAME_ShowMission
 *  HISTORY: 
 *     16.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GAME_ShowMission(Int32 iPlayer)
{
  char buf[256];
  GAME_MissionToStr(iPlayer, RISK_GetMissionTypeOfPlayer(iPlayer),
                             RISK_GetMissionContinent1OfPlayer(iPlayer),
                             RISK_GetMissionContinent2OfPlayer(iPlayer),
                             GAME_IsMissionAccomplied(iPlayer, -1),
		    buf, sizeof(buf));
  UTIL_DisplayError(buf);
}
