/**
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
 *   $Id: cards.c,v 1.20 2000/01/12 19:40:41 morphy Exp $
 *
 *   $Log: cards.c,v $
 *   Revision 1.20  2000/01/12 19:40:41  morphy
 *   Comment changes, removed note about Argentina card color (not current anymore)
 *
 *   Revision 1.19  2000/01/10 22:47:40  tony
 *   made colorstuff more private to colormap.c, only scrollbars get set wrong, rest seems to work ok now
 *
 *   Revision 1.18  2000/01/09 16:07:46  tony
 *   wrong doxygen tags
 *
 *   Revision 1.17  2000/01/08 18:38:03  tony
 *   oops, even dumped core
 *
 *   Revision 1.16  2000/01/08 17:28:03  tony
 *   comment added
 *
 *   Revision 1.15  2000/01/08 01:45:34  tony
 *   color greenland card fixed! ?
 *
 *   Revision 1.13  2000/01/04 21:41:53  tony
 *   removed redundant stuff for jokers
 *
 *   Revision 1.12  2000/01/04 21:11:06  tony
 *   a bit more structure by using Cards[]
 *
 *   Revision 1.11  1999/12/25 21:58:02  morphy
 *   Fixed typo in doxygen file comment
 *
 *   Revision 1.10  1999/12/19 22:51:09  tony
 *   cl0d fixed greenland card
 *
 */

/** \file
 * Graphical part of cards handling for client
 */

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "network.h"
#include "gui-vars.h"
#include "riskgame.h"
#include "client.h"
#include "types.h"
#include "utils.h"
#include "cards.h"
#include "colormap.h"
#include "callbacks.h"
#include "debug.h"

#define PICTURE_FRACTION 0.6

/* Private function */
static void _CARDS_ComputeScaleVector(Int32 *piVector, Int32 x0, Int32 y0, 
				     Int32 x1, Int32 y1);

/* Private data */

/** A structure to hold the directory information */
static struct Directory
{
  Int32   iWidth, iHeight, iLength;
  Int32   lOffset;
} pDirectory[NUM_COUNTRIES];

/** Cards */
static struct {
    Pixmap pixmap;
    Int32 color;
} Cards[NUM_CARDS];

/** Font structure for card texts */
static XFontStruct   *pCardFont;


/**
 * Initialize the set of cards, called once on start
 *
 * \b  History:
 * \arg 27.10.99 TdH Moved out of CARDS_RenderCard
 */
void CARDS_Init() {
    int i;
    /* Load the font */
    if ((pCardFont=XLoadQueryFont(hDisplay, "*helvetica-b*-r-*12*")) == NULL)
    {
        (void)UTIL_PopupDialog("Warning",
                               "Could not open card font (using fixed)\n", 1,
                               "Ok", NULL, NULL);
        /* Assume 'fixed' is always there -- not good. */
        pCardFont = XLoadQueryFont(hDisplay, "fixed");
    }
    /* Init the pixmap cache */
    for (i=0; i!=NUM_CARDS; i++)  {
        Cards[i].pixmap = 0L;
        Cards[i].color = -1;
    }
}


/**
 * Set color of card, create bitmap if not yet there
 * \b History:
 * \arg 05.01.00 TdH Created
 *
 * \b  Notes:
 * Took out of CARDS_RenderCard
 */

