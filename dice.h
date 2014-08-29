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
 *   $Id: dice.h,v 1.4 1999/11/13 21:58:31 morphy Exp $
 */

#ifndef _DICE
#define _DICE

#include "types.h"

#ifndef DICE_AI
void   DICE_Init(void);
void   DICE_DrawDice(Int32 *pAttackDice, Int32 *pDefendDice);
#endif

void   DICE_Refresh(void);
void   DICE_Hide(void);
Int32  DICE_Attack(Int32 iAttack, Int32 iDefend, 
		   Int32 iAttacker, Int32 iDefender);
#endif
