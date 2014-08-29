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
 *   $Id: addPlayer.c,v 1.12 2000/01/23 18:37:44 tony Exp $
 *
 *   $Log: addPlayer.c,v $
 *   Revision 1.12  2000/01/23 18:37:44  tony
 *   removed a debug printf
 *
 *   Revision 1.11  2000/01/15 17:34:12  morphy
 *   Fixes to update color indicator to initial color and after editing color
 *
 *   Revision 1.10  2000/01/10 22:47:39  tony
 *   made colorstuff more private to colormap.c, only scrollbars get set wrong, rest seems to work ok now
 *
 *
 *
 \file "Add player" dialog
 */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/List.h>

#include <stdlib.h>

#include "addPlayer.h"
#include "colorEdit.h"
#include "client.h"
#include "utils.h"
#include "types.h"
#include "gui-vars.h"
#include "riskgame.h"
#include "callbacks.h"
#include "debug.h"


/* Private functions */
void  PLAYER_EditColor(void);
void  PLAYER_SetSpecies(Int32 iSpecies);
void  PLAYER_SpeciesCreated(Int32 iSpecies);
void  PLAYER_SpeciesDestroyed(Int32 iSpecies);
void  PLAYER_RenderSpecies(void);
void  PLAYER_Ok(void);
void  PLAYER_Cancel(void);
void  PLAYER_SelectSpecies(Widget w, XtPointer pData, XtPointer call_data);
Int32 PLAYER_SlotToSpecies(Int32 iSlot);
Int32 PLAYER_SpeciesToSlot(Int32 iSpecies);


/* Globals */
static Flag    fPoppedUp = FALSE;
static String *ppstrStrings = NULL;
static Int32  *piSlotToSpecies = NULL;
static Int32   iCurrentSpecies = SPECIES_HUMAN;
static Int32   iNumSlots = 1;
Int32 r, g, b; /* really static? at least keep global */

/* Action tables */
static XtActionsRec actionTable[] =
{
  { "playerEditColor", (XtActionProc)PLAYER_EditColor },
  { "playerOk",        (XtActionProc)PLAYER_Ok },
  { "playerCancel",    (XtActionProc)PLAYER_Cancel },
  { NULL, NULL }
};

/* Widgets */
static Widget wAddPlayerShell;
static Widget wPlayerNameText;
static Widget wPlayerSpeciesListbox;
static Widget wPlayerDescText;
static Widget wPlayerVersionText;
static Widget wPlayerAuthorText;
static Widget wPlayerDummy1;
static Widget wPlayerDummy2;
static Widget wPlayerDummy3;
static Widget wPlayerDummy4;
static Widget wAddPlayerForm;
static Widget wPlayerNameLabel;
static Widget wPlayerSpeciesLabel;
static Widget wPlayerSpeciesViewport;
static Widget wPlayerAuthorLabel;
static Widget wPlayerVersionLabel;
static Widget wPlayerDescLabel;
static Widget wPlayerColorLabel;
static Widget wPlayerColorDisplay;
static Widget wPlayerOk;
static Widget wPlayerCancel;


/**
 * Pick a color for this player
 \par store in map
 */

void PLAYER_InitColor() {
  r = rand() % 65536;
  g = rand() % 65536;
  b = rand() % 65536;
  while ((r+g+b) < 120000)
  {
    r = r + (rand() % 30000);
    g = g + (rand() % 30000);
    b = b + (rand() % 30000);
  }
  if (r>=65536)
    r = 65535;
  if (g>=65536)
    g = 65535;
  if (b>=65536)
    b = 65535;
  COLOR_StoreColor(COLOR_DieToColor(0), r, g, b);
}


/************************************************************************ 
 *  FUNCTION: PLAYER_BuildDialog
 *  HISTORY: 
 *     01.29.95  ESF  Created.
 *  PURPOSE:
 *  NOTES: 
 ************************************************************************/