void CARDS_SetColor(Int32 iCard, Int32 iColor) {
    XImage  *pimageCountry;
    Int32    iPictureWidth, iPictureHeight, iPictureOffset;
    Int32    iFontHeight, iFontWidth;
    Int32    blackpixel,whitepixel;
    Int32    x, y, c;
    char buf[256];

    Cards[iCard].color = iColor;/* new color */
    whitepixel = WhitePixel(hDisplay, 0);
    blackpixel = BlackPixel(hDisplay, 0);


    /* Makes life easier */
    iPictureWidth  = CARD_WIDTH;
    iPictureHeight = (double)CARD_HEIGHT * PICTURE_FRACTION;
    iPictureOffset = CARD_HEIGHT - iPictureHeight;

    /* Check to see if pixmap is in cache.  If not, build it */
    if (Cards[iCard].pixmap == 0L)   {
        Cards[iCard].pixmap = XCreatePixmap(hDisplay, pixMapImage,
                                            CARD_WIDTH, CARD_HEIGHT,
                                            COLOR_GetDepth());

        /* Fill the card up with white and black */
        XSetForeground(hDisplay, hGC, blackpixel);
        XFillRectangle(hDisplay, Cards[iCard].pixmap, hGC,
                       0, 0,
                       CARD_WIDTH, CARD_HEIGHT);
        XSetForeground(hDisplay, hGC, WhitePixel(hDisplay, 0));
        XFillRectangle(hDisplay, Cards[iCard].pixmap, hGC,
                       0, 0,
                       CARD_WIDTH, iPictureOffset);
    }

    pimageCountry = CARDS_GetCountryImage(iCard, -1, -1);

    /* Compress the card image, being careful about the pointer */
    pimageCountry = CARDS_ScaleImage(pimageCountry,
                                    (Int32)(4.0/5.0 * (float)iPictureWidth),
                                    (Int32)(4.0/5.0 * (float)iPictureHeight));

    /* Dump image, centered, on lower 2/3 of card */
    if (COLOR_IsTrueColor()) {
        for (y = 0; y < pimageCountry->height; y++) {
            for (x = 0; x < pimageCountry->width; x++)  {
                c = XGetPixel(pimageCountry, x, y);
                if (c != blackpixel) {
                    c = iColor;
                }
                XSetForeground(hDisplay, hGC, c);
                XDrawPoint(hDisplay, Cards[iCard].pixmap, hGC,
                           (iPictureWidth - pimageCountry->width)/2 + x,
                           iPictureOffset + (iPictureHeight - pimageCountry->height)/2 + y);
            }
        }
    }  else
        XPutImage(hDisplay, Cards[iCard].pixmap, hGC, pimageCountry, 0, 0,
                  (iPictureWidth - pimageCountry->width)/2,
                  iPictureOffset +
                  (iPictureHeight - pimageCountry->height)/2,
                  pimageCountry->width,
                  pimageCountry->height);


    /* Write the name of the country on the card */
    iFontHeight = (pCardFont->max_bounds.ascent
                   + pFont->max_bounds.descent);
    iFontWidth  = XTextWidth(pCardFont,
                             RISK_GetNameOfCountry(iCard),
                             strlen(RISK_GetNameOfCountry(iCard)));

    /* Set the font */
    XSetFont(hDisplay, hGC, pCardFont->fid);

    XSetForeground(hDisplay, hGC, whitepixel);
    XDrawString(hDisplay, Cards[iCard].pixmap, hGC,
                (iPictureWidth - iFontWidth)/2,
                iPictureOffset+iFontHeight,
                RISK_GetNameOfCountry(iCard),
                strlen(RISK_GetNameOfCountry(iCard)));

    /* Lose all of the used memory */
    XDestroyImage(pimageCountry);

    /* Now put the appropriate bitmap there */
#ifdef ENGLISH
    snprintf(buf, sizeof(buf), "%s", (iCard%3==0) ? "Cavalry" :
             (iCard%3==1) ? "Infantry" : "Artillery");
#endif
#ifdef FRENCH
    snprintf(buf, sizeof(buf), "%s", (iCard%3==0) ? "Cavalerie" :
             (iCard%3==1) ? "Infanterie" : "Artillerie");
#endif
    iFontWidth = XTextWidth(pCardFont, buf, strlen(buf));
    XSetForeground(hDisplay, hGC, blackpixel);
    XDrawString(hDisplay, Cards[iCard].pixmap, hGC,
                (CARD_WIDTH - iFontWidth)/2,
                iFontHeight,
                buf, strlen(buf));
}


/**
 * Create pixmap for joker
 *
 * \b History:
 * \arg 05.01.00 TdH Created
 *
 * \b Notes:
 * Took out of CARDS_RenderCard
 */

