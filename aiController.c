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
 *   $Id: aiController.c,v 1.4 1999/11/13 21:58:30 morphy Exp $
 */

#include "aiController.h"
#include "aiClient.h"
#include "client.h"
#include "riskgame.h"
#include "game.h"
#include "debug.h"

/* Prototypes */
Int32  CLNT_GetCommLinkOfClient(Int32 iThisClient);

/* Externs */
extern void  *(*__AI_Callback)(void *, Int32, void *);
extern void  *pvContext[MAX_PLAYERS];
extern Int32  iState, iCurrentPlayer;
extern Flag   fGameStarted;

/* Globals */
Flag          fPlayerFortified;


/************************************************************************ 
 *  FUNCTION: AI_Init
 *  HISTORY: 
 *     04.23.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_Init(void)
{
  fPlayerFortified=FALSE;
  fGameStarted = FALSE;
  fGetsCard=FALSE;
}


/************************************************************************ 
 *  FUNCTION: AI_CheckCards
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *     28.08.95  JC   If the computerized must exchange cards then do.
                      (copy of ExchangeCards)
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_CheckCards(void)
{
  Int32 i, j, nb, typ, piCards[4], nbCards[4], piCardValues[MAX_CARDS];
  Flag  fOptimal;

  if (RISK_GetNumCardsOfPlayer (iCurrentPlayer) <= 4)
    return;
#ifdef ENGLISH
  printf("AI: Error -- Checking cards.\n");
#endif
#ifdef FRENCH
  printf("IA: Erreur -- Vérification des cartes.\n");
#endif

  nb = RISK_GetNumCardsOfPlayer(iCurrentPlayer);
  do
    {
      piCards[0]=piCards[1]=piCards[2]=piCards[3]=-1;
      nbCards[0]=nbCards[1]=nbCards[2]=nbCards[3]=0;
      fOptimal = FALSE;
      for (i=0; i<nb; i++)
        {
          piCardValues[i] = RISK_GetCardOfPlayer(iCurrentPlayer, i);
          /* Set the type of the card */
          if (piCardValues[i]<NUM_COUNTRIES)
            {
              typ = piCardValues[i] % 3;
              nbCards[typ]++;
              if (RISK_GetOwnerOfCountry(piCardValues[i]) == iCurrentPlayer)
                  piCards[typ] = i;
              else if (piCards[typ] == -1)
                  piCards[typ] = i;
            }
          else  /* Joker */
            {
	      piCards[3] = i;
	      nbCards[3]++;
	    }
        }
      if ((nbCards[0]>0)&&(nbCards[1]>0)&&(nbCards[2]>0))
        {
          AI_ExchangeCards(piCards);
          fOptimal = TRUE;
        }
      else if (nb >= 5)
        {
          if ((nbCards[0]>0)&&(nbCards[1]>0)&&(nbCards[3]>0))
              piCards[2] = piCards[3];
          else if ((nbCards[0]>0)&&(nbCards[3]>0)&&(nbCards[2]>0))
              piCards[1] = piCards[3];
          else if ((nbCards[3]>0)&&(nbCards[1]>0)&&(nbCards[2]>0))
              piCards[0] = piCards[3];
          else if (nbCards[0]>=3)
            {
              j = 0;
              for (i=0; i<nb; i++)
                  if ((piCardValues[i] % 3) == 0)
                      piCards[j++]=i;
            }
          else if (nbCards[1]>=3)
            {
              j = 0;
              for (i=0; i<nb; i++)
                  if ((piCardValues[i] % 3) == 1)
                      piCards[j++]=i;
            }
          else if (nbCards[2]>=3)
            {
              j = 0;
              for (i=0; i<nb; i++)
                  if ((piCardValues[i] % 3) == 2)
                      piCards[j++]=i;
            }
          else
            {
              piCards[0] = 0;
              piCards[1] = 1;
              piCards[2] = 2;
            }
          AI_ExchangeCards(piCards);
        }
      nb = RISK_GetNumCardsOfPlayer(iCurrentPlayer);
    }
  while ((fOptimal && (nb >= 3)) || (nb >= 5));
}