void PLAYER_BuildDialog(void)
{
  wAddPlayerShell = XtCreatePopupShell("wAddPlayerShell",
				       transientShellWidgetClass,
				       wToplevel, pVisualArgs, iVisualCount);

  /* The form */
  wAddPlayerForm = XtCreateManagedWidget("wAddPlayerForm", 
					  formWidgetClass, wAddPlayerShell, 
					  NULL, 0);
  
  /* Player name */
  wPlayerNameLabel = XtCreateManagedWidget("wPlayerNameLabel", 
					   labelWidgetClass, 
					   wAddPlayerForm, NULL, 0);
  wPlayerNameText = XtVaCreateManagedWidget("wPlayerNameText", 
					    asciiTextWidgetClass,
					    wAddPlayerForm, 
					    XtNeditType, XawtextEdit,
					    NULL);
  
  /* Player species */
  wPlayerSpeciesLabel = XtCreateManagedWidget("wPlayerSpeciesLabel",
					      labelWidgetClass,
					      wAddPlayerForm, NULL, 0);
  wPlayerSpeciesViewport = XtCreateManagedWidget("wPlayerSpeciesViewport",
						 viewportWidgetClass,
						 wAddPlayerForm, NULL, 0);
  wPlayerSpeciesListbox = XtVaCreateManagedWidget("wPlayerSpeciesListbox",
						  listWidgetClass, 
						  wPlayerSpeciesViewport, 
						  NULL);
  XtAddCallback(wPlayerSpeciesListbox, XtNcallback, PLAYER_SelectSpecies, 
		NULL);

  /* Player description */
  wPlayerDummy1 = XtCreateManagedWidget("wPlayerDummy1", 
					labelWidgetClass, 
					wAddPlayerForm, NULL, 0);
  wPlayerDescLabel = XtCreateManagedWidget("wPlayerDescLabel", 
					   labelWidgetClass, 
					   wAddPlayerForm, NULL, 0);
  wPlayerDescText = XtVaCreateManagedWidget("wPlayerDescText", 
					    asciiTextWidgetClass,
					    wAddPlayerForm, 
					    XtNwrap, XawtextWrapWord,
					    XtNautoFill, True,
					    NULL);
  
  /* Player version */
  wPlayerDummy2 = XtCreateManagedWidget("wPlayerDummy2", 
					labelWidgetClass, 
					wAddPlayerForm, NULL, 0);
  wPlayerVersionLabel = XtCreateManagedWidget("wPlayerVersionLabel", 
					      labelWidgetClass,
					      wAddPlayerForm, NULL, 0);
  wPlayerVersionText = XtCreateManagedWidget("wPlayerVersionText", 
					     asciiTextWidgetClass, 
					     wAddPlayerForm, NULL, 0);

  /* Player author */
  wPlayerAuthorLabel = XtCreateManagedWidget("wPlayerAuthorLabel", 
					     labelWidgetClass,
					     wAddPlayerForm, NULL, 0);
  wPlayerAuthorText = XtCreateManagedWidget("wPlayerAuthorText", 
					    asciiTextWidgetClass, 
					    wAddPlayerForm, NULL, 0);

  /* Player color */
  wPlayerColorLabel = XtCreateManagedWidget("wPlayerColorLabel", 
					    labelWidgetClass, 
					    wAddPlayerForm, NULL, 0);
  wPlayerColorDisplay = XtVaCreateManagedWidget("wPlayerColorDisplay",
						labelWidgetClass,
						wAddPlayerForm, 
						XtNbackground, 
						COLOR_QueryColor(COLOR_DieToColor(0)),
						NULL);

  /* Space */
  wPlayerDummy3 = XtCreateManagedWidget("wPlayerDummy3", formWidgetClass,
					wAddPlayerForm, NULL, 0);

  /* OK and Cancel buttons */
  wPlayerDummy4 = XtCreateManagedWidget("wPlayerDummy4", formWidgetClass,
					wAddPlayerForm, NULL, 0);
  wPlayerOk = XtCreateManagedWidget("wPlayerOk", commandWidgetClass,
				    wAddPlayerForm, NULL, 0);
  XtAddCallback(wPlayerOk, XtNcallback, (XtCallbackProc)PLAYER_Ok, NULL);
  wPlayerCancel = XtCreateManagedWidget("wPlayerCancel", commandWidgetClass,
					wAddPlayerForm, NULL, 0);
  XtAddCallback(wPlayerCancel, XtNcallback, 
		(XtCallbackProc)PLAYER_Cancel, NULL);

  /* Add Actions */
  XtAppAddActions(appContext, actionTable, XtNumber(actionTable));

  /* Init the string table and mapping function */
  ppstrStrings = (String *)MEM_Alloc(sizeof(String)*2);
  ppstrStrings[0] = (String)MEM_Alloc(strlen(
				      RISK_GetNameOfSpecies(SPECIES_HUMAN))+1);
  strcpy(ppstrStrings[0], RISK_GetNameOfSpecies(SPECIES_HUMAN));
  ppstrStrings[1] = (String)NULL;
  piSlotToSpecies = (Int32 *)MEM_Alloc(sizeof(Int32)*1);
  piSlotToSpecies[0] = SPECIES_HUMAN;
}


