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
 *   $Id: colormap.h,v 1.9 2000/01/15 11:07:32 morphy Exp $
 *
 *   $Log: colormap.h,v $
 *   Revision 1.9  2000/01/15 11:07:32  morphy
 *   Corrected copyright notice
 *
 *   Revision 1.8  2000/01/12 19:35:45  tony
 *   added $Log and $Id tags
 *
 */

#ifndef _COLORMAP
#define _COLORMAP

#include "riskgame.h"
#include "types.h"

/* Data structures */
typedef struct _Color
{
  unsigned char r, g, b;
} Color;

#define BLACK 255
#define WHITE 254

void   COLOR_Init(void);
void   COLOR_SetWorldColormap(Byte *rawmap);
void   COLOR_SetWorldColors(void);
Int32  COLOR_DieToColor(Int32 iDie);
Int32  COLOR_PlayerToColor(Int32 iPlayer);
void   COLOR_ColorCountry(Int32 iCountry, Int32 iPlayer);
Int32  COLOR_CountryToColor(Int32 iCountry);
Int32  COLOR_ColorToCountry(Int32 iColor);
void   COLOR_CopyColor(Int32 iSrc, Int32 iDst);
void   COLOR_StoreNamedColor(CString strPlayerColor, Int32 iPlayer);
void   COLOR_StoreColor(Int32 iColorNum, Int32 iRed, Int32 iGreen, Int32 iBlue);
void   COLOR_GetColor(Int32 iColor, UInt16 *iRed, UInt16 *iGreen, 
		      UInt16 *iBlue);
void   COLOR_GetColormap(void *pVisualArgs, Int32 *piNumArgs, 
			 Int32 iNeededColors, Int32 argc, CString *argv);
Flag   COLOR_IsTrueColor();
Int32 COLOR_QueryColor(Int32 iColor);
Int32 COLOR_rgbToNum(Int32 r, Int32 g, Int32 b);
Int32 COLOR_GetNumColors(void);
void COLOR_SetNumColors(Int32 iNum);
Int32 COLOR_GetDepth();

extern Colormap        cmapColormap;
/*
extern Int32           iNumColors;
extern Int32           COLOR_Depth;
*/
#endif