void CARDS_GetJoker(Int32 iCard) {
    char buf[256];

    if (Cards[iCard].pixmap == 0L){
        Int32    iFontHeight, iFontWidth;

        /* Makes life easier */

        Cards[iCard].pixmap = XCreatePixmap(hDisplay, pixMapImage,
                                            CARD_WIDTH, CARD_HEIGHT, COLOR_GetDepth());

        /* Jokers are all white */
        XSetForeground(hDisplay, hGC, WhitePixel(hDisplay, 0));
        XFillRectangle(hDisplay, Cards[iCard].pixmap, hGC,
                       0, 0,
                       CARD_WIDTH, CARD_HEIGHT);
        /* Set the font */
        XSetFont(hDisplay, hGC, pCardFont->fid);
        /* It's a joker, put all of the bitmaps there ??*/
        snprintf(buf, sizeof(buf), "%s", "Joker");
        iFontWidth = XTextWidth(pCardFont, buf, strlen(buf));
        iFontHeight = (pCardFont->max_bounds.ascent +
                       pCardFont->max_bounds.descent);
        XSetForeground(hDisplay, hGC, BlackPixel(hDisplay, 0));
        XDrawString(hDisplay, Cards[iCard].pixmap, hGC,
                    (CARD_WIDTH - iFontWidth)/2,
                    iFontHeight,
                    buf, strlen(buf));
    }
}

/**
 * Draw cards
 *
 * \b History:
 * \arg 02.21.94  ESF  Created.
 * \arg 02.22.94  ESF  Fixed positioning bug, made prettier.
 * \arg 03.02.94  ESF  Added the printing of the country name.
 * \arg 03.29.94  ESF  Fixed the setup for printing the jokers.
 * \arg 04.02.94  ESF  Fixed name centering, wasn't XSetFont'ing.
 * \arg 05.06.94  ESF  Made it more beautiful, took out hacks.
 * \arg 06.25.94  ESF  Optimized card decompression.
 * \arg 09.02.94  ESF  Fixed off-by-two bug.
 * \arg 23.08.95  JC   Use COLOR_Depth.
 * \arg 27.10.99  TdH  Took out CARDS_Init to get rid of static and check
 */
 
void CARDS_RenderCard(Int32 iCard, Int32 iPosition) {
    Int32 c;
    /* Range check */
    if (iCard<0 || iCard>=NUM_CARDS || iPosition>=MAX_CARDS || iPosition<0)  {
        (void)UTIL_PopupDialog("Warning!",
                               "Bogus request to CARDS_RenderCard()!\n", 1,
                               "Ok", NULL, NULL);
        return;
    }

    /* Find out which country/image goes with the card */
    if (iCard < NUM_COUNTRIES)  {
        /* Check to see if the color of the card/country has changed
         * if card has no pixmap, color will be -1, which is no existing color
         */
        c = COLOR_QueryColor(COLOR_CountryToColor(iCard));
      if(Cards[iCard].color != c) 
          CARDS_SetColor(iCard,c);
    }
    else {/* it's a joker */
        if (Cards[iCard].pixmap == 0L)
            CARDS_GetJoker(iCard);
    }

    /* Now it's built, put it in the card's bitmap resource, manage the card */
    XtVaSetValues(wCardToggle[iPosition],XtNbitmap, Cards[iCard].pixmap, NULL);
    XtManageChild(wCardToggle[iPosition]);
}


/**
 * Using Bresenham's algorithm, squish the image passed in to
 * a box [x, y].  Return the compressed image.  Preserve aspect
 * ratio of the image, so find out the MAX of the compression in
 * the x and y directions.
 * \b History:
 * \arg ??.??.93  ESF  Created for the XDissolver.
 * \arg 02.22.94  ESF  Made pretty, changed to work with 8 bit images.
 * \arg 11.29.94  ESF  Completely rewrote to use true Bresenham algorithm.
 * \arg 01.15.95  ESF  Fixed off by one error causing memory access errors.
 */

