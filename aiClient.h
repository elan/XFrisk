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
 *   $Id: aiClient.h,v 1.3 1999/11/13 21:58:30 morphy Exp $
 */

#ifndef _AICLIENT
#define _AICLIENT

#include "types.h"

/* Private stuff */
#define DefineSpecies(callback, name, author, version, desc) \
void *(*__AI_Callback)(void *, Int32, void *) = callback; \
CString __strName = name; \
CString __strAuthor = author; \
CString __strVersion = version; \
CString __strDesc = desc;

/* Registering to observe the distributed object (for advanced clients
 * only -- actually this was only added because the client I was
 * porting over needed it, but it might be useful for others).
 * Preobservers get called before the object is changed, so that they
 * can see what changed.  Postobservers get called after the object is
 * changed.
 */

void AI_RegisterPreObserver(void (*PreCallback)(Int32, void *));
void AI_RegisterPostObserver(void (*PostCallback)(Int32, void *));

/* Commands sent to the callback */
#define AI_INIT_ONCE        0
#define AI_INIT_GAME        1
#define AI_INIT_TURN        2
#define AI_FORTIFY          3 /* (Number of armies) */
#define AI_PLACE            4
#define AI_ATTACK           5
#define AI_MOVE             6
#define AI_EXCHANGE_CARDS   7
#define AI_SERVER_MESSAGE   8 /* (Text of the message) */
#define AI_MESSAGE          9 /* (Text of the message) */
#define AI_MOVE_MANUAL     10 /* (Number of armies) */

/* Actions computer players can perform -- PUBLIC INTERFACE */
Flag AI_Place(Int32 iCountry, Int32 iNumArmies);
Flag AI_Attack(Int32 iSrcCountry, Int32 iDstCountry, 
	       Int32 iAttackMode, Int32 iDiceMode, Int32 iMoveMode);
Flag AI_Move(Int32 iSrcCountry, Int32 iDstCountry, Int32 iNumArmies);
Flag AI_ExchangeCards(Int32 *piCards);
Flag AI_SendMessage(Int32 iMessDest, CString strMessage);
Flag AI_EndTurn(void);

/* Attack modes */
#define ATTACK_ONCE      0
#define ATTACK_DOORDIE   1

/* Dice modes */
#define DICE_MAXIMUM     0
#define DICE_ONE         1
#define DICE_TWO         2
#define DICE_THREE       3

/* What to do with armies after taking a country.  If you wish to move
 * the maximum amount of armies into the country you might occupy with
 * the attack, pass ARMIES_MOVE_MAX.  If you want the minimum number
 * of armies moved, pass ARMIES_MOVE_MIN.  If you want finer control,
 * pass in ARMIES_MOVE_MANUAL, and your callback will be called with
 * the command AI_MOVE_MANUAL.
 */

#define ARMIES_MOVE_MAX    0
#define ARMIES_MOVE_MIN    1
#define ARMIES_MOVE_MANUAL 2

#endif
