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
 *   $Id: colorEdit.c,v 1.21 2000/01/22 22:11:35 morphy Exp $
 *
 *   $Log: colorEdit.c,v $
 *   Revision 1.21  2000/01/22 22:11:35  morphy
 *   Comment changes
 *
 *   Revision 1.20  2000/01/22 22:46:54  tony
 *   now updates example image when color entered in text inputline
 *
 *   Revision 1.19  2000/01/18 18:55:02  morphy
 *   Reordered parameters passed to XawScrollbarSetThumb
 *
 *   Revision 1.18  2000/01/16 18:49:29  tony
 *   scrollbar still screwed
 *
 *   Revision 1.17  2000/01/12 19:33:26  tony
 *   moving around, commenting, scrollbars,edit box still no good
 *
 *   Revision 1.16  2000/01/10 22:47:40  tony
 *   made colorstuff more private to colormap.c, only scrollbars get set wrong, rest seems to work ok now
 *
 *   Revision 1.15  2000/01/10 00:19:56  tony
 *   and one more
 *
 *   Revision 1.14  2000/01/10 00:01:44  tony
 *   and more..
 *
 *   Revision 1.13  2000/01/09 22:44:12  morphy
 *   Fixes and optimizations in example image drawing
 *
 *   Revision 1.12  2000/01/09 21:47:48  morphy
 *   Moved drawing of example country image to separate function
 *
 *   Revision 1.11  2000/01/09 21:20:33  morphy
 *   Merged in fix for example image background color
 *
 *   Revision 1.10  2000/01/09 21:55:10  tony
 *   made iCountry static, little fix in scrollbars
 *
 *   Revision 1.9  2000/01/09 20:58:38  tony
 *   some comment
 *
 *   Revision 1.8  2000/01/05 23:21:35  tony
 *   changed CARD_ to CARDS_
 *
 *   Revision 1.7  2000/01/04 21:41:53  tony
 *   removed redundant stuff for jokers
 */

/** \file Change color of player */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>

#include "colorEdit.h"
#include "dice.h"
#include "callbacks.h"
#include "colormap.h"
#include "client.h"
#include "utils.h"
#include "cards.h"
#include "riskgame.h"
#include "gui-vars.h"
#include "debug.h"
#include "riskgame.h"
#include "viewStats.h"


/* Local functions */
static void COLEDIT_Cancel(void);
static void COLEDIT_ScrollbarMoved(void);
static void COLEDIT_UpdateExampleImage(Int32 iColor);

/* Action tables */
static XtActionsRec actionTable[] =
{
  { "updateColor",      (XtActionProc)COLEDIT_UpdateColorWithInput },
  { "colorEditCancel",  (XtActionProc)COLEDIT_Cancel },
  { NULL, NULL }
};

/* Widgets */
static Widget wColorEditShell;
static Widget wColorEditForm;
static Widget wColorEditLabel;
static Widget wRedScrollbar, wBlueScrollbar, wGreenScrollbar;
static Widget wSampleCountryForm;
static Widget wColorInputLabel, wColorInputText;
static Widget wColorOK, wColorCancel;

/* Color editing related things */
#define COLOR_EDIT_OK           1
#define COLOR_EDIT_CANCEL       0
#define COLOR_EDIT_INPROGRESS  -1

static Int32 iQueryResult = COLOR_EDIT_OK;
static Pixmap pixSampleImage;/**< sample image */
static XImage *pScaledSample;
Int32 iCountry;/**< index of country clicked on*/
Int32 iglobalColor;/**< index of color */


/**
 * \b History:
 * \arg 01.22.95  ESF  Created.
 * \arg 01.24.95  ESF  Changed to get coordinates manually.
 * \arg 01.25.95  ESF  Added check that the country was not empty.
 * \arg 12.07.95  ESF  Fixed a bug -- don't allow editing of ocean.
 */