/************************************************************************ 
 *  FUNCTION: PLAYER_PopupDialog
 *  HISTORY: 
 *     01.29.95  ESF  Created.
 *     02.26.95  ESF  Fixed to generate brighter random colors.  
 *     04.30.95  ESF  Fixed to generate even brighter random colors. 
 *     05.06.95  ESF  Fixed a bug whereas the wrong species was highlighted.
 *     15.01.00  MSH  Fixed to update color indicator to initial color.
 *  PURPOSE: 
 *  NOTES: called from "Register players"
 ************************************************************************/
void PLAYER_PopupDialog(void)
{
    Int32 x, y;
  /* Center the new shell */
  UTIL_CenterShell(wAddPlayerShell, wToplevel, &x, &y);
  XtVaSetValues(wAddPlayerShell, 
		XtNallowShellResize, False,
		XtNx, x, 
		XtNy, y, 
		XtNborderWidth, 1,
		XtNtitle, "Add Player",
		NULL);


  /* TBD: Could make sure here that COLOR_DieToColor(0) had a
   * reasonable color for the new player, i.e. unique, nice...
   * for now pick a random color...but not too dim!!
   */
  PLAYER_InitColor();

  /* Update the color indicator */
  XtVaSetValues(wPlayerColorDisplay, XtNbackground, 
		COLOR_QueryColor(COLOR_DieToColor(0)), NULL);

  /* Do the species stuff */
  PLAYER_RenderSpecies();
  PLAYER_SetSpecies(iCurrentSpecies);
  XawListHighlight(wPlayerSpeciesListbox, 
		   PLAYER_SpeciesToSlot(iCurrentSpecies));
  
  /* Pop the dialog up, set the keyboard focus */
  XtPopup(wAddPlayerShell, XtGrabExclusive); 
  fPoppedUp = TRUE;
  XtSetKeyboardFocus(wToplevel, wPlayerNameText);
  XtSetKeyboardFocus(wAddPlayerShell, wPlayerNameText);
}


