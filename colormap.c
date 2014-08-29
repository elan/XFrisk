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
 *   $Id: colormap.c,v 1.18 2000/01/15 21:13:35 morphy Exp $
 *
 *   $Log: colormap.c,v $
 *   Revision 1.18  2000/01/15 21:13:35  morphy
 *   Removed superfluous debug code
 *
 *   Revision 1.17  2000/01/15 11:33:28  morphy
 *   Comments in doxygen format, minor clarity fixes in code
 *
 *   Revision 1.16  2000/01/10 22:47:40  tony
 *   made colorstuff more private to colormap.c, only scrollbars get set wrong, rest seems to work ok now
 *
 *   Revision 1.15  2000/01/09 20:05:02  morphy
 *   Corrections to color map loading - previously struct padding possibility was ignored
 *
 *   Revision 1.14  2000/01/09 19:59:58  morphy
 *   Corrected bit operation brainfart
 *
 *   Revision 1.13  2000/01/09 20:29:44  tony
 *   removed some more comments
 *
 *   Revision 1.12  2000/01/09 19:24:36  morphy
 *   Editorial changes, removed dead code an superfluous temporary variables
 *
 *   Revision 1.11  2000/01/08 18:49:04  tony
 *   oops
 *
 *   Revision 1.10  2000/01/04 21:41:53  tony
 *   removed redundant stuff for jokers
 *
 *   Revision 1.9  2000/01/02 15:59:36  tony
 *   truecolor mapiing fixed, rgbToNum was the one
 *
 *   Revision 1.8  1999/12/26 23:13:24  tony
 *   some comments about colormaps, QueryColor().
 *   some cleaning up in COLOR_GetColormap()
 */

/** \file
 * Color mapping for XFrisk GUI
 *
 * \b See: http://www.hp.com/xwindow/sharedInfo/Whitepapers/Visuals/visuals.html#WHAT_VISUAL
 *
 * \b From: http://www.motifzone.com/resources/man/XAllocStandardColormap.html
 *
 *   The colormap member is the colormap created by the
 *   XCreateColormap function.  The red_max, green_max, and
 *   blue_max members give the maximum red, green, and blue
 *   values, respectively. Each color coefficient ranges from
 *   zero to its max, inclusive. For example, a common colormap
 *   allocation is 3/3/2 (3 planes for red, 3 planes for green,
 *   and 2 planes for blue). This colormap would have red_max =
 *   7, green_max = 7, and blue_max = 3. An alternate allocation
 *   that uses only 216 colors is red_max = 5, green_max = 5, and
 *   blue_max = 5.
 *
 *   The red_mult, green_mult, and blue_mult members give the
 *   scale factors used to compose a full pixel value. (See the
 *   discussion of the base_pixel members for further informa-
 *   tion.) For a 3/3/2 allocation, red_mult might be 32,
 *   green_mult might be 4, and blue_mult might be 1. For a 6-
 *   colors-each allocation, red_mult might be 36, green_mult
 *   might be 6, and blue_mult might be 1.
 *
 *   The base_pixel member gives the base pixel value used to
 *   compose a full pixel value. Usually, the base_pixel is
 *   obtained from a call to the XAllocColorPlanes function.
 *   Given integer red, green, and blue coefficients in their
 *   appropriate ranges, one then can compute a corresponding
 *   pixel value by using the following expression:
 *
 *   (r * red_mult + g * green_mult + b * blue_mult + base_pixel) & 0xFFFFFFFF
 *
 */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "colormap.h"
#include "client.h"
#include "utils.h"
#include "riskgame.h"
#include "gui-vars.h"
#include "debug.h"


/* Private data */

/**
 * Fallback resource filename for use in discovering visual properties
 * \bug Should use a globally defined filename macro
 */
static CString strResources[] = {
#ifdef ENGLISH
#include "english.res"
#endif
#ifdef FRENCH
#include "french.res"
#endif
 NULL
};

