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
 *   $Id: deck.c,v 1.11 1999/12/25 19:18:09 morphy Exp $
 *   $Log: deck.c,v $
 *   Revision 1.11  1999/12/25 19:18:09  morphy
 *   Corrected comment errors
 *
 *   Revision 1.10  1999/12/19 22:48:27  tony
 *   cl0d feexed the greenland card!
 *
 *   Revision 1.9  1999/12/19 19:25:43  tony
 *   added $Log: deck.c,v $
 *   added Revision 1.11  1999/12/25 19:18:09  morphy
 *   added Corrected comment errors
 *   added
 *   added Revision 1.10  1999/12/19 22:48:27  tony
 *   added cl0d feexed the greenland card!
 *   added
 *
 */

/** \file
 * Card deck handling routines for the server
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "deck.h"
#include "debug.h"

/** \struct _Deck
 * Structure for card deck data.
 *
 * \b Note: This is now opaque - it can only be manipulated within \link deck.c deck.c \endlink
 */
struct _Deck
{
  Int32  iCardsLeft;       /**< Number of cards still in the deck */
  Int32  iCardsReturned;   /**< Number of cards returned to the deck */
  Int32  iTotalCards;      /**< Total number of cards in deck */
  Int32 *piCards;          /**< Pointer to list of cards still in the deck */
  Int32 *piCardsReturned;  /**< Pointer to list of cards returned to the deck */
};


/**
 * Create a card deck.
 *
 * \b History:
 * \arg 02.04.94  ESF Created.
 * \arg 03.16.94  ESF Added code for returned card handling.
 */
Deck *DECK_Create(Int32 iNumCards)
{
  Deck   *pDeck = (Deck *)MEM_Alloc(sizeof(Deck));
  Int32   i;

  /* Seed the random number generator */
  srand(time(NULL));

  /* Init the structure */
  pDeck->iCardsLeft = pDeck->iTotalCards = iNumCards;
  pDeck->iCardsReturned = 0;
  pDeck->piCards         = (Int32 *)MEM_Alloc(sizeof(Int32)*iNumCards);
  pDeck->piCardsReturned = (Int32 *)MEM_Alloc(sizeof(Int32)*iNumCards);

  /* Init all of the cards */
  for(i=0; i!=iNumCards; i++)
    pDeck->piCards[i] = i;

  return(pDeck);
}


/**
 * Draw a random card from the deck.
 *
 * \b History: 
 * \arg 02.04.94  ESF  Created.
 * \arg 03.16.94  ESF  Added code for returned card handling.
 * \arg 05.15.94  ESF  Changed to return -1 when deck is empty.
 */
Int32 DECK_GetCard(Deck *pDeck)
{
  Int32 iCardIndex, iCard;
  
  /* If there are no cards left, take all of the cards from the 
   * returned pile and put them into the deck.
   */

  if (!pDeck->iCardsLeft)
    {
      /* Are there cards to return to the deck? */
      if (!pDeck->iCardsReturned)
	return(-1);

      /* Copy all of the returned cards to the deck */
      memcpy((char *)pDeck->piCards, (char *)pDeck->piCardsReturned, 
	     sizeof(Int32)*pDeck->iCardsReturned);
      pDeck->iCardsLeft = pDeck->iCardsReturned;
      pDeck->iCardsReturned = 0;

#ifdef MEM_DEBUG
      {
	Int32 i;
	printf("New deck:\n");
	for (i=0; i!=pDeck->iCardsLeft; i++)
	  printf("%d.", pDeck->piCards[i]);
	printf("\n");
      }
#endif
    }

  /* Pick a card out of the remaining ones */
  iCardIndex = rand() % pDeck->iCardsLeft;

  /* Switch the last card with this one. */
  iCard = pDeck->piCards[iCardIndex];
  pDeck->piCards[iCardIndex] = pDeck->piCards[--pDeck->iCardsLeft];

  return(iCard);
}


/**
 * Add (return) a card to the deck.
 * \param pDeck Pointer to the deck
 * \param iCard Number of card to add
 *
 * \b History:
 * \arg 02.04.94  ESF  Created.
 * \arg 03.16.94  ESF  Fixed so that it would act like a real deck.
 * \arg 03.29.94  ESF  Fixed a dumb bug in full deck detection.
 * \arg 25.08.95  JC   Check total of cards.
 * \arg 15.11.99  Tdh  Changed '>' into '>='
 * \arg 17.12.99  MSH  Removed superfluous UTIL_ExitProgram() call

 */
void DECK_PutCard(Deck *pDeck, Int32 iCard)
{
    /* If returned card pile fills up, this is bad */
    D_Assert(pDeck,"no deck passed");
  if ((pDeck->iCardsLeft + pDeck->iCardsReturned) >= pDeck->iTotalCards)
    {
#ifdef ENGLISH
      printf("Fatal Error! DECK: Can't add to full deck!\n");
#endif
#ifdef FRENCH
      printf("Erreur fatale! Trop de cartes dans le jeu!\n");
#endif
      return;
    }
  
  /* Add the card to the retured cards pile */
  pDeck->piCardsReturned[pDeck->iCardsReturned++] = iCard;
}


/**
 * Destroy the deck.
 *
 * \b History:
 * \arg 02.04.94  ESF  Created.
 * \arg 03.16.94  ESF  Added code for returned card handling.
 */
void DECK_Destroy(Deck *pDeck)
{
  MEM_Free(pDeck->piCards);
  MEM_Free(pDeck->piCardsReturned);
  MEM_Free(pDeck);
}