XImage *CARDS_ScaleImage(XImage *pimageInput, Int32 iMaxWidth, Int32 iMaxHeight)
{
  XImage   *pimageCompressed;
  Byte     *pbData;
  Int32     iOutputWidth, iOutputHeight;

  /* Believe it or not, scaling a 2D image is like drawing two lines, 
   * using Bresenham's algorithm.  This algorithm is probably analogous
   * to the methods used in games such as Wolfenstein 3D, since one can
   * use a similar algorithm for rotating bitmaps in 3D.
   *
   * The key here is that the first "line" that we use to scale is the 
   * hypotenuse of a triangle.  The base of the triangle is the width of 
   * the input image, and the other side (vertical) is the width of the 
   * output image.
   *
   * The second "line" is similar, except that the dimensions of the sides
   * are the heights of the input image and output image.
   *
   * We proceed to "draw" (i.e. perform the Bresenham algorithm for) the
   * two lines, in the structure of a nested loop -- for each pixel of
   * the outer line, draw the entire inner line).
   *
   * The line is going to be in the first quadrant, since the dimensions
   * of the images are positive.  Thus, the pixels of the line will either
   * be drawn "to the right of", "above", or "above and to the right of"
   * the anterior pixel (in other words, as we draw the line, the next 
   * pixel will either go to the right, up, or diagonally up and to the
   * right.
   *
   * Remember, the base of the triangle represents the dimension of the
   * _input_ image.  As we draw the line, if the pixel that we draw is
   * _heigher_ than the last one, we copy a pixel from the source image
   * to the destination image.  The pixel that we copy is the one that
   * falls directly _below_ the pixel we are drawing, and we copy it to 
   * the location directly to the _right_ of the current pixel.
   *
   * You should be able to imagine in this fashion scaling a single
   * scan-line of an image.  The source line was the base of the 
   * triangle, and the verticle side of the triangle was the 
   * destination scan-line.  Thus, in order to scale a 2D image, we
   * simply perform two line drawings, one inside the other, as
   * mentioned above.
   *
   * As for an informal proof of this method, (not like I was ever
   * any good at _formal_ proofs), I show that the method works for
   * a scan-line at the border conditions, and then, using smoke, magic, 
   * and poor man's induction, I handwave my way into 2 dimensions.
   *
   * The easiest scaling to prove is the 1:1 ratio.  This will result in
   * a completely diagonal line.  Since we copy a pixel across whenever
   * we go up at all, we will copy across a pixel every time we move,
   * since we are moving diagonally all the way.  Thus the scan-lines
   * will be identical.  
   *
   */

  Int32    *pHorizontalLine, *pVerticalLine;
  Int32     iCompression;
  Int32     iWidthCompression, iHeightCompression;
  Int32     x, y;

  /* Calculate the dimensions of the output image, preserving aspect ratio.
   * I want to avoid using any floating point, so use a primitive fixed
   * point representation here, simply multiply the numbers by 1024 and then
   * divide the final result by the same factor.  This has two consequences:
   *
   *  a) The accuracy is limited to approximately 3 digits.
   *  b) The range on the input parameters of the calculation is
   *     limited to 2^32 >> 10 = 2^22.  I don't think there are
   *     countries larger than this in width or height :)
   */

  /* Watch out for negative numbers! */
  D_Assert(pimageInput->width >= 0 && pimageInput->height >=0, 
	   "Negative image?");

  /*** Scale (encoding) ***/
  pimageInput->width  <<= 10;
  pimageInput->height <<= 10;

  iWidthCompression = MAX(pimageInput->width / iMaxWidth, 1<<10); 
  iHeightCompression = MAX(pimageInput->height / iMaxHeight, 1<<10);

  /* To preserve the aspect ratio, choose the larger of 
   * the compression ratios, and use it in both directions.
   */

  iCompression = MAX(iWidthCompression, iHeightCompression);
  
  /* Find out the size of the output image */
  iOutputWidth = (pimageInput->width)/iCompression;
  iOutputHeight = (pimageInput->height)/iCompression;

  /*** Scale (decoding) ***/
  pimageInput->width  >>= 10;
  pimageInput->height >>= 10;

  /* After we have done this, do a sanity check on the result */
  D_Assert(iOutputWidth <= iMaxWidth && iOutputHeight <= iMaxHeight,
	   "Scaling algorithm meltdown!");
  
  /* Allocate memory for the lines */
  pHorizontalLine = (Int32 *)MEM_Alloc(sizeof(Int32)*iOutputWidth);
  pVerticalLine   = (Int32 *)MEM_Alloc(sizeof(Int32)*iOutputHeight);

  /* We want to draw lines that represent the hypotenuse of triangles
   * that have heights of pimageInput->[height | width] pixels and 
   * widths of iOutput[Width | Height] pixels.  Thus, since out lines
   * start at (0, 0), we _must_ subtract 1 from the widths and heights
   * in order to draw a line or the correct dimensions.
   */

  /* First "draw" the line representing the "width" compression */
  _CARDS_ComputeScaleVector(pHorizontalLine, 0, 0, 
			   pimageInput->width-1, iOutputWidth-1);

  /* Now "draw" the line representing the "height" compression */
  _CARDS_ComputeScaleVector(pVerticalLine, 0, 0, 
			   pimageInput->height-1, iOutputHeight-1);

  /* This will hold the compressed input data as we're compressing it */
  pbData = (Byte *)XtMalloc(sizeof(Byte)*iOutputWidth*iOutputHeight);
  pimageCompressed = XCreateImage(hDisplay, pVisual,
				  8, ZPixmap, 0, (char *)pbData, 
				  iOutputWidth, iOutputHeight, 8, 
				  iOutputWidth);

  /* Loop through the scale vectors and compress the image (UNROLL?) */
  for (y=0; y!=iOutputHeight; y++)
    for (x=0; x!=iOutputWidth; x++)
      {
	/* CSE hand optimization */
	const Int32 xx = pHorizontalLine[x], yy = pVerticalLine[y];
	
	/* Sanity check */
	D_Assert(xx>=0 && xx<pimageInput->width, "Bogus horiz. r-pixel.");
	D_Assert(yy>=0 && yy<pimageInput->height, "Bogus vert. r-pixel.");
	D_Assert(x>=0  &&  x<pimageCompressed->width, "Bogus horiz. w-pixel.");
	D_Assert(y>=0  &&  y<pimageCompressed->height, "Bogus vert. w-pixel.");

	/* Copy a pixel */
	XPutPixel(pimageCompressed, x, y, XGetPixel(pimageInput, xx, yy));
      }
  
  /* Destroy the old image */
  XDestroyImage(pimageInput);
  
  /* Free memory */
  MEM_Free(pHorizontalLine);
  MEM_Free(pVerticalLine);

  /* Return the compressed image */
  return(pimageCompressed);
}