/** Color index to country index mapping table */
static Int32   piColorToCountry[MAX_COLORS];

/** Main color map */
static Color   pWorldColors[MAX_COLORS];

/** Flag to indicate if display is true color */
static Flag    trueColor = FALSE;

/** Colormap used in truecolor */
unsigned long  colormap[MAX_COLORS];

/**
 * Country index to color index mapping
 * 42 countries, 1 ocean, 1 lines, 3 dice, x players...
 * They must be long because that's how XAllocColorCells fills it
 */
unsigned long  plCountryToColor[MAX_COLORS];
Colormap       cmapColormap = 0;
Int32          iNumColors;
Int32          COLOR_Depth;

/* Local variables for conversion between rgb and screen values. */
static int   redShift, greenShift, blueShift;
static int   redMove,  greenMove,  blueMove;
static Int32 redMask,  greenMask,  blueMask;


/**
 * Returns number of used colors
 */
Int32 COLOR_GetNumColors(void) {
    return iNumColors;
}


/**
 * Sets number of used colors
 */
void COLOR_SetNumColors(Int32 iNum) {
    iNumColors = iNum;
}


/**
 * Returns color depth
 */
Int32 COLOR_GetDepth(void) {
    return COLOR_Depth;

}


/**
 * Finds and stores RGB shift and move values
 *
 * \b History:
 * \arg 31.01.98 JRXR Created.
 *
 */
void COLOR_InitRGB(XVisualInfo *Info)
{
  Int32 r,g,b;
  int i;
  
  if (Info->class == TrueColor)
    {
      r = redMask   = Info->red_mask;
      g = greenMask = Info->green_mask;
      b = blueMask  = Info->blue_mask;
      
      for ( i=0 ; !(r & 1) ; r=r>>1,i++ )
	;
      redShift=i;

      for ( i=16 ; r ; r=r>>1,i-- )
	;
      redMove=i;

      for ( i=0 ; !(g & 1) ; g=g>>1,i++ )
	;
      greenShift=i;

      for ( i=16 ; g ; g=g>>1,i-- )
	;
      greenMove=i;

      for ( i=0 ; !(b & 1) ; b=b>>1,i++ )
	;
      blueShift=i;

      for ( i=16 ; b ; b=b>>1,i-- )
	;
      blueMove=i;
    }
  else if (Info->class == PseudoColor)
    {
      redMove    = greenMove = blueMove = 8;
      redMask    = 0x0000ff;
      greenMask  = 0x00ff00;
      blueMask   = 0xff0000;
      redShift   = 0;
      greenShift = 8;
      blueShift  = 16;
    }
}


/**
 * Separates RGB components from 32bit representation to separate 16bit variables
 *
 * \param Num 32bit composite RGB representation
 * \param Red Pointer to 16bit variable for red component
 * \param Green Pointer to 16bit variable for green component
 * \param Blue Pointer to 16bit variable for blue component
 *
 * \b History:
 * \arg 31.01.98 JRXR Created.
 * \arg 13.01.00 MSH  Commented.
 */
void COLOR_numTorgb( Int32 Num, UInt16 *Red, UInt16 *Green, UInt16 *Blue )
{
  /* Mask out component and shift it to correct position */
  *Red   = ((Num & redMask)   >> redShift)   << redMove;
  *Green = ((Num & greenMask) >> greenShift) << greenMove;
  *Blue  = ((Num & blueMask)  >> blueShift)  << blueMove;
}


/**
 * Combines given RGB component values to 32bit composite representation.
 *
 * \param Red 16bit red component
 * \param Green 16bit green component
 * \param Blue 16bit blue component
 *
 * \b History:
 * \arg 31.01.98 JRXR Created.
 * \arg 13.01.00 MSH  Commented.
 */
Int32 COLOR_rgbToNum(Int32 Red, Int32 Green, Int32 Blue)
{
  Int32 r,g,b;

  /* Separate significant bits from color components */
  r = Red   >> redMove;
  g = Green >> greenMove;
  b = Blue  >> blueMove;
  /* Sum up the components */
  return (r<<redShift) | (g<<greenShift) | (b<<blueShift);
}