void COLEDIT_MapShiftClick(void)
{
  Int32   iColor;
  Int32   x, y, xRoot, yRoot;
  UInt32  uiButtons;
  Window  winDummy1, winDummy2;

  if (!XQueryPointer(hDisplay, hWindow, &winDummy1, &winDummy2, 
		    &xRoot, &yRoot, &x, &y, &uiButtons))
    {
      /* Pointer has somehow left window */
      return;
    }

  /* Get the coordinates of the mouse */
  iColor = XGetPixel(pMapImage, x, y);
  iCountry = COLOR_ColorToCountry(iColor);

  /* See if there is a player on the country
   * no idea why?
   */
  if (iCountry<0 || iCountry>=NUM_COUNTRIES ||
      RISK_GetOwnerOfCountry(iCountry) == -1 ||
      RISK_GetNumArmiesOfCountry(iCountry) == 0)
    return;

  /* Edit this color */
  COLEDIT_EditColor(iColor, TRUE);/* ok here */
}

void COLEDIT_Ok(void) { iQueryResult = COLOR_EDIT_OK; }
void COLEDIT_Cancel(void) { iQueryResult = COLOR_EDIT_CANCEL; }


/**
 * Set scrollbars and text input box to given color
 *
 * \b History:
 * \arg 01.22.95  ESF  Created.
 * \arg 18.08.95  JC   Modified to call COLOR_GetColor.
 * \arg 18.01.00  MSH  Reordered X..SetThumb parameters.
 */
void COLEDIT_InitDialogWithColor (Int32 iColorNum)
{
  float  f;
  UInt16 r, g, b;
  char buf[256];

  COLOR_GetColor(iColorNum, &r, &g, &b);

  f = 1.0 - ((float)r)/65535.0;
  XawScrollbarSetThumb(wRedScrollbar, f, 0.05);

  f = 1.0 - ((float)g)/65535.0;
  XawScrollbarSetThumb(wGreenScrollbar, f, 0.05);

  f = 1.0 - ((float)b)/65535.0;
  XawScrollbarSetThumb(wBlueScrollbar, f, 0.05);

  /* Display the name of the color */
  snprintf(buf, sizeof(buf), "#%02x%02x%02x",
	  r/256, g/256, b/256);
  XtVaSetValues(wColorInputText, 
		XtNstring, buf, 
		XtNinsertPosition, strlen(buf),
		NULL);
}


/**
 * Build the X dialog structure. Called from gui.c
 *
 * \b History:
 * \arg 01.21.95  ESF  Created.
 */
 void COLEDIT_BuildDialog(void)
{
    Widget  wColorDummy;

  /* Add Actions */
  XtAppAddActions(appContext, actionTable, XtNumber(actionTable));

  wColorEditShell = XtCreatePopupShell("wColorEditShell", 
				       transientShellWidgetClass,
				       wToplevel, pVisualArgs, iVisualCount);
  wColorEditForm = XtCreateManagedWidget("wColorEditForm", formWidgetClass, 
					 wColorEditShell, NULL, 0);
  /* The label for the dialog */
  wColorEditLabel = XtCreateManagedWidget("wColorEditLabel", labelWidgetClass, 
					  wColorEditForm, NULL, 0);

  /* The three (RGB) scrollbars */
  wRedScrollbar = XtCreateManagedWidget("wRedScrollbar",
					scrollbarWidgetClass,
					wColorEditForm, NULL, 0);
  XtAddCallback(wRedScrollbar, XtNjumpProc, 
		(XtCallbackProc)COLEDIT_ScrollbarMoved, NULL);
  XawScrollbarSetThumb(wRedScrollbar, 0.0, 0.05);
  wGreenScrollbar = XtCreateManagedWidget("wGreenScrollbar", 
					  scrollbarWidgetClass,
					  wColorEditForm, NULL, 0);
  XtAddCallback(wGreenScrollbar, XtNjumpProc, 
		(XtCallbackProc)COLEDIT_ScrollbarMoved, NULL);
  XawScrollbarSetThumb(wGreenScrollbar, 0.0, 0.05);
  wBlueScrollbar = XtCreateManagedWidget("wBlueScrollbar", 
					 scrollbarWidgetClass,
					 wColorEditForm, NULL, 0);
  XtAddCallback(wBlueScrollbar, XtNjumpProc, 
		(XtCallbackProc)COLEDIT_ScrollbarMoved, NULL);
  XawScrollbarSetThumb(wBlueScrollbar, 0.0, 0.05);

  /* A sample country in the chosen color */
  wSampleCountryForm = XtCreateManagedWidget("wSampleCountryForm", 
					     labelWidgetClass,
					     wColorEditForm, NULL, 0);

  /* Input text label */
  wColorInputLabel = XtCreateManagedWidget("wColorInputLabel",
					   labelWidgetClass,
					   wColorEditForm, NULL, 0);

  /* The actual widget for inputting text */
  wColorInputText = XtVaCreateManagedWidget("wColorInputText",
					    asciiTextWidgetClass,
					    wColorEditForm, 
					    XtNeditType, XawtextEdit, NULL);

  /* This is used to align the OK/Cancel buttons */
  wColorDummy = XtCreateManagedWidget("wColorDummy", formWidgetClass,
				      wColorEditForm, NULL, 0);

  /* The buttons */
  wColorOK = XtCreateManagedWidget("wColorOK",
				   commandWidgetClass,
				   wColorEditForm, NULL, 0);
  XtAddCallback(wColorOK, XtNcallback, (XtCallbackProc)COLEDIT_Ok, NULL);  
  wColorCancel = XtCreateManagedWidget("wColorCancel",
				       commandWidgetClass,
				       wColorEditForm, NULL, 0);
  XtAddCallback(wColorCancel, XtNcallback, (XtCallbackProc)COLEDIT_Cancel, 
		NULL);
}