/**
 * "Draws" a line from (x0, y0) to (x1, y1), including both endpoints,
 * which is stored in a vector of function values in which (V[y], y)
 * form the point pairs.  Caller is reponsible for allocation and
 * deallocation of vector.
 *
 * \b History:
 * \arg 12.30.94  ESF  Created.
 * \arg 01.15.95  ESF  Added check that line is in first quadrant.
 *
 * \par Notes:
 * Taken from _Computer Graphics_, by Folen and van Dam et. al.,
 * slightly hand optimized, at the cost of clarity -- sorry.
 * Remember that the vector must have (y1-y0)+1 elements allocated.
 * Also, this algorithm only draws lines in the first quadrant.
 */
static void _CARDS_ComputeScaleVector(Int32 *piVector, Int32 x0, Int32 y0, 
				     Int32 x1, Int32 y1)
{
  Int32 dx, dy, incrE, incrNE, d, x, y;
  
  /* Ensure that the line is in the first quadrant */
  if (x0>x1 || y0>y1)
    {
#ifdef ENGLISH
      printf("Error: (_CARDS_ScaleVector) Line not in first quadrant.");
#endif
#ifdef ENGLISH
      printf("Erreur: (_CARDS_ScaleVector) La ligne n'est pas dans le premier quadrant.");
#endif
      UTIL_ExitProgram(-1);
    }

  /* Initialize variables, prime the algorithm */
  dx      = x1 - x0;
  dy      = y1 - y0;
  d       = 2*dy - dx;
  incrE   = 2*dy;
  incrNE  = 2*(dy-dx);
  x       = x0;
  y       = y0;

  /* Map one value (like drawing a pixel) */
  piVector[y] = x;

  while (x < x1)
    {
      if (d <= 0)
	x++, d+=incrE;
      else
	x++, y++, d+=incrNE;

      /* Map one value (like drawing a pixel) */
      piVector[y] = x;
    }
}


/**
 * \b History:
 * \arg 01.22.95  ESF  Created.
 * \arg 06.05.97  DAH  Make hFile a local; remember to close it.
 * \arg 06.06.97  DAH  Actually, it needs to be static.
 * \arg 19.12.99  TdH  Cl0d suggested this fix, greenland card now shows
 */