/**
 * Returns flag indicating whether truecolor or pseudocolor is in use.
 *
 * \b History:
 * \arg 07.08.95 JC  Created.
 * \arg 13.01.00 MSH Commented.
 */
Flag COLOR_IsTrueColor()
{
  return(trueColor);
}


/**
 * Maps color index to real color value in truecolor mode. In pseudocolor
 * this is a dummy operation.
 *
 * \b History:
 * \arg 07.08.95  JC  Created.
 * \arg 10.12.99  TdH Made it return realColor when COLOR_Depth==16
 * \arg 09.01.00  MSH Removed dead code, this works just fine without it
 *
 * \bug This must go, it's silly, mapping should have been done already
 */
Int32 COLOR_QueryColor(Int32 iColor)
{
  /* In pseudocolor etc. the index is valid 'as-is' */
  if (!trueColor)
    return(iColor);
  else
    return colormap[iColor];
}


/**
 * Initializes player colors, turn idicator and player color edit entries
 * to black.
 *
 * \b History:
 * \arg 09.08.94  ESF  Created.
 * \arg 01.01.95  ESF  Added making the player turn indicator black.
 * \arg 13.01.00  MSH  Commented.
 *
 * \bug Turn indicator does not need its own colormap entry
 */
void COLOR_Init(void)
{
  Int32 i;

  /* Set the initial player colors to be black */
  for (i=0; i!=MAX_PLAYERS; i++)
    COLOR_StoreNamedColor("Black", i);

  /* Set the initial player turn indicator to be black */
  COLOR_CopyColor(COLOR_PlayerToColor(0), COLOR_DieToColor(2));

  /* Set the initial player color edit color to black */
  COLOR_CopyColor(COLOR_PlayerToColor(0), COLOR_DieToColor(3));
}


/**
 * Selects an appropriate visual if one is available, and then builds
 * the required X args to pass to any top-level shells.  If no
 * appropriate visual is available, then exit.  If a usable PseudoColor 
 * visual exists, but there aren't enough colors available from the
 * default colormap, then allocate a private colormap for the 
 * application.  It allocates a colormap and sets up two mappings,
 * from the index to the colors, and vice-versa.
 *
 * \b History:
 * \arg 09.19.94  ESF  Moved this stuff over here from gui.c.
 * \arg 10.08.94  ESF  Enhanced to reduce flashing with private colormap. 
 * \arg 17.08.95  JC   True colors.
 * \arg 29.08.95  JC   If the server support TrueColor, use it.
 * \arg 13.01.00  MSH  Commented.
 *
 * \bug This function is specific to Frisk and sets global variables
 * cmapColormap, hDisplay, etc. Change it evantually.
 */