/** Cleanup macro for the function COLEDIT_EditColor */
#define COLEDIT_CloseColorDialog() \
 UTIL_DisplayError(""); \
 XtSetKeyboardFocus(wToplevel, wToplevel); \
 XtRemoveGrab(wColorEditShell); \
 XtUnrealizeWidget(wColorEditShell);


/**
 * Displays the dialog with given color. Called by player add and
 * registration routines.
 *
 * \b History:
 * \arg 01.22.95  ESF  Created.
 * \arg 01.26.95  ESF  Fixed last few bugs.
 * \arg 23.08.95  JC   Use of COLOR_Depth.
 * \arg 12.07.95  ESF  Made the editable color be the dice color.
 */
String COLEDIT_EditColor(Int32 iColorNum, Flag fStoreColor)
{
  XEvent   xEvent;
  Int32    x, y;
  CString  strNewColor=NULL;

  D_Assert(iColorNum >= COLOR_CountryToColor(0) && iColorNum <= COLOR_DieToColor(3),
	   "Asked to edit bogus color!");

  /* Center the new shell */
  UTIL_CenterShell(wColorEditShell, wToplevel, &x, &y);
  XtVaSetValues(wColorEditShell, 
		XtNallowShellResize, False,
		XtNx, x, 
		XtNy, y, 
		XtNborderWidth, 1,
#ifdef ENGLISH
		XtNtitle, "Edit Color",
#endif
#ifdef FRENCH
		XtNtitle, "Edition des couleurs",
#endif
		NULL);

  /* Initialize the scrollbars to represent the right color */
  COLEDIT_InitDialogWithColor(iColorNum);

  /* TdH looks wrong */
  iCountry = COLOR_ColorToCountry(iColorNum);

  /* If the country requested was not a valid country, then 
   * pick a random country for the caller.
   */

  if (iCountry >= NUM_COUNTRIES || iCountry < 0)
    iCountry = rand() % NUM_COUNTRIES;

  /* Hide the dice */
  DICE_Hide();

  /* Put the country on the sample */
  pScaledSample = CARDS_GetCountryImage(iCountry, 
				iColorNum,
				COLOR_CountryToColor(NUM_COUNTRIES));
  
  /* Scale it to the sample country form */
  pScaledSample = CARDS_ScaleImage(pScaledSample, 100, 100);
  
  /* Set the initial color of the sample country
   * TdH: to check!
   */
  COLOR_CopyColor(iColorNum, iColorNum);

  /* Dump it on the sample country location */
  pixSampleImage = XCreatePixmap(hDisplay, pixMapImage, 140, 140, COLOR_GetDepth());
  XSetForeground(hDisplay, hGC, COLOR_QueryColor(COLOR_CountryToColor(NUM_COUNTRIES)));
  XFillRectangle(hDisplay, pixSampleImage, hGC, 0, 0, 140, 140);
  XtVaSetValues(wSampleCountryForm, XtNbackground, 
		COLOR_QueryColor(COLOR_CountryToColor(NUM_COUNTRIES)), NULL);
  COLEDIT_UpdateExampleImage(iColorNum);

  /* Popup the color editing dialog */
  XtMapWidget(wColorEditShell);
  XtAddGrab(wColorEditShell, True, True);
  XtSetKeyboardFocus(wToplevel, wColorEditShell); 
  XtSetKeyboardFocus(wColorEditShell, wColorInputText);

  /* Look until user presses one of the buttons */
  iQueryResult = COLOR_EDIT_INPROGRESS;/* TdH: why this read?? */
  while (iQueryResult == COLOR_EDIT_INPROGRESS) 
    {
      /* pass events */
      XNextEvent(hDisplay, &xEvent);
      XtDispatchEvent(&xEvent);
    }

  if (iQueryResult == COLOR_EDIT_OK)
    {
      Int32   i;
      
      /* Just in case the player typed a color name in and then hit "Ok"
       * update the color in the country in the last second that the dialog
       * is up, so that when we copy the color out of there it will be the
       * color that the player typed in.
       */

      COLEDIT_UpdateColorWithInput();

      /* Get the color from the text widget.  If the player moved the
       * scrollbars, then the color will have been written in here.
       * If the player typed something in, it will be in here.
       */

      XtVaGetValues(wColorInputText, XtNstring, &strNewColor, NULL);

      /* Are we trying to store the color? */
      if (fStoreColor) {
	  /* Sanity check */
	  if (iCountry < 0 || iCountry >= NUM_COUNTRIES ||
	      RISK_GetOwnerOfCountry(iCountry) == -1 ||
	      RISK_GetNumArmiesOfCountry(iCountry) <= 0)  {
	      (void)UTIL_PopupDialog("Error", 
				     "Cannot set new value for color!", 
				     1, "Ok", NULL, NULL);
          } else {
              Int32 iOwner = RISK_GetOwnerOfCountry(iCountry);
              Flag isTrueColor = COLOR_IsTrueColor();

              /* And also the actual color for the player */
              COLOR_StoreNamedColor(strNewColor, iOwner);

              /* Set all the countries that the player owns to be new color */
              for (i=0; i!=NUM_COUNTRIES; i++)
                  if (RISK_GetOwnerOfCountry(i) == iOwner) {
                      COLOR_CopyColor(COLOR_DieToColor(0),
                                      COLOR_CountryToColor(i));

                      if (isTrueColor) {
                          COLOR_ColorCountry(i, iOwner);
                          UTIL_DarkenCountry(i);
                      }
                  }
	      if (isTrueColor)
                  STAT_RenderSlot(STAT_PlayerToSlot(iOwner), iOwner);
	  
              /* If the player whose color changed is the current
               * player, then recolor the current player indicator.
               * Also, the dice really need to change color too
               * (thanks, Kirsten, for reminding me about this -- who
               * says English majors can't program?)  For now punt on
               * the dice.
               */

              if (RISK_GetOwnerOfCountry(iCountry) == iCurrentPlayer)
                  COLOR_CopyColor(COLOR_DieToColor(0), COLOR_DieToColor(2));
          }
      }
    }

  XDestroyImage(pScaledSample);
  
  COLEDIT_CloseColorDialog();
  return strNewColor;
}