/************************************************************************ 
 *  FUNCTION: AI_CheckFortification
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_CheckFortification(void)
{
  /* If the player hasn't fortified yet, do it */
  if (!fPlayerFortified)
    {
      Int32 i;

#ifdef ENGLISH
      printf("AI: Error -- Checking fortification.\n");  
#endif
#ifdef FRENCH
      printf("IA: Erreur -- Vérification de la fortification.\n");  
#endif

      /* Place an army in the first country we find */
      for (i=0; i!=NUM_COUNTRIES; i++)
	if (RISK_GetOwnerOfCountry(i) == iCurrentPlayer)
	  {
	    AI_Place(i, 1);
	    return;
	  }

      D_Assert(FALSE, "Player didn't own any countries??");
    }
}


/************************************************************************ 
 *  FUNCTION: AI_CheckPlacement
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_CheckPlacement(void)
{
  if (RISK_GetNumArmiesOfPlayer(iCurrentPlayer) != 0)
    {
      Int32 i;

#ifdef ENGLISH
      printf("AI: Error -- Checking placement.\n");  
#endif
#ifdef FRENCH
      printf("IA: Erreur -- Vérification du placement.\n");  
#endif
      /* Dump the armies on the first country */
      for (i=0; i!=NUM_COUNTRIES; i++)
	if (RISK_GetOwnerOfCountry(i) == iCurrentPlayer)
	  {
	    AI_Place(i, RISK_GetNumArmiesOfPlayer(iCurrentPlayer));
	    return;
	  }

      D_Assert(FALSE, "Player didn't own any countries??");
    }
}


/************************************************************************ 
 *  FUNCTION: AI_Place
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *     21.08.95  JC   AI_CheckCards if game is started.
 *     28.08.95  JC   No AI_CheckCards.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag AI_Place(Int32 iCountry, Int32 iNumArmies)
{
  if (GAME_ValidPlaceDst(iCountry) && 
      iNumArmies <= RISK_GetNumArmiesOfPlayer(iCurrentPlayer) &&
      ((iState == STATE_FORTIFY && iNumArmies == 1) || 
       iState != STATE_FORTIFY))
    {
      GAME_PlaceArmies(iCountry, iNumArmies);
      
      /* Player has fortified if in fortification stage */
      fPlayerFortified = TRUE;

      return TRUE;
    }
  else
    {
#ifdef ENGLISH
      printf("AI: Error -- illegal place (%d, %d)\n", iCountry, iNumArmies);
#endif
#ifdef FRENCH
      printf("IA: Erreur -- placement illégal (%d, %d)\n", iCountry, iNumArmies);
#endif
      return FALSE;
    }
}


/************************************************************************/

Int32 iMoveMode;