void COLOR_GetColormap(void *pData, Int32 *piNumArgs, Int32 iNeededColors,
		       Int32 argc, CString *argv)
{
  Widget       wDummy;
  Arg         *pVisualArgs = (Arg *)pData; 
  XVisualInfo  Info;
  Int32        i;


  /* Create a dummy top level shell to learn more about the display */
  wDummy = XtAppInitialize(&appContext, "XFrisk", NULL, 0, 
			   &argc, argv, strResources, NULL, 0);
  hDisplay = XtDisplay(wDummy);
  trueColor = FALSE;
  COLOR_Depth = 0;

  /* See if there is a TrueColor visual with a depth of 24 bits */
  if (XMatchVisualInfo(hDisplay, DefaultScreen(hDisplay), 
	            	    24, TrueColor, &Info))  {
      trueColor = TRUE;
      COLOR_Depth = 24;
  }

  /* See if there is a TrueColor visual with a depth of 16 bits */
  else if (XMatchVisualInfo(hDisplay, DefaultScreen(hDisplay),
                            16, TrueColor, &Info))  {
      trueColor = TRUE;
      COLOR_Depth = 16;
  }

  /* See if there is a TrueColor visual with a depth of 15 bits */
  else if (XMatchVisualInfo(hDisplay, DefaultScreen(hDisplay), 
                            15, TrueColor, &Info))  {
      trueColor = TRUE;
      COLOR_Depth = 15;
  }
  /* See if there is a PseudoColor visual with a depth of 8 bits */
  else if (XMatchVisualInfo(hDisplay, DefaultScreen(hDisplay), 
                            8, PseudoColor, &Info))  {
      /* Save the visual for use in the program -- this shouldn't be here,
       * once all of this gets cleaned up.
       */

      COLOR_InitRGB(&Info);
      pVisual = Info.visual;
      COLOR_Depth = 8;

      /* Try to allocate the needed colors from the default colormap.
       * If this fails, then try allocating a private colormap.  If
       * it fails, it is probably because the Display is TrueColor, 
       * or else because there aren't enough free colors in the
       * default colormap.
       */

      cmapColormap = DefaultColormap(hDisplay, DefaultScreen(hDisplay));
      if (!XAllocColorCells(hDisplay, cmapColormap, False, NULL, 0, 
			    plCountryToColor, iNeededColors))
	{
 	  XColor    xColor;

	  /* We must use a private colormap */
#ifdef ENGLISH
	  printf("CLIENT: Using a private colormap.\n");
#endif
#ifdef FRENCH
	  printf("CLIENT: Utilise une palette privée.\n");
#endif
	  cmapColormap = XCreateColormap(hDisplay,
					 RootWindowOfScreen(XtScreen(wDummy)),
					 Info.visual, AllocNone);

  	  /* Since we only need some of the colors, copy the first bunch
  	   * of colors from the default colormap, in the hope that we'll
  	   * get the window manager colors, so that nasty flashing won't
  	   * occur...
  	   */

  	  for (i=0; i < 256 - iNeededColors; i++)
  	    {
  	      xColor.pixel = i;
  	      xColor.flags = DoRed | DoGreen | DoBlue;
  	      XQueryColor(hDisplay, 
  			  XDefaultColormap(hDisplay, DefaultScreen(hDisplay)),
  			  &xColor);
  	      XAllocColor(hDisplay, cmapColormap, &xColor);
	    }

	  /* Allocate colors from this colormap */
	  if (!XAllocColorCells(hDisplay, cmapColormap, False, NULL, 0, 
				plCountryToColor, iNeededColors))
	    {
#ifdef ENGLISH
	      printf("CLIENT: Strange error, could not allocate colors.\n");
#endif
#ifdef FRENCH
	      printf("CLIENT: Erreur étrange, pas d'allocation de couleurs.\n");
#endif
	      UTIL_ExitProgram(-1);
            }
	}
    }

  else

    {
      /* Print an error message, deregister the client, and get out! */
#ifdef ENGLISH
      printf("Fatal Error!  Could not find a PseudoColor visual to use,\n"
	     "              or the one found was not deep enough to\n"
	     "              allocate %d colors.\n", iNeededColors);
#endif
#ifdef FRENCH
      printf("Erreur fatale!  Impossible de trouver un PseudoColor visual\n"
	     "                à utiliser ou celui trouvé ne permet pas\n"
	     "                d'utiliser %d couleurs.\n", iNeededColors);
#endif
      UTIL_ExitProgram(0);
    }

  if (trueColor)
  {
      COLOR_InitRGB(&Info);
      cmapColormap = DefaultColormap(hDisplay, DefaultScreen(hDisplay));

      /* to make it work with pseudocolor as well as truecolor
       * in pseudocolor it gets another mapping
       **/
      for(i=0; i < MAX_COLORS; i++)
          plCountryToColor[i] = i;

      /* Set up the arguments */
      *piNumArgs = 0;
      XtSetArg(pVisualArgs[*piNumArgs], XtNvisual, Info.visual); 
      (*piNumArgs)++;
      XtSetArg(pVisualArgs[*piNumArgs], XtNdepth, Info.depth); 
      (*piNumArgs)++;
    }

  else
    {
      /* Set up the arguments */
      *piNumArgs = 0;
      XtSetArg(pVisualArgs[*piNumArgs], XtNvisual, pVisual); 
      (*piNumArgs)++;
      XtSetArg(pVisualArgs[*piNumArgs], XtNdepth, 8); 
      (*piNumArgs)++;  
      XtSetArg(pVisualArgs[*piNumArgs], XtNcolormap, cmapColormap);
      (*piNumArgs)++;
    }

  /* Set up mapping from color to country.  With this set up,
   * we have bidirectional mapping, from country to color and from
   * color to country. 
   */
  memset(piColorToCountry, (Byte)0, sizeof(Int32)*MAX_COLORS);
  for(i=0; i < iNeededColors; i++)
      piColorToCountry[plCountryToColor[i]] = i;

  /* We don't need this anymore */
  XtDestroyWidget(wDummy);
}


