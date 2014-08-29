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
 *   $Id: diceCommon.c,v 1.9 2000/01/10 22:47:40 tony Exp $
 */

#ifndef DICE_AI
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <stdlib.h>
#include <stdio.h>
#endif


#include "dice.h"
#include "riskgame.h"
#include "debug.h"
#include "types.h"
#include "client.h"
#include "utils.h"

#ifndef DICE_AI
#include "gui-vars.h"
#include "colormap.h"
#include "dice.bmp"
#endif

#ifndef DICE_AI
static Pixmap   pixAttackDice[6];
static Pixmap   pixDefendDice[6];
static Window   hDiceWindow;

static Int32    iWidth, iHeight;
static Int32    iHorizontalOffset, iVerticalOffset;
static Int32    iBoxWidth, iBoxHeight;
static Int32    iBackgroundColor;
#endif

static Int32    piAttackDice[3] = {-1, -1, -1}; 
static Int32    piDefendDice[3] = {-1, -1, -1};

#define DICE_Swap(a, b, temp) { temp=a; a=b; b=temp; }
#define DICE_SPACING 6

#ifndef DICE_AI
/************************************************************************ 
 *  FUNCTION: DICE_Creat
 *  HISTORY: 
 *     17.08.95  JC   Created (copy of DICE_Init).
 *     23.08.95  JC   Use of COLOR_Depth.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void DICE_Creat(Flag eff, Int32 iAttack, Int32 iDefend)
{
  static Int32 attackColor = -1;
  static Int32 defendColor = -1;
  static struct
    {
      Byte *pbBits;
    } pDice[] = 
      {
	{ one_bits }, 
	{ two_bits },
	{ three_bits },
	{ four_bits },
	{ five_bits },
	{ six_bits },
      };
  Int32       i;
  Dimension   dimBoxWidth, dimBoxHeight;

  hDiceWindow = XtWindow(wDiceBox);

  /* Find the width and height of the bitmaps, assuming THEY ARE EQUAL */
  iWidth = one_height; iHeight = one_width;

  /* Get data from the die box widget */
  XtVaGetValues(wDiceBox, 
		XtNwidth, &dimBoxWidth, 
		XtNheight, &dimBoxHeight, 
		XtNbackground, &iBackgroundColor,
		NULL);
  iBoxWidth = dimBoxWidth;
  iBoxHeight = dimBoxHeight;

  iHorizontalOffset = ((Int32)dimBoxWidth-3*iWidth-2*DICE_SPACING)/2;
  iVerticalOffset   = ((Int32)dimBoxHeight-2*iWidth-DICE_SPACING)/2;

  iAttack = COLOR_QueryColor(iAttack);
  if (iAttack != attackColor)
    {
      attackColor = iAttack;
      /* Create the attack dice */
      for (i=0; i!=6; i++)
        {
          if (eff)
              XFreePixmap (hDisplay, pixAttackDice[i]);
          pixAttackDice[i] = XCreatePixmapFromBitmapData(hDisplay, hDiceWindow, 
						         (char *)pDice[i].pbBits, 
						         iWidth, iHeight,
						         BlackPixel(hDisplay, 0),
						         attackColor, 
						         COLOR_GetDepth());
        }
    }

  iDefend = COLOR_QueryColor(iDefend);
  if (iDefend != defendColor)
    {
      defendColor = iDefend;
      /* Create the defend dice */
      for (i=0; i!=6; i++)
        {
          if (eff)
              XFreePixmap (hDisplay, pixDefendDice[i]);
          pixDefendDice[i] = XCreatePixmapFromBitmapData(hDisplay, hDiceWindow, 
						         (char *)pDice[i].pbBits,
						         iWidth, iHeight,
						         BlackPixel(hDisplay, 0),
						         defendColor, 
						         COLOR_GetDepth());
        }
    }
}