/************************************************************************ 
 *  FUNCTION: PLAYER_Ok
 *  HISTORY: 
 *     01.29.94  ESF  Created.
 *     01.31.94  ESF  Delete the strings after registering.
 *     02.05.94  ESF  Remove local registration from here.
 *     02.05.94  ESF  Adding color validity checking.
 *     03.07.94  ESF  Switched to varargs Xt calls.
 *     03.08.94  ESF  Fixed lack of NULL termination in XtVa calls.
 *     05.04.94  ESF  Fixed DistObj changes, added SetNumLivePlayers() 
 *     05.07.94  ESF  Fixed to not let too many players register.
 *     11.16.94  ESF  Fixed to not send info to the server re. new player.
 *     01.15.95  ESF  Fixed to not allocate memory for the empty strings.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_Ok(void)
{
  CString  strPlayerName;
  Int32    iNewPlayer;
  XColor   xColor;
  char buf[256];
  
  /* Get the name */
  XtVaGetValues(wPlayerNameText, XtNstring, &strPlayerName, NULL);

  /* Don't bother doing anything if something's not filled out */
  if (!strlen(strPlayerName))
    return;

  /* See if there are too many players */
  if ((iNewPlayer=CLNT_AllocPlayer(CBK_IncomingMessage)) == -1)
    {
      (void)UTIL_PopupDialog("Error", "Maximum number of players exceeded!", 
			     1, "Ok", NULL, NULL);
      return;
    }

  /* Get the XColor from the color index */
  COLOR_GetColor(COLOR_DieToColor(0), 
		 &xColor.red, &xColor.green, &xColor.blue); 
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", 
	  xColor.red/256, xColor.green/256, xColor.blue/256);

  /* Init. the player */
  RISK_SetColorCStringOfPlayer(iNewPlayer, buf);
  RISK_SetSpeciesOfPlayer(iNewPlayer, iCurrentSpecies);
  RISK_SetNameOfPlayer(iNewPlayer, strPlayerName);

  /* If the client is human, then the client is this one, otherwise
   * it's the AI client that registered it.
   */

  if (iCurrentSpecies == SPECIES_HUMAN)
    RISK_SetClientOfPlayer(iNewPlayer, CLNT_GetThisClientID());
  else
    RISK_SetClientOfPlayer(iNewPlayer, 
			   RISK_GetClientOfSpecies(iCurrentSpecies));

  /* This player is now finished.  Note this */
  RISK_SetAllocationStateOfPlayer(iNewPlayer, ALLOC_COMPLETE);

  /* Make sure the name's erased */
  XtVaSetValues(wPlayerNameText, XtNstring, "", NULL);

  XtPopdown(wAddPlayerShell);
  fPoppedUp = FALSE;

  /* Reset keyboard focus */
  XtSetKeyboardFocus(wToplevel, wToplevel);
}


/************************************************************************ 
 *  FUNCTION: PLAYER_Cancel
 *  HISTORY: 
 *     01.29.95  ESF  Created.
 *     02.12.95  ESF  Finished.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_Cancel(void)
{
  CString strName;

  /* Make sure the player name widget 
   * is empty, popup warning if not.
   */
  
  XtVaGetValues(wPlayerNameText, XtNstring, &strName, NULL);
  if (strlen(strName))
    if (UTIL_PopupDialog("Warning", "Discard current registration data?", 
			 2, "Yes", "No", NULL) == QUERY_NO)
      return;

  XtPopdown(wAddPlayerShell);

  /* Reset keyboard focus */
  XtSetKeyboardFocus(wToplevel, wToplevel);
}


/************************************************************************ 
 *  FUNCTION: PLAYER_EditColor
 *  HISTORY: 
 *     01.29.95  ESF  Created.
 *     15.01.00  MSH  Fixed to update color indicator widget after edit.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_EditColor(void)
{
  /* Relinquish the keyboard focus */
  XtUngrabKeyboard(wPlayerNameText, CurrentTime);

  /* Popup the color editing dialog */
  COLEDIT_EditColor(COLOR_DieToColor(0), FALSE);

  /* Update the color indicator */
  XtVaSetValues(wPlayerColorDisplay, XtNbackground, 
		COLOR_QueryColor(COLOR_DieToColor(0)), NULL);

  /* Regain the keyboard focus */
  while(XtGrabKeyboard(wPlayerNameText, True, GrabModeAsync, GrabModeAsync, 
		       CurrentTime) == GrabNotViewable)
    ; /* TwiddleThumbs() */
}