/**
 * Reads color map from raw byte array (originally read from file),
 * separating RGB values to structure fields.
 *
 * \b History:
 * \arg 05.12.94  ESF  Created.
 * \arg 09.01.00  MSH  Corrected to take struct padding into account.
 * \arg 13.01.00  MSH  Commented.
 */
void COLOR_SetWorldColormap(Byte *rawmap)
{
  Int32 i;

  for(i = 0; i < iNumColors; i++)
    {
      pWorldColors[i].r = *rawmap++;
      pWorldColors[i].g = *rawmap++;
      pWorldColors[i].b = *rawmap++;
    }
}


/**
 * Sets colormap entries in pseudocolor or redraws map with real
 * colors instead of colormap indexes in truecolor.
 *
 * \b History:
 * \arg 05.12.94  ESF  Created.
 * \arg 08.03.94  ESF  Fixed loop bug.
 * \arg 17.08.95  JC   True colors.
 * \arg 29.08.95  JC   Moved.
 * \arg 02.01.00  TdH  Fixed truecolors in colormap
 */
void COLOR_SetWorldColors(void)
{
  XColor   xColor;
  Int32    i ;
  unsigned long c;


  /* Now read in the colormap, store it, and setup the screen.
   * Note that color == country here.
   * background is #42, lines #43
   */

  for (i=0; i < iNumColors; i++) {
      colormap[i] = COLOR_rgbToNum(pWorldColors[i].r * 256,
                                   pWorldColors[i].g * 256 ,
                                   pWorldColors[i].b * 256);
  }
  if (trueColor) {
      Int32   x, y, i;
      for (y = 0; y < pMapImage->height; y++)  {
          for (x = 0; x < pMapImage->width; x++)  {
              /* this returns *index* of color from the original worldmap */
              c = XGetPixel(pMapImage, x, y);
                /* needed for compatibility with pseudocolor.
                 * it's one on one in truecolor
                 * country that belongs to this pixelvalue */
              i = COLOR_ColorToCountry(c);
              c = COLOR_QueryColor(i);
              XSetForeground(hDisplay, hGC, c);
              XDrawPoint(hDisplay, pixMapImage, hGC, x, y);
          }
      }
      XCopyArea(hDisplay, pixMapImage, hWindow, hGC,
                  0, 0, pMapImage->width, pMapImage->height, 0, 0);
  } else {
      for (i=0; i < iNumColors; i++)
      {
          xColor.flags = DoRed | DoGreen | DoBlue;
          xColor.pixel = COLOR_CountryToColor(i);
          xColor.red   = pWorldColors[i].r << 8;
          xColor.green = pWorldColors[i].g << 8;
          xColor.blue  = pWorldColors[i].b << 8;

          D_Assert(xColor.pixel<=MAX_COLORS, "Pixel out of range.");
          XStoreColor(hDisplay, cmapColormap, &xColor);
      }
  }

  XFlush(hDisplay);
}