/**
 * Updates color value and text box based on scrollbar movement.
 * Called during X event handling from the scrollbar widget.
 *
 * \b History:
 * \arg 01.22.95  ESF  Created.
 */
void COLEDIT_ScrollbarMoved(void)
{
  float   r, g, b;
  XColor  xColor;
  char buf[256];

  XtVaGetValues(wRedScrollbar, XtNtopOfThumb, &r, NULL);
  XtVaGetValues(wGreenScrollbar, XtNtopOfThumb, &g, NULL);
  XtVaGetValues(wBlueScrollbar, XtNtopOfThumb, &b, NULL);

  /* Setup the new color */
  xColor.red   = (Int32)((1.0-r)*65535.0);
  xColor.green = (Int32)((1.0-g)*65535.0);
  xColor.blue  = (Int32)((1.0-b)*65535.0);

  /* Color the sample country with the new color
   */
  COLOR_StoreColor(COLOR_DieToColor(0), xColor.red, xColor.green, xColor.blue);
  COLEDIT_UpdateExampleImage(COLOR_DieToColor(0));
  XFlush(hDisplay);

  /* Display the name of the color */
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", 
	  xColor.red/256, xColor.green/256, xColor.blue/256);
  XtVaSetValues(wColorInputText, 
		XtNstring, buf, 
		XtNinsertPosition, strlen(buf),
		NULL);
}


