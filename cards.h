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
 *   $Id: cards.h,v 1.5 2000/01/05 23:21:13 tony Exp $
 */

#ifndef _CARDS
#define _CARDS

#include "types.h"
#include <X11/Xlib.h>

#define CARD_WIDTH  138
#define CARD_HEIGHT 190



/* Card types */
#define CARD_CANON    0
#define CARD_FOOTMAN  1
#define CARD_HORSEMAN 2
#define CARD_JOKER    3 



/* Initialize */
void CARDS_Init();

/* The main function */
void    CARDS_RenderCard(Int32 iCard, Int32 iPositions);

/* Worker functions */
XImage *CARDS_ScaleImage(XImage *pimageInput, Int32 iMaxWidth, 
			Int32 iMaxHeight);
XImage *CARDS_GetCountryImage(Int32 iCountry, Int32 iFgColor, Int32 iBgColor);

/* This is the mapping from (card #) :==> (country, type) where country
 * is the country to portray on the card, and type is defined in risk.h,
 * as CARD_[FOOTMAN|HORSEMAN|CANNON|JOKER]:
 *
 * The first 42 cards are: Card i is of country i and of type (i mod 3).
 * The next 2 cards are jokers.
 */

#endif