/**
 * Maps die index to color index.
 *
 * \bug Should map to real color value instead of just index.
 *
 * \b History:
 * \arg 03.04.94  ESF  Created.
 * \arg 03.05.94  ESF  Fixed bug, wrong offset.
 * \arg 08.28.94  ESF  Fixed bug, wrong argument.
 * \arg 13.01.00  MSH  Commented.
 */
Int32 COLOR_DieToColor(Int32 iDie)
{
  D_Assert(iDie>=0 && iDie<MAX_PLAYERS, 
	   "Wrong range for die color.");
  return (plCountryToColor[NUM_COUNTRIES+2+MAX_PLAYERS+iDie]);
}


/**
 * Maps country index to color index.
 *
 * \bug Should map to real color value instead of just index.
 *
 * \b History:
 * \arg 02.05.94  ESF  Created.
 * \arg 05.05.94  ESF  Fixed for new colormap scheme.
 * \arg 13.01.00  MSH  Commented.
 */
Int32 COLOR_CountryToColor(Int32 iCountry)
{
  /* It's +2 because of the two reserved colors for ocean and lines */
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES+2,
	   "Country out of range.");
  return(plCountryToColor[iCountry]);
}


/**
 * Maps player index to color index.
 *
 * \bug Should map to real color value instead of just index.
 *
 * \b History:
 * \arg 02.05.94  ESF  Created.
 * \arg 13.01.00  MSH  Commented; removed irrelevant bug comment.
 */
Int32 COLOR_PlayerToColor(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS,
	   "Player out of range.");
  return (plCountryToColor[NUM_COUNTRIES+2+iPlayer]);
}


/**
 * Change color of given country to the one of given player.
 *
 * \bug Redrawing country image doesn't belong to this module!
 *
 * \b History:
 * \arg 02.05.94  ESF  Created.
 * \arg 02.05.94  ESF  Factored out color changing code.
 * \arg 13.01.00  MSH  Commented.
 */
void COLOR_ColorCountry(Int32 iCountry, Int32 iPlayer)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES &&
	   iPlayer>=0 && iPlayer<MAX_PLAYERS,
	   "Bad range for ColorCountry().");
  COLOR_CopyColor(COLOR_PlayerToColor(iPlayer), 
		  COLOR_CountryToColor(iCountry));
  if (COLOR_IsTrueColor())
    {
      Int32   x, y, i, j;
      UInt32  c;

      c = COLOR_CountryToColor(iCountry);
      x = RISK_GetTextXOfCountry(iCountry) - 200;
      y = RISK_GetTextYOfCountry(iCountry) - 75;
      XSetForeground(hDisplay, hGC, COLOR_QueryColor(c));
      for (i = 0 ; i < 150 && y+i< pMapImage->height ; i++) {
	if(y+i<0)
	  continue;
	for (j = 0; j < 300 && x+j < pMapImage->width ; j++) {
	  if(x+j<0)
	    continue;
	  if (XGetPixel(pMapImage, x+j, y+i) == c)
	    XDrawPoint(hDisplay, pixMapImage, hGC, x+j, y+i);
	}
      }
      XCopyArea(hDisplay, pixMapImage, hWindow, hGC, 
	        x, y, 300, 150, x, y);
    }
}


/**
 * Maps color index to country index.
 *
 * \b History:
 * \arg 02.05.94  ESF  Created.
 * \arg 05.05.94  ESF  Fixed for new colormap scheme.
 */
Int32 COLOR_ColorToCountry(Int32 iColor)
{
  D_Assert(iColor>=0 && iColor<MAX_COLORS,
	   "Color out of range.");
  return(piColorToCountry[iColor]);
}


/**
 * Copy value in colormap from colormap[src] to colormap[dst]
 *
 * \b History:
 * \arg 02.05.94  ESF  Created.
 * \arg 17.08.95  JC   True colors.
 * \arg 13.01.00  MSH  Fixed comments.
 * \arg 13.01.00  MSH  Fixed logic in truecolor.
 */