/**
 * Parse value from input box, update scrollbars and example
 * country image to reflect change. Uses XParseColor to
 * handle named colors listed in rgb.txt
 *
 * \b History:
 * \arg 01.23.95  ESF  Created.
 */
void COLEDIT_UpdateColorWithInput(void)
{
  CString  strColor;
  float    r, g, b;
  XColor   xColor1, xColor2;
  char buf[256];

  /* Get the color from the text widget */
  XtVaGetValues(wColorInputText, XtNstring, &strColor, NULL);

  /* Also get the color from the scrollbars. */
  XtVaGetValues(wRedScrollbar, XtNtopOfThumb, &r, NULL);
  XtVaGetValues(wGreenScrollbar, XtNtopOfThumb, &g, NULL);
  XtVaGetValues(wBlueScrollbar, XtNtopOfThumb, &b, NULL);
  snprintf(buf, sizeof(buf), "#%02x%02x%02x",
	  (Int32)((1.0-r)*65535.0)/256, 
	  (Int32)((1.0-g)*65535.0)/256, 
	  (Int32)((1.0-b)*65535.0)/256);

  /* Adjust for the screen */
  xColor1.flags = DoRed | DoGreen | DoBlue;
  (void)XParseColor(hDisplay, cmapColormap, buf, &xColor1);


  /* Is it a valid color? */
  if (XParseColor(hDisplay, cmapColormap, strColor, &xColor2))
    {
      /* If they match, then take this to mean the same as the "Ok" button. */
        if (xColor1.red   == xColor2.red &&
            xColor1.green == xColor2.green &&
	  xColor1.blue  == xColor2.blue)  {
	  COLEDIT_Ok();
        }   else   {
            COLOR_StoreColor(COLOR_DieToColor(0),
                             xColor2.red, xColor2.green, xColor2.blue);
            COLEDIT_InitDialogWithColor(COLOR_DieToColor(0));
            COLEDIT_UpdateExampleImage(COLOR_DieToColor(0));
        }
    }  else
#ifdef ENGLISH
    (void)UTIL_PopupDialog("Error", "That color does not exist!", 
			   1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
    (void)UTIL_PopupDialog("Erreur", "Cette couleur n'existe pas!", 
			   1, "Ok", NULL, NULL);
#endif
}


/**
 * Draw example country image with given color.
 *
 * \b History:
 * \tag 09.01.00  MSH  Created.
 */
static void COLEDIT_UpdateExampleImage(Int32 iColor)
{
  Int32    x, y;

  if (COLOR_IsTrueColor())
    {
      XSetForeground(hDisplay, hGC, COLOR_QueryColor(iColor));
      for (y = 0; y < pScaledSample->height; y++)
	for (x = 0; x < pScaledSample->width; x++)
	  {
	    if (XGetPixel(pScaledSample, x, y) == NUM_COUNTRIES)
	      continue;
	    XDrawPoint(hDisplay, pixSampleImage, hGC,
		       (140-pScaledSample->width)/2 + x,
		       (140-pScaledSample->height)/2 + y);
	  }
    }
  else
    {
      XPutImage(hDisplay, pixSampleImage, hGC, pScaledSample, 0, 0, 
	        (140-pScaledSample->width)/2, (140-pScaledSample->height)/2, 140, 140);
    }

  XtVaSetValues(wSampleCountryForm, XtNbitmap, pixSampleImage, NULL); 
}

/* EOF */