/************************************************************************ 
 *  FUNCTION: PLAYER_SetSpecies
 *  HISTORY: 
 *     02.12.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_SetSpecies(Int32 iSpecies)
{
  if ((RISK_GetNameOfSpecies(iSpecies)) == NULL)
    {
      printf("Warning: Illegal species (%d)\n", iSpecies);
      return;
    }
  
  XtVaSetValues(wPlayerDescText, XtNstring, 
		RISK_GetDescriptionOfSpecies(iSpecies), NULL);
  XtVaSetValues(wPlayerAuthorText, XtNstring, 
		RISK_GetAuthorOfSpecies(iSpecies), NULL);
  XtVaSetValues(wPlayerVersionText, XtNstring, 
		RISK_GetVersionOfSpecies(iSpecies), NULL);
}


/************************************************************************ 
 *  FUNCTION: PLAYER_Callback
 *  HISTORY: 
 *     02.23.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_Callback(Int32 iMessType, void *pvMess)
{
  /*
   * SpeciesCreated         --> Add species to species list
   * SpeciesDestroyed       --> Delete species from species list
   */

  if (iMessType == MSG_OBJINTUPDATE && 
      ((MsgObjIntUpdate *)pvMess)->iField == SPE_ALLOCATION &&
      ((MsgObjIntUpdate *)pvMess)->iNewValue == ALLOC_COMPLETE)
    {
      /* SpeciesCreated */
      PLAYER_SpeciesCreated(((MsgObjStrUpdate *)pvMess)->iIndex1);
    }
  
  else if (iMessType == MSG_OBJINTUPDATE && 
	   ((MsgObjIntUpdate *)pvMess)->iField == SPE_ALLOCATION &&
	   ((MsgObjIntUpdate *)pvMess)->iNewValue == ALLOC_NONE)
    {
      /* SpeciesDestroyed */
      PLAYER_SpeciesDestroyed(((MsgObjIntUpdate *)pvMess)->iIndex1);
    }
}


#define PAD_LENGTH 34

/************************************************************************ 
 *  FUNCTION: PLAYER_RenderSpecies
 *  HISTORY: 
 *     02.23.95  ESF  Created.
 *     03.03.95  ESF  Finished.
 *     04.01.95  ESF  Fixed a bug, needed parens around iNumSpecies+1.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_RenderSpecies(void)
{
  /* This is just a first approximation of a future, perhaps much 
   * faster or smarter version.  This one does its job, though...
   */

  XawListReturnStruct  *pItem;
  Int32     i, j, k, iLastSelection, iLastSpecies;
  Int32     iNumSpecies = RISK_GetNumSpecies();
  CString  *ppstr = (CString *)MEM_Alloc(sizeof(CString)*(iNumSpecies+1));
  Int32    *piMap = (Int32 *)MEM_Alloc(sizeof(Int32)*iNumSpecies);
  CString   str;

  for (i=j=0; i<RISK_GetNumSpecies(); i++, j++)
    {
      /* Find the next live species */
      while(RISK_GetAllocationStateOfSpecies(j) == FALSE)
	j++; 
      
      str = RISK_GetNameOfSpecies(j);
      D_Assert(str, "Null species.");
      
      /* Add it to the list, with a padding of spaces */
      ppstr[i] = MEM_Alloc(sizeof(Char)*PAD_LENGTH);
      strcpy(ppstr[i], str);
      for(k=strlen(str); k<PAD_LENGTH-1; k++)
	ppstr[i][k] = ' ';
      ppstr[i][k] = '\0';

      /* Add the mapping from slot to species */
      piMap[i] = j;
    }
  ppstr[i] = (String)NULL;

  /* Find out the last selection */
  pItem          = XawListShowCurrent(wPlayerSpeciesListbox);
  iLastSelection = pItem->list_index;
  iLastSpecies   = PLAYER_SlotToSpecies(iLastSelection);
  XtFree((void *)pItem);

  /* Display the list! */
  XtVaSetValues(wPlayerSpeciesListbox, 
		XtNlist, ppstr, 
		XtNnumberStrings, iNumSpecies,
		NULL);

  /* If there were old strings, free all of the memory */
  if (ppstrStrings)
    {
      for (i=0; i!=iNumSlots; i++)
	MEM_Free(ppstrStrings[i]);
      MEM_Free(ppstrStrings);
    }
  if (piSlotToSpecies)
    MEM_Free(piSlotToSpecies);

  /* Keep the new list of strings and the new mapping */
  ppstrStrings     = ppstr;
  piSlotToSpecies  = piMap;
  iNumSlots        = iNumSpecies;

  /* Select the last species if it's still around */
  if ((iLastSpecies = PLAYER_SpeciesToSlot(iLastSpecies)) != -1)
    XawListHighlight(wPlayerSpeciesListbox, iLastSpecies);
  else
    XawListHighlight(wPlayerSpeciesListbox, 
		     MIN(iLastSelection, iNumSpecies-1));
}


