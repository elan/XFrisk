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
 *   $Id: game.h,v 1.6 1999/11/13 21:58:31 morphy Exp $
 */

#ifndef _GAME
#define _GAME

#include "types.h"

#define PLACE_ONE       0
#define PLACE_MULTIPLE  1
#define PLACE_ALL       2



void     GAME_PlaceClick(Int32 iCountry, Int32 iPlaceType);
void     GAME_PlaceArmies(Int32 iCountry, Int32 iNumArmies);
Flag     GAME_ValidPlaceDst(Int32 iPlaceDst);

void     GAME_MoveArmies(Int32 iSrcCountry, Int32 iDstCountry, 
			 Int32 iNumArmies);
void     GAME_MoveClick(Int32 iCountry);
Flag     GAME_ValidMoveSrc(Int32 iMoveSrc);
Flag     GAME_ValidMoveDst(Int32 iMoveSrc, Int32 iMoveDst);

void     GAME_DoAttack(Int32 iSrcCountry, Int32 iDstCountry, 
		       Flag fCacheNotify);
void     GAME_Attack(Int32 iSrcCountry, Int32 iDstCountry);
void     GAME_AttackClick(Int32 iCountry);
Flag     GAME_ValidAttackSrc(Int32 iCountry, Flag fVerbose);
Flag     GAME_ValidAttackDst(Int32 iAttackSrc, Int32 iCountry, Flag fVerbose);
Flag     GAME_ValidAttackDice(Int32 iAttackDice, Int32 iCountry);
Flag     GAME_IsEnemyAdjacent(Int32 iCountry);
Flag     GAME_IsFrontierAdjacent(Int32 srcCountry, Int32 destCountry);
Int32    GAME_FindEnemyAdjacent(Int32 iCountry);


void     GAME_SetTurnArmiesOfPlayer(Int32 iCurrentPlayer);
Int32    GAME_GetContinentBonus(Int32 iPlayer);
void     GAME_PlayerDied(Int32 iDeadPlayer);
void     GAME_GetCard(Int32 iPlayer);
void     GAME_ExchangeCards(Int32 *piCards);
void     GAME_MissionAccomplied(Int32 iWinner, Int16 iTyp, Int32 iNum1, Int32 iNum2);
void     GAME_Victory(Int32 iWinner);
void     GAME_GameOverMan(void);
void     GAME_EndTurn(void);

Flag     GAME_IsMissionAccomplied(Int32 iPlayer, Int32 iKilledPlayer);
void     GAME_ShowMission(Int32 iPlayer);
Int32    GAME_AttackFrom(void);
void     GAME_SetAttackSrc(Int32 src);

/* Private routines <sic> */
Flag  GAME_CanAttack(Int32 AttackSrc, Int32 iCountry);


extern Int32      iLastAttackSrc, iLastAttackDst;
extern Int32      iMoveSrc;
extern Flag       fGetsCard, fCanExchange;

#endif