/************************************************************************ 
 *  FUNCTION: DICE_Init
 *  HISTORY: 
 *     02.07.94  ESF  Created.
 *     02.10.94  ESF  Started work on coloring dice.
 *     03.03.94  ESF  Changed to calculate offset numbers.
 *     03.05.94  ESF  Changed to use varargs calls.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void DICE_Init(void)
{
  DICE_Creat(FALSE, COLOR_DieToColor(0), COLOR_DieToColor(1));
}
#endif

/************************************************************************ 
 *  FUNCTION: DICE_Attack
 *  HISTORY: 
 *     02.07.94  ESF  Created.
 *     03.03.94  ESF  Changed to center the dice correctly.
 *     03.03.94  ESF  Adding sorting of the dice and misc. checks.
 *     03.07.94  ESF  Fixed iArmiesWon calculation.
 *     04.11.94  ESF  Added refresh storage, factored out refresh code.
 *     10.30.94  ESF  Added sending message to server, attacker/defender.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 DICE_Attack(Int32 iAttack, Int32 iDefend, 
		  Int32 iAttacker, Int32 iDefender)
{
  Int32        i, j, iTemp;
  Int32        iArmiesWon=0;
  MsgDiceRoll  msgDiceRoll;
  UNUSED(iAttacker);

  if (iAttack>3 || iAttack<0 || iDefend>2 || iDefend<0)
    {
      /* (void)UTIL_PopupDialog("Fatal Error", 
	 "DICE: Attack has bogus params...\n",
	 1, "Ok", NULL, NULL); */
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_EXIT, NULL);
      UTIL_ExitProgram(-1);
    }

  /* Erase previous rolls */
  DICE_Hide();

  /* Init dice */
  for (i=0; i!=3; i++)
    piAttackDice[i] = piDefendDice[i] = -1;

  /* Get the attack rolls (ranging 0..5 !!*/
  for (i=0; i!=iAttack; i++)
      piAttackDice[i] = (int) (6.0 * (rand()/(RAND_MAX+1.0)));

  
  /* Get the defense rolls */
  for (i=0; i!=iDefend; i++)
    piDefendDice[i] = (int) (6.0 * (rand()/(RAND_MAX+1.0)));

  /* Sort the dice rolls using bubble sort (at most needs two passes) */
  for (i=0; i!=2; i++)
    for (j=0; j!=2-i; j++)
      {
	if (piAttackDice[j] < piAttackDice[j+1])
	  DICE_Swap(piAttackDice[j], piAttackDice[j+1], iTemp);
	if (piDefendDice[j] < piDefendDice[j+1])
	  DICE_Swap(piDefendDice[j], piDefendDice[j+1], iTemp);
      }

  /* Send a message to the server about the dice roll */
  for (i=0; i!=3; i++)
    {
      msgDiceRoll.pAttackDice[i] = piAttackDice[i];
      msgDiceRoll.pDefendDice[i] = piDefendDice[i];
    }
  msgDiceRoll.iDefendingPlayer = iDefender;
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_DICEROLL, &msgDiceRoll);

  /* Show the dice */
  DICE_Refresh();

  /* Find out the outcome of the battle */
  for (i=0; i!=MIN(iDefend, iAttack); i++)
    if (piAttackDice[i] > piDefendDice[i])
      iArmiesWon++;
  
  return iArmiesWon; /* For defender, iArmiesWon = iDefend-iArmiesWon */
}

#ifndef DICE_AI
/************************************************************************ 
 *  FUNCTION: DICE_Hide
 *  HISTORY: 
 *     02.07.94  ESF  Stubbed.
 *     03.06.94  ESF  Coded.
 *     04.11.94  ESF  Added invalidation of the dice.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void DICE_Hide(void)
{
  Int32 i;

  /* Delete the contents of the box */
  XSetForeground(hDisplay, hGC, iBackgroundColor);
  XFillRectangle(hDisplay, hDiceWindow, hGC, 0, 0, iBoxWidth, iBoxHeight); 

  /* Invalidate the saved dice */
  for (i=0; i!=3; i++)
    piAttackDice[i] = piDefendDice[i] = -1;
}


/************************************************************************ 
 *  FUNCTION: DICE_Refresh
 *  HISTORY: 
 *     04.11.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void DICE_Refresh(void)
{
  DICE_DrawDice(piAttackDice, piDefendDice);
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     10.30.94  ESF  Moved here and added parameter validation.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void DICE_DrawDice(Int32 *pAttackDice, Int32 *pDefendDice)
{
  Int32 i;

  for (i=0; i!=3; i++)
    {
      /* Save these so that a refresh can see them */
      piAttackDice[i] = pAttackDice[i];
      piDefendDice[i] = pDefendDice[i];

      /* Parameter validation */
      D_Assert(pAttackDice[i]==-1 || (pAttackDice[i]>=0 && pAttackDice[i]<=5),
	       "Dice number is out of range.");
      D_Assert(pDefendDice[i]==-1 || (pDefendDice[i]>=0 && pDefendDice[i]<=5),
	       "Dice number is out of range.");
    }

  DICE_Creat(TRUE, COLOR_DieToColor(0), COLOR_DieToColor(1));
  /* Show the dice */
  for (i=0; i!=3; i++)
    {
      if (piAttackDice[i] != -1)
	XCopyArea(hDisplay, pixAttackDice[pAttackDice[i]], hDiceWindow, hGC, 
		  0, 0, iWidth, iHeight, 
		  iHorizontalOffset + i*(iWidth+DICE_SPACING), 
		  iVerticalOffset);
      if (piDefendDice[i] != -1)
	XCopyArea(hDisplay, pixDefendDice[pDefendDice[i]], hDiceWindow, hGC, 
		  0, 0, iWidth, iHeight, 
		  iHorizontalOffset + i*(iWidth+DICE_SPACING), 
		  iVerticalOffset + (iHeight+DICE_SPACING));
    }
}
#endif