/************************************************************************ 
 *  FUNCTION: PLAYER_SpeciesDestroyed
 *  HISTORY: 
 *     02.23.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_SpeciesDestroyed(Int32 iSpecies)
{
  UNUSED(iSpecies);
  if (fPoppedUp == TRUE)
    PLAYER_RenderSpecies();
}


/************************************************************************ 
 *  FUNCTION: PLAYER_SpeciesCreated
 *  HISTORY: 
 *     02.23.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_SpeciesCreated(Int32 iSpecies)
{
  UNUSED(iSpecies);
  if (fPoppedUp == TRUE)
    PLAYER_RenderSpecies();
}


/************************************************************************ 
 *  FUNCTION: PLAYER_SelectSpecies
 *  HISTORY: 
 *     03.03.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void PLAYER_SelectSpecies(Widget w, XtPointer pData, XtPointer call_data)
{
  const XawListReturnStruct  *pItem = 
                                  XawListShowCurrent(wPlayerSpeciesListbox);
  const Int32                 iIndex = pItem->list_index;
  UNUSED(w);
  UNUSED(pData);
  UNUSED(call_data);

  /* Free up memory */
  XtFree((void *)pItem);

  /* If it's off any of the species, then set the one that was set */
  if (iIndex == -1)
    {
      PLAYER_SetSpecies(iCurrentSpecies);
      XawListHighlight(wPlayerSpeciesListbox, 
		       PLAYER_SpeciesToSlot(iCurrentSpecies));
      return;
    }

  /* Find out what species it is */
  iCurrentSpecies = PLAYER_SlotToSpecies(iIndex);

  /* Put in the description, author, etc. */
  PLAYER_SetSpecies(iCurrentSpecies);
}


/************************************************************************ 
 *  FUNCTION: PLAYER_SlotToSpecies
 *  HISTORY: 
 *     03.03.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 PLAYER_SlotToSpecies(Int32 iSlot)
{
  D_Assert(iSlot>=-1, "Wierd slot!");
  D_Assert(piSlotToSpecies, "No mapping!");

  /* This means that the user has clicked off of any slot */
  if (iSlot == -1)
    return iCurrentSpecies;

  return piSlotToSpecies[iSlot];
}


/************************************************************************ 
 *  FUNCTION: PLAYER_SpeciesToSlot
 *  HISTORY: 
 *     03.03.95  ESF  Created.
 *     05.13.95  ESF  Fixed to not return error.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 PLAYER_SpeciesToSlot(Int32 iSpecies)
{
  Int32 i;

  for (i=0; 
       ppstrStrings[i]!=NULL;
       i++)
    {
      if (piSlotToSpecies[i] == iSpecies)
	return i;
    }

  /* The species doesn't exist, return SPECIES_HUMAN */
  iCurrentSpecies = SPECIES_HUMAN;
  return iCurrentSpecies;
}
