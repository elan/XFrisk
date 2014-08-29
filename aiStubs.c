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
 *   $Id: aiStubs.c,v 1.4 1999/11/13 21:58:30 morphy Exp $
 */

#include <stdio.h>

#include "riskgame.h"
#include "game.h"
#include "types.h"
#include "utils.h"

extern Int32 iCurrentPlayer;

Int32 UTIL_PopupDialog(CString strTitle, CString strQuestion, 
		       Int32 iNumOptions, CString strOption1, 
		       CString strOption2, CString strOption3)
{
  UNUSED(iNumOptions);
  UNUSED(strOption1);
  UNUSED(strOption2);
  UNUSED(strOption3);

#ifdef ENGLISH
  printf("AI: [%s] `%s'\n", strTitle, strQuestion);
#endif
#ifdef FRENCH
  printf("IA: [%s] `%s'\n", strTitle, strQuestion);
#endif
  return 0;
}

void UTIL_ServerEnterState(Int32 iNewState) 
{
  UNUSED(iNewState);
}

void UTIL_DisplayComment(CString strComment) 
{
  UNUSED(strComment);
/*
#ifdef ENGLISH
  printf("AI: `%s'\n", strComment);
#endif
#ifdef FRENCH
  printf("IA: `%s'\n", strComment);
#endif
*/}

void UTIL_DisplayError(CString strError) 
{
  UNUSED(strError);
/*
#ifdef ENGLISH
  printf("AI: `%s'\n", strError);
#endif
#ifdef FRENCH
  printf("IA: `%s'\n", strError);
#endif
*/}

void   UTIL_DarkenCountry(Int32 iCountry) { 
  UNUSED(iCountry); 
}
void   UTIL_LightCountry(Int32 iCountry) {
  UNUSED(iCountry); 
}
void   UTIL_DisplayActionCString(Int32 iState, Int32 iPlayer) {
  UNUSED(iState);
  UNUSED(iPlayer);
}
void   UTIL_ExitProgram(Int32 iExitValue) { exit(iExitValue); }
void   REG_PopupDialog(void) {}
void   COLOR_SetWorldColors() {}
Int32  COLOR_DieToColor(Int32 iDie) { 
  UNUSED(iDie);
  return 0; 
}
Int32  COLOR_PlayerToColor(Int32 iPlayer) { 
  UNUSED(iPlayer);
  return 0; 
}
void   COLOR_CopyColor(Int32 iSrc, Int32 iDst) {
  UNUSED(iSrc);
  UNUSED(iDst);
}
void   DICE_Refresh(void) {}
void   DICE_Hide(void) {}