void COLOR_CopyColor(Int32 iSrc, Int32 iDst)
{
  XColor  xColor;

  D_Assert(iSrc>=COLOR_CountryToColor(0) && 
	   iSrc<=COLOR_DieToColor(3), "Source color out of range.");
  D_Assert(iDst>=COLOR_CountryToColor(0) && 
	   iDst<=COLOR_DieToColor(3), "Dest. color out of range.");

  if(trueColor)
    {
      colormap[iDst] = colormap[iSrc];
    }
  else
    {
      xColor.flags = DoRed | DoGreen | DoBlue;
      xColor.pixel = iSrc;
      XQueryColor(hDisplay, cmapColormap, &xColor);

      xColor.pixel = iDst;
      XStoreColor(hDisplay, cmapColormap, &xColor);
    }

  XFlush(hDisplay);
}


/**
 * Set color of given player to given color string.
 *
 * \b History:
 * \arg 05.03.94  ESF  Created.
 * \arg 04.06.95  ESF  Fixed bug, store closest displayable color.
 * \arg 17.08.95  JC   true colors.
 */
void COLOR_StoreNamedColor(CString strPlayerColor, Int32 iPlayer) 
{
  XColor xColor, xColorClosest;

  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS,
	   "Player out of range.");
  D_Assert(cmapColormap!=0, "Colormap is not there!");

  /* Get the closest color we can display on this screen */
  XLookupColor(hDisplay, cmapColormap, strPlayerColor, 
	       &xColor, &xColorClosest);

  /* Store it */
  colormap[COLOR_PlayerToColor(iPlayer)] = 
    COLOR_rgbToNum(xColorClosest.red, xColorClosest.green,
		   xColorClosest.blue);

  if (!trueColor)
    {
      xColorClosest.flags = DoRed | DoGreen | DoBlue;
      xColorClosest.pixel = COLOR_PlayerToColor(iPlayer);
      XStoreColor(hDisplay, cmapColormap, &xColorClosest);
    }
}


/**
 * Change color.
 * \param iColorNum Index of color to change
 * \param iRed Red component of new color
 * \param iGreen Green component of new color
 * \param iBlue Blue component of new color
 *
 * \b  History
 * \arg 01.22.95  ESF  Created.
 * \arg 17.08.95  JC   True colors.
 * \arg 13.01.00  MSH  Fixed comments, faulty assertion range
 * \arg 13.01.00  MSH  Fixed function logic on trueColor flag
 */
void COLOR_StoreColor(Int32 iColorNum, Int32 iRed, Int32 iGreen, Int32 iBlue)
{
  XColor xColor;
  Int32  c = COLOR_rgbToNum(iRed, iGreen, iBlue);

  D_Assert(iColorNum && iColorNum <= MAX_COLORS, "Color out of range!");

  if (trueColor)
    {
      colormap[iColorNum] = c;
    }
  else
    {
      xColor.pixel = iColorNum;/* seems wrong!! this range ??*/
      xColor.flags = DoRed | DoGreen | DoBlue;
      xColor.red   = iRed;
      xColor.green = iGreen;
      xColor.blue  = iBlue;
      XStoreColor(hDisplay, cmapColormap, &xColor);
    }
}


/**
 * Returns colormap entry to separate 16bit components
 * \param iColor Colormap index
 * \param iRed Pointer to red component variable
 * \param iGreen Pointer to green component variable
 * \param iBlue Pointer to blue component variable
 *
 * \b History:
 * \arg 02.12.95  ESF  Created.
 * \arg 13.01.00  MSH  Commented; fixed bogus assert range
 */
void COLOR_GetColor(Int32 iColor, UInt16 *iRed, UInt16 *iGreen, UInt16 *iBlue)
{
  D_Assert(iColor && iColor <= MAX_COLORS, "Color out of range!");

  COLOR_numTorgb(colormap[iColor], iRed, iGreen, iBlue);
}

/* EOF */