XImage *CARDS_GetCountryImage(Int32 iCountry, Int32 iFgColor, Int32 iBgColor)
{
  static Flag  fDirectoryRead = FALSE;
  Int32        iCardColor, iCardBackground, i;
  Int32        iNumCountries;
  Byte         bLength, bColor;
  Byte        *pbImage, *pTemp, *pTemp2;
  Byte        *pbBogus, *pbCompressed;
  XImage      *pimageCountry;
  static FILE *hFile;
  char buf[256];

  const Int32  iCard = iCountry;
  
  if (!fDirectoryRead)
    {
      fDirectoryRead = TRUE;
      
      /* Open the file */
      if ((hFile=UTIL_OpenFile(COUNTRYFILE, "r"))==NULL)
	{
#ifdef ENGLISH
	  snprintf(buf, sizeof(buf), "CARDS: Cannot open %s!\n", COUNTRYFILE);
	  (void)UTIL_PopupDialog("Fatal Error", buf,
#endif
#ifdef FRENCH
	  snprintf(buf, sizeof(buf), "CARDS: Ne peut ouvrir %s!\n", COUNTRYFILE);
	  (void)UTIL_PopupDialog("Erreur fatale", buf,
#endif
				 1, "Ok", NULL, NULL);
	  UTIL_ExitProgram(-1);
	}
      
      /* Read the directory */
      fscanf(hFile, "%d%c", &iNumCountries, (char *)&pbBogus);
      if (iNumCountries != NUM_COUNTRIES) /* One is for ocean */
	{
#ifdef ENGLISH
	  (void)UTIL_PopupDialog("Fatal Error", 
				 "CARDS: Wrong number of countries!",
#endif
#ifdef FRENCH
	  (void)UTIL_PopupDialog("Erreur fatale", 
				 "CARDS: Nombre de pays invalide!",
#endif
				 1, "Ok", NULL, NULL);
	  UTIL_ExitProgram(-1);
	}
      fread(pDirectory, iNumCountries, sizeof(pDirectory[0]), hFile);
      
      /* Hack for now */
      iNumCountries = NUM_COUNTRIES;
    }

  /* Allocate the memory for the card */
  pbImage = (Byte *)XtMalloc(pDirectory[iCard].iWidth * 
			     pDirectory[iCard].iHeight);
  pbCompressed = (Byte *)MEM_Alloc(pDirectory[iCard].iLength);
  
  /* Go seek the card */
  fseek(hFile, pDirectory[iCard].lOffset, SEEK_SET);
  
  /* Read it in from the data file */
  fread(pbCompressed, pDirectory[iCard].iLength, 1, hFile);
  
  /* Get the colors of the card.  This is an optimization, 
   * since we should get the color from the compressed card
   * bitmap.  However, we know what color the card should
   * be because of our nice organization of the card ->
   * country mapping.  If the caller has passed in specific values
   * for these, use them instead of the defaults.
   */

  /* do NOT ask me why +1 must be used, but it seems to work */
  iCardColor = (iFgColor == -1) ? COLOR_CountryToColor(iCard + 1) : iFgColor;
  iCardBackground = (iFgColor == -1) ? BlackPixel(hDisplay, 0) : iBgColor;

  /* Uncompress the data */
  for (i=0,pTemp2=pbCompressed,pTemp=pbImage; 
       i<pDirectory[iCard].iLength; i+=2)
    {
      /* Get a color segment */
      bLength = *pTemp2++;
      bColor  = *pTemp2++;
      
      /* Put it in the country image */
      if (bColor==BLACK)
	memset(pTemp, iCardBackground, bLength);
      else
	memset(pTemp, iCardColor, bLength);
      pTemp += bLength; 
    }

  MEM_Free(pbCompressed);

  /* Create the image */
  pimageCountry = XCreateImage(hDisplay, pVisual, 8, ZPixmap, 0, 
			       (char *)pbImage, 
			       pDirectory[iCard].iWidth, 
			       pDirectory[iCard].iHeight, 
			       8, pDirectory[iCard].iWidth);
  return pimageCountry;  
}