/************************************************************************ 
 *  FUNCTION: AI_Attack
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *     23.08.95  JC   Corrected a bug if iSrcCountry or iDstCountry are 
 *                    totally invalid.
 *                    If ARMIES_MOVE_MANUAL, conserve NumArmiesOfPlayer.
 *     30.08.95  JC   Do the move immediatly. 
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag AI_Attack(Int32 iSrcCountry, Int32 iDstCountry, Int32 iAttack, 
	       Int32 iDice, Int32 iMoveSelectMode)
{
  Int32 iAttackMode, iDiceMode;
  char  *srcName, *dstName;

  iMoveMode = iMoveSelectMode;
  /* Set the modes for the attack */
  switch (iDice)
    {
    case DICE_ONE:
      iDiceMode   = ATTACK_ONE;
      break;
    case DICE_TWO:
      iDiceMode   = ATTACK_TWO;
      break;
    case DICE_THREE:
      iDiceMode   = ATTACK_THREE;
      break;
    case DICE_MAXIMUM:
      iDiceMode   = ATTACK_AUTO;
      break;
    default:
#ifdef ENGLISH
      printf("AI: Error -- choosing dice mode for attack.\n");
#endif
#ifdef FRENCH
      printf("IA: Erreur -- choix du mode des dés pour combattre.\n");
#endif
      return FALSE;
    }

  switch (iAttack)
    {
    case ATTACK_ONCE:
      iAttackMode = ACTION_ATTACK;
      break;
    case ATTACK_DOORDIE:
      iAttackMode = ACTION_DOORDIE;
      break;
    default:
#ifdef ENGLISH
      printf("AI: Error -- choosing attack mode for attack.\n");
#endif
#ifdef FRENCH
      printf("IA: Erreur -- choix du mode d'attaque pour combattre.\n");
#endif
      return FALSE;
    }

  RISK_SetAttackModeOfPlayer(iCurrentPlayer, iAttackMode);
  RISK_SetDiceModeOfPlayer(iCurrentPlayer, iDiceMode);

  /* BUG -- return true if victory? */

  if (GAME_ValidAttackSrc(iSrcCountry, TRUE) &&
      GAME_ValidAttackDst(iSrcCountry, iDstCountry, TRUE) &&
      GAME_ValidAttackDice(iDiceMode, iSrcCountry))
    {
      GAME_Attack(iSrcCountry, iDstCountry);
      return TRUE;
    }
  else
    {
      if ((iSrcCountry>=0) && (iSrcCountry<NUM_COUNTRIES))
          srcName = RISK_GetNameOfCountry(iSrcCountry);
      else
          srcName = "";
      if ((iDstCountry>=0) && (iDstCountry<NUM_COUNTRIES))
          dstName = RISK_GetNameOfCountry(iDstCountry);
      else
          dstName = "";
#ifdef ENGLISH
      printf("AI: Error -- illegal attack (%s --> %s)\n", 
#endif
#ifdef FRENCH
      printf("IA: Erreur -- illégall attaque (%s --> %s)\n", 
#endif
	     srcName, dstName);
      return FALSE;
    }
}


/************************************************************************ 
 *  FUNCTION: AI_Move
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *     23.08.95  JC   Don't check Cards and placement
 *                    if state <> STATE_MOVE.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag AI_Move(Int32 iSrcCountry, Int32 iDstCountry, Int32 iNumArmies)
{
  if (GAME_ValidMoveSrc(iSrcCountry) &&
      GAME_ValidMoveDst(iSrcCountry, iDstCountry) &&
      RISK_GetNumArmiesOfCountry(iSrcCountry) >= iNumArmies+1)
    {
      GAME_MoveArmies(iSrcCountry, iDstCountry, iNumArmies);
      return TRUE;
    }
  else
    {
#ifdef ENGLISH
      printf("AI: Error -- illegal move (%s --> %s)\n", 
#endif
#ifdef FRENCH
      printf("IA: Erreur -- mouvement illégal (%s --> %s)\n", 
#endif
	     RISK_GetNameOfCountry(iSrcCountry), 
	     RISK_GetNameOfCountry(iDstCountry));
      return FALSE;
    }
}


/************************************************************************ 
 *  FUNCTION: AI_ExchangeCards
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag AI_ExchangeCards(Int32 *piCards)
{
  GAME_ExchangeCards(piCards);
  return TRUE;
}


/************************************************************************ 
 *  FUNCTION: AI_SendMessage
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag AI_SendMessage(Int32 iMessDest, CString strMessage)
{
  MsgMessagePacket mess;

  mess.strMessage = strMessage;
  mess.iFrom      = iCurrentPlayer;
  mess.iTo        = iMessDest;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			 MSG_MESSAGEPACKET, &mess);
  return TRUE;
}


/************************************************************************ 
 *  FUNCTION: AI_EndTurn
 *  HISTORY: 
 *     03.31.95  ESF  Created.
 *     21.08.95  JC   Added fGetsCard.
 *     23.08.95  JC   Added test the mission.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag AI_EndTurn(void)
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

  if (fGameStarted)
      fCanExchange = TRUE;
  else
      fPlayerFortified = FALSE;

  /* Actually end the turn */
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			 MSG_ENDTURN, NULL);

  return TRUE;
}
