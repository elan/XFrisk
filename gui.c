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
 *   $Id: gui.c,v 1.12 2000/01/10 22:47:40 tony Exp $
 *
 *   $Log: gui.c,v $
 *   Revision 1.12  2000/01/10 22:47:40  tony
 *   made colorstuff more private to colormap.c, only scrollbars get set wrong, rest seems to work ok now
 *
 *   Revision 1.11  2000/01/09 20:05:02  morphy
 *   Corrections to color map loading - previously struct padding possibility was ignored
 *
 *   Revision 1.10  2000/01/09 19:17:43  morphy
 *   Added Log tag in comment header
 *
 */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/Form.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Scrollbar.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "gui-vars.h"
#include "gui-func.h"
#include "callbacks.h"
#include "dice.h"
#include "cards.h"
#include "colormap.h"
#include "utils.h"
#include "debug.h"
#include "types.h"
#include "version.h"
#include "colorEdit.h"
#include "registerPlayers.h"
#include "addPlayer.h"
#include "viewCards.h"
#include "viewStats.h"
#include "viewFeedback.h"
#include "viewLog.h"
#include "viewStats.h"
#include "viewChat.h"


/* Main window widgets */
static Widget wMap;
static Widget wCurrentPlayer;
Widget wToplevel;
Widget wDiceBox, wMsgDestList, wErrorLabel;
Widget wActionList, wAttackList;
Widget wSendMsgText;
Widget wMsgText, wCommentLabel;

/* Main stuff */
XtAppContext  appContext;


/* The view cards popup shell */
Widget wCardShell;
Widget wCardToggle[MAX_CARDS];

/* The "place armies" popup shell */
Widget wArmiesShell, wArmiesText, wCancelArmiesButton;

/* The "help" popup shell */
Widget wHelpShell;
Widget wHelpTopicList, wHelpLabel, wHelpText;

/* The "generic popup" dialog */
Widget wDialogLabel, wDialogButton[3];
Widget wDialogShell;

/* Action tables */
static XtActionsRec actionTable[] =
{
  { "popupArmies",      (XtActionProc)UTIL_QueryYes },
  { "mapClick",         (XtActionProc)CBK_MapClick },
  { "mapShiftClick",    (XtActionProc)COLEDIT_MapShiftClick },
  { "sendMessage",      (XtActionProc)CBK_SendMessage },
  { NULL, NULL }
};

#ifdef ENGLISH
static char *pstrAttackDice[] = 
{
  "1 die", 
  "2 dice",
  "3 dice",
  "Auto",
};
#endif
#ifdef FRENCH
static char *pstrAttackDice[] = 
{
  "1 dé", 
  "2 dés",
  "3 dés",
  "Auto",
};
#endif

#ifdef ENGLISH
static char *pstrActions[] = 
{
  "Place", 
  "Attack", 
  "Do or die",
  "move",
};
#endif
#ifdef FRENCH
static char *pstrActions[] = 
{
  "Placer", 
  "Attaquer", 
  "À mort",
  "Déplacer",
};
#endif

/* Private functions */
static Int32 GUI_GetNumColorsInMap(CString strMapFile);

Display        *hDisplay;
Window          hWindow;
GC              hGC, hGC_XOR;
Int32           iScreen;
XFontStruct    *pFont;
Pixmap          pixMapImage;
XImage         *pMapImage;
Visual         *pVisual;

/* These are used to set up the visual in all shell widgets */
Int32           iVisualCount;
Arg             pVisualArgs[3];

/************************************************************************ 
 *  FUNCTION: GUI_Setup
 *  HISTORY: 
 *     01.25.94  ESF  Created.
 *     02.19.94  ESF  Added Card UI stuff.
 *     03.03.94  ESF  Added callbacks for the remaining buttons.
 *     03.03.94  ESF  Changed so `Number of Armies?' starts unmanaged.
 *     03.04.94  ESF  Added another comment widget for error messages.
 *     03.06.94  ESF  Added an army placement shell.
 *     03.14.94  ESF  Changed shells to be transient.
 *     04.01.94  ESF  Added help popup.
 *     04.02.94  ESF  Added generic dialog with three possible answers.
 *     05.19.94  ESF  Added TrueColor display patches.
 *     09.14.94  ESF  Fixed so that it works with more TrueColor visuals.
 *     01.24.95  ESF  Changed so that actions are added in one place.
 *     01.24.95  ESF  Changed so that accelerators are done in res. file.
 *     01.25.95  ESF  Added dummy widget for centering buttons in dialog.
 *     19.12.99  TdH  Moved call to GUI_LoadMap(MAPFILE) here
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GUI_Setup(Int32 argc, CString *argv)
{
  Int32         iCount, i, iVisualCount;
  Arg           pArgs[10], pVisualArgs[3];
  char buf[256];

  Widget wForm, wControls;
  Widget wStatViewButton, wAboutButton;
  Widget wAttackLabel;
  Widget wActionLabel;
  Widget wMsgDestLabel, wMsgDestViewport;
  Widget wPlayField;
  Widget wQuitButton, wRepeatButton, wCancelAttackButton;
  Widget wHelpButton, wShowCardsButton, wShowMissionButton;
  Widget wEndTurnButton;

  /* The view cards popup shell */
  Widget wCardForm, wCardTableBox;
  Widget wCardViewport;
  Widget wExchangeButton, wCancelCardsButton;

  /* The "place armies" popup shell */
  Widget wArmiesForm, wArmiesLabel;
  Widget wFinishArmiesButton;

  /* The "help" popup shell */
  Widget wHelpForm, wHelpTopicViewport; 
  Widget wHelpTopicLabel, wHelpOkButton;

  /* The "generic popup" dialog */
  Widget wDialogForm;

  /* Setup a colormap for the application */
  COLOR_GetColormap(pVisualArgs, &iVisualCount, 
		    GUI_GetNumColorsInMap(MAPFILE) + 
		    MAX_PLAYERS + NUM_OTHERCOLORS, argc, argv);

  /* Create the application's main window */
  wToplevel = XtAppCreateShell(NULL, "XFrisk", applicationShellWidgetClass,
			       hDisplay, pVisualArgs, iVisualCount);

  /* Make sure to set the arguments from the command line */
  XtVaSetValues(wToplevel, 
		XtNargc, argc,
 		XtNargv, argv,
 		NULL); 

  /* Add the action table */
  XtAppAddActions(appContext, actionTable, XtNumber(actionTable));

  /* The main window UI stuff */
  wForm = XtCreateManagedWidget("wForm", formWidgetClass, wToplevel, 
				NULL, 0);
  wMap = XtCreateManagedWidget("wMap", formWidgetClass, wForm,
			       NULL, 0);
  wPlayField = XtCreateManagedWidget("wPlayField", formWidgetClass, wForm, 
                                    NULL, 0);
  wControls = XtCreateManagedWidget("wControls", formWidgetClass, wForm, 
                                    NULL, 0);

  /* Holds the number of attack dice to use (1, 2, or 3) */
  wAttackLabel = XtCreateManagedWidget("wAttackLabel", labelWidgetClass, 
				       wControls, NULL, 0);
  iCount=0;
  XtSetArg(pArgs[iCount], XtNlist, pstrAttackDice); iCount++;
  XtSetArg(pArgs[iCount], XtNnumberStrings, 4); iCount++;
  wAttackList = XtCreateManagedWidget("wAttackList", listWidgetClass, 
				      wControls, pArgs, iCount);
  XtAddCallback(wAttackList, XtNcallback, CBK_Attack, NULL); 
  
  /* Holds the action to perform (Place Armies, Attack, Free Move) */
  wActionLabel = XtCreateManagedWidget("wActionLabel", labelWidgetClass, 
				       wControls, NULL, 0);
  iCount=0;
  XtSetArg(pArgs[iCount], XtNlist, pstrActions); iCount++;
  XtSetArg(pArgs[iCount], XtNnumberStrings, 4); iCount++;
  wActionList = XtCreateManagedWidget("wActionList", listWidgetClass, 
				      wControls, pArgs, iCount);
  XtAddCallback(wActionList, XtNcallback, CBK_Action, NULL); 

  /* Holds the destination of a message (All, [players...]) */
  wMsgDestLabel = XtCreateManagedWidget("wMsgDestLabel", labelWidgetClass, 
					wControls, NULL, 0);
  wMsgDestViewport = XtCreateManagedWidget("wMsgDestViewport", 
					   viewportWidgetClass, wControls, 
					   NULL, 0);

#ifdef ENGLISH
  pstrMsgDstCString[0] = "All Players";
#endif
#ifdef FRENCH
  pstrMsgDstCString[0] = "Tous les joueurs";
#endif
  wMsgDestList = XtVaCreateManagedWidget("wMsgDestList", listWidgetClass,
					 wMsgDestViewport, 
					 XtNlist, pstrMsgDstCString,
					 XtNnumberStrings, 1, 
					 NULL);
  XtAddCallback(wMsgDestList, XtNcallback, CBK_MsgDest, NULL); 

  /* Holds the message to send */
  wSendMsgText = XtVaCreateManagedWidget("wSendMsgText", asciiTextWidgetClass,
					 wControls, 
					 XtNeditType, XawtextEdit,
					 XtNwrap, XawtextWrapNever,
					 NULL);

  /* Holds all of the messages */
  wMsgText = XtVaCreateManagedWidget("wMsgText", asciiTextWidgetClass, 
				     wControls, 
				     XtNstring, "",
				     XtNscrollVertical, XawtextScrollAlways, 
				     XtNwrap, XawtextWrapWord,
				     XtNautoFill, True,
				     NULL);

  /* Turns the color of the player who's turn it is */
  wCurrentPlayer = XtCreateManagedWidget("wCurrentPlayer", formWidgetClass, 
					 wPlayField, NULL, 0);
  
  /* Running commentary on the game, one liners, rules */
  wCommentLabel = XtCreateManagedWidget("wCommentLabel", labelWidgetClass, 
					wPlayField, NULL, 0);

  /* Tells about the game version */
  wAboutButton = XtCreateManagedWidget("wAboutButton", 
				       commandWidgetClass, wPlayField, 
				       NULL, 0);
  XtAddCallback(wAboutButton, XtNcallback, CBK_About, NULL); 

  /* Cancel an attack */
  wCancelAttackButton = XtCreateManagedWidget("wCancelAttackButton", 
					      commandWidgetClass, wPlayField, 
					      NULL, 0);
  XtAddCallback(wCancelAttackButton, XtNcallback, CBK_CancelAttack, NULL); 

  /* Repeat a single attack */
  wRepeatButton = XtCreateManagedWidget("wRepeatButton", commandWidgetClass, 
					wPlayField, NULL, 0);
  XtAddCallback(wRepeatButton, XtNcallback, CBK_Repeat, NULL); 

  /* End a turn */
  wEndTurnButton = XtCreateManagedWidget("wEndTurnButton", commandWidgetClass, 
					 wPlayField, NULL, 0);
  XtAddCallback(wEndTurnButton, XtNcallback, CBK_EndTurn, NULL);

  /* Show the cards */
  wShowCardsButton = XtCreateManagedWidget("wShowCardsButton", 
					   commandWidgetClass, wPlayField, 
					   NULL, 0);
  XtAddCallback(wShowCardsButton, XtNcallback, CBK_ShowCards, NULL);

  /* Show the mission */
  wShowMissionButton = XtCreateManagedWidget("wShowMissionButton", 
					     commandWidgetClass, wPlayField, 
					     NULL, 0);
  XtAddCallback(wShowMissionButton, XtNcallback, CBK_ShowMission, NULL);

  /* Statistics View */
  wStatViewButton = XtCreateManagedWidget("wStatViewButton", 
					  commandWidgetClass, wPlayField, 
					  NULL, 0);
  XtAddCallback(wStatViewButton, XtNcallback, 
		(XtCallbackProc)STAT_PopupDialog, NULL);

  /* Help */
  wHelpButton = XtCreateManagedWidget("wHelpButton", commandWidgetClass, 
				      wPlayField, NULL, 0);
  XtAddCallback(wHelpButton, XtNcallback, CBK_Help, NULL); 

  /* The Quit button */
  wQuitButton = XtCreateManagedWidget("wQuitButton", commandWidgetClass, 
					 wPlayField, NULL, 0);
  XtAddCallback(wQuitButton, XtNcallback, CBK_Quit, NULL);  

  /* Error messages */
  wErrorLabel = XtCreateManagedWidget("wErrorLabel", labelWidgetClass, 
				      wPlayField, NULL, 0);

  /* Holds the rolled dice bitmaps */
  wDiceBox = XtCreateManagedWidget("wDiceBox", formWidgetClass, 
				   wPlayField, NULL, 0);

  /* Finished */
  XtRealizeWidget(wToplevel);

  /* Build the card popup shell */
  wCardShell = XtCreatePopupShell("Cards", transientShellWidgetClass,
				  wToplevel, pVisualArgs, iVisualCount);
  wCardForm = XtCreateManagedWidget("wCardForm", formWidgetClass, 
				    wCardShell, NULL, 0);
  wCardViewport = XtCreateManagedWidget("wCardViewport", viewportWidgetClass, 
					wCardForm, NULL, 0);
  wCardTableBox = XtCreateManagedWidget("wCardTableBox", boxWidgetClass, 
					 wCardViewport, NULL, 0);
  
  for (i=0; i!=MAX_CARDS; i++)
    {
      snprintf(buf, sizeof(buf), "wCardToggle%d", i);
      wCardToggle[i] = XtVaCreateWidget(buf, toggleWidgetClass,
					wCardTableBox, 
					XtNwidth, CARD_WIDTH,
					XtNheight, CARD_HEIGHT,
					XtNborderWidth, 0,
					XtNhighlightThickness, 0,
					XtNlabel, "",
					NULL);
    }
  
  wExchangeButton = XtCreateManagedWidget("wExchangeButton", 
					  commandWidgetClass, wCardForm, 
					  NULL, 0);
  XtAddCallback(wExchangeButton, XtNcallback, CBK_ExchangeCards, NULL); 
  wCancelCardsButton = XtCreateManagedWidget("wCancelCardsButton", 
					     commandWidgetClass, wCardForm, 
					     NULL, 0);
  XtAddCallback(wCancelCardsButton, XtNcallback, CBK_CancelCards, NULL);

  /* Build the army placement shell */
  wArmiesShell = XtCreatePopupShell("Armies", transientShellWidgetClass,
				      wToplevel, pVisualArgs, iVisualCount);
  wArmiesForm = XtCreateManagedWidget("wArmiesForm", formWidgetClass, 
				      wArmiesShell, NULL, 0);
  wArmiesLabel = XtCreateManagedWidget("wArmiesLabel", labelWidgetClass, 
				       wArmiesForm, NULL, 0);
  wArmiesText = XtVaCreateManagedWidget("wArmiesText", asciiTextWidgetClass, 
					wArmiesForm, XtNeditType, XawtextEdit,
					NULL);
  wFinishArmiesButton = XtCreateManagedWidget("wFinishArmiesButton", 
					      commandWidgetClass, wArmiesForm, 
					      NULL, 0);
  XtAddCallback(wFinishArmiesButton, XtNcallback, 
		(XtCallbackProc)UTIL_QueryYes, NULL);

  wCancelArmiesButton = XtCreateManagedWidget("wCancelArmiesButton", 
					      commandWidgetClass, wArmiesForm, 
					      NULL, 0);
  XtAddCallback(wCancelArmiesButton, XtNcallback, 
		(XtCallbackProc)UTIL_QueryNo, NULL);

  /* Build the help shell */
  wHelpShell = XtCreatePopupShell("Help", transientShellWidgetClass,
				    wToplevel, pVisualArgs, iVisualCount);
  wHelpForm = XtCreateManagedWidget("wHelpForm", formWidgetClass, 
				    wHelpShell, NULL, 0);
  wHelpTopicLabel = XtCreateManagedWidget("wHelpTopicLabel", labelWidgetClass, 
					  wHelpForm, NULL, 0);
  wHelpLabel = XtCreateManagedWidget("wHelpLabel", labelWidgetClass, 
				     wHelpForm, NULL, 0);
  wHelpTopicViewport = XtCreateManagedWidget("wHelpTopicViewport", 
					     viewportWidgetClass, 
					     wHelpForm, NULL, 0);
  wHelpTopicList = XtCreateManagedWidget("wHelpTopicList", listWidgetClass, 
					 wHelpTopicViewport, NULL, 0);
  XtAddCallback(wHelpTopicList, XtNcallback, CBK_HelpSelectTopic, NULL); 
  wHelpText = XtVaCreateManagedWidget("wHelpText", asciiTextWidgetClass, 
				      wHelpForm,
				      XtNwrap, XawtextWrapWord,
				      XtNdisplayCaret, False,
				      XtNscrollVertical, XawtextScrollAlways,
				      NULL);
  wHelpOkButton = XtCreateManagedWidget("wHelpOkButton", 
					commandWidgetClass, wHelpForm, 
					NULL, 0);
  XtAddCallback(wHelpOkButton, XtNcallback, CBK_HelpOk, NULL);

  /* Build the generic dialog shells */
  wDialogShell = XtCreatePopupShell("Dialog", transientShellWidgetClass,
				      wToplevel, pVisualArgs, iVisualCount);
  wDialogForm = XtCreateManagedWidget("wDialogForm", formWidgetClass, 
				      wDialogShell, NULL, 0);
  wDialogLabel = XtCreateManagedWidget("wDialogLabel", labelWidgetClass, 
				       wDialogForm, NULL, 0);
  wDialogButton[0] = XtCreateManagedWidget("wDialogButton1", 
					   commandWidgetClass, wDialogForm, 
					   NULL, 0);
  XtAddCallback(wDialogButton[0], XtNcallback, (XtCallbackProc)UTIL_QueryYes, 
		NULL);
  wDialogButton[1] = XtCreateManagedWidget("wDialogButton2", 
					   commandWidgetClass, wDialogForm, 
					   NULL, 0);
  XtAddCallback(wDialogButton[1], XtNcallback, (XtCallbackProc)UTIL_QueryNo, 
		NULL);
  wDialogButton[2] = XtCreateManagedWidget("wDialogButton3", 
					   commandWidgetClass, wDialogForm, 
					   NULL, 0);
  XtAddCallback(wDialogButton[2], XtNcallback, 
		(XtCallbackProc)UTIL_QueryCancel, NULL);

  /* Get a couple globals for the future */
  hDisplay = XtDisplay(wMap);
  hWindow  = XtWindow(wMap);
  iScreen  = DefaultScreen(hDisplay);
  hGC      = XCreateGC(hDisplay, hWindow, 0, NULL);
  hGC_XOR  = XCreateGC(hDisplay, hWindow, 0, NULL);  

  /* Set the XOR GC appropriately */
  XSetFunction(hDisplay, hGC_XOR, GXinvert); 
  XSetLineAttributes(hDisplay, hGC_XOR, 2, LineSolid, CapButt, JoinMiter);

  /* Get the font */
  if ((pFont=XLoadQueryFont(hDisplay, "fixed")) == NULL)
    {
#ifdef ENGLISH
      (void)UTIL_PopupDialog("Fatal Error", 
			     "GUI: Couldn't get font 'fixed'!\n", 1,
#endif
#ifdef FRENCH
      (void)UTIL_PopupDialog("Erreur fatale", 
			     "GUI: Impossible de charger la fonte'fixed'!\n", 1,
#endif
			     "Ok", NULL, NULL);
      UTIL_ExitProgram(-1);
    }

  /* Build the object-like MVC View dialogs */
  COLEDIT_BuildDialog();
  REG_BuildDialog();
  PLAYER_BuildDialog();
  CARD_BuildDialog();
  CHAT_BuildDialog();
  STAT_BuildDialog();
  LOG_BuildDialog();
  FDBK_BuildDialog();

  /* Print a welcome message */
#ifdef ENGLISH
  snprintf(buf, sizeof(buf), "Welcome to Frisk %s!", VERSION);
#endif
#ifdef FRENCH
  snprintf(buf, sizeof(buf), "Bienvenu à Frisk %s!", VERSION);
#endif
  UTIL_DisplayComment(buf);
  GUI_LoadMap(MAPFILE);
}


/************************************************************************ 
 *  FUNCTION: GUI_Start
 *  HISTORY: 
 *     01.25.94  ESF  Created.
 *     01.29.94  ESF  Added Registration popup.
 *     03.16.94  ESF  Centering and automatic placement of wRegisterShell.
 *     04.01.94  ESF  Fixed bug, XtNy had an x by it.
 *     01.29.95  ESF  Cleaned up, almost nothing left.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GUI_Start(void)
{
    XtAppMainLoop(appContext);
}



/************************************************************************ 
 *  FUNCTION: GUI_AddCallbacks
 *  HISTORY: 
 *     01.25.94  ESF  Created 
 *     04.11.94  ESF  Added event handler for dice region.
 *     01.24.95  ESF  Added event handler for keyboard.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GUI_AddCallbacks(Int32 iReadSock)
{
  /* Add the input to receive messages from */
  (void)XtAppAddInput (appContext, iReadSock, (XtPointer)XtInputReadMask, 
		       CBK_XIncomingMessage, NULL); 

  /* Add event handlers */
  XtAddEventHandler(wMap, ExposureMask, False, (XtEventHandler)CBK_RefreshMap,
		    NULL);
  XtAddEventHandler(wMap, ExposureMask, False, (XtEventHandler)CBK_RefreshDice,
		    NULL);
}


/************************************************************************ 
 *  FUNCTION: GUI_LoadMap
 *  HISTORY: 
 *     01.26.94  ESF  Created.
 *     01.27.94  ESF  Added palette code.
 *     01.28.94  ESF  Changed to allocate pixmap as well as image.
 *     02.03.94  ESF  Don't allocate colors in order, use mapping.
 *     02.23.94  ESF  Started adding 24 bit server support.
 *     04.01.94  ESF  Fixed free on bogus pointer.
 *     05.12.94  ESF  Moved world colors to colormap.c.
 *     05.19.94  ESF  Added TrueColor display patches.
 *     09.09.94  ESF  Fixed small bug in initializing player turn indicator.
 *  PURPOSE: 
 *  NOTES: TdH: i think here need to do some adjusment for colormap
 ************************************************************************/
void GUI_LoadMap(CString strMapFile)
{
  Int32   iHeight, iWidth ,iNumPixels;
  FILE   *hMap;
  Int32   iPixels;
  Byte    bColor, bLength, *pMapData, *pDataStream, *pBuffer, *pTemp;
  Int32   lCurrent, lEnd, iNC;
  char buf[256];

  /* Open file and get dimensions */
  if ((hMap=UTIL_OpenFile(strMapFile, "r"))==NULL)
    {
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "GUI: Could not load %s.", strMapFile);
      (void)UTIL_PopupDialog("Fatal Error", buf, 1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "GUI: Ne peut charger %s.", strMapFile);
      (void)UTIL_PopupDialog("Erreur fatale", buf, 1, "Ok", NULL, NULL);
#endif
      UTIL_ExitProgram(-1);
    }

  fscanf(hMap, "%d%d%d\n", &iWidth, &iHeight, &iNC);
  COLOR_SetNumColors(iNC);

  /* Allocate memory for the map data */
  pMapData = (unsigned char *)MEM_Alloc(iWidth*iHeight);
  
  /* Allocate memory for the buffer; first find out size of file */
  lCurrent = ftell(hMap);
  fseek(hMap, 0, SEEK_END);
  lEnd = ftell(hMap);
  fseek(hMap, lCurrent, SEEK_SET);
  pTemp = pBuffer = (unsigned char *)MEM_Alloc(lEnd-lCurrent);

  /* Read in the rest of the file */
  fread(pBuffer, 1, lEnd-lCurrent, hMap);

  iNumPixels = iWidth*iHeight;
  for (iPixels=0,pDataStream=pMapData; iPixels < iNumPixels;  iPixels+=bLength)
    {
      /* Get a color segment */
      bLength = *pBuffer++;
      bColor  = *pBuffer++;
     
      /* Adjust for black/white */
      if (bColor == BLACK)
	bColor = COLOR_GetNumColors()-2;
      else if (bColor == WHITE)
	bColor = COLOR_GetNumColors()-1;

      /* Put it in the map image */
      memset(pDataStream, COLOR_CountryToColor(bColor), bLength);
      pDataStream += bLength; 
    }
  
  /* Create an image to hold the map */
  pMapImage = XCreateImage(hDisplay, pVisual,
			   8, ZPixmap, 0, (char *)pMapData, iWidth, iHeight, 
			   8, iWidth);

  /* Create a pixmap to hold the image server-side */
  pixMapImage = XCreatePixmap(hDisplay, 
			      RootWindowOfScreen(XtScreen(wMap)),
			      iWidth, iHeight, 
			      DefaultDepth(hDisplay, DefaultScreen(hDisplay)));

  /* Save the world colormap */
  COLOR_SetWorldColormap(pBuffer);
  COLOR_SetWorldColors();

  /* Finally write map image to pixmap and window */
  if (!COLOR_IsTrueColor())
      XPutImage(hDisplay, pixMapImage, hGC, pMapImage, 0, 0, 0, 0, 
		iWidth, iHeight);
  XCopyArea(hDisplay, pixMapImage, hWindow, hGC, 
	    0, 0, iWidth, iHeight, 0, 0);

  /* After colormap is established we want to do this (move it elsewhere!) */
  XtVaSetValues(wCurrentPlayer, XtNbackground, 
     COLOR_QueryColor(COLOR_DieToColor(2)), NULL);

  /* Free up memory we were using */
  MEM_Free(pTemp);
}


/************************************************************************ 
 *  FUNCTION: GUI_GetNumColorsInMap
 *  HISTORY: 
 *     09.14.94  ESF  Created.
 *     10.02.94  ESF  Fixed to not call UTIL_PopupDialog.
 *  PURPOSE: 
 *  NOTES: Called once?
 ************************************************************************/
static Int32 GUI_GetNumColorsInMap(CString strMapFile)
{
  FILE   *hMap;
  Int32   iWidth, iHeight, iNumColors;

  /* Open file and get dimensions */
  if ((hMap=UTIL_OpenFile(strMapFile, "r"))==NULL)
    {
      printf("CLIENT: Fatal error, could not load %s.\n", strMapFile);
      UTIL_ExitProgram(-1);
    }

  /* Read the header and close the file */
  fscanf(hMap, "%d%d%d\n", &iWidth, &iHeight, &iNumColors);
  fclose(hMap);

  return iNumColors;
}


/************************************************************************ 
 *  FUNCTION: GUI_SetColorOfCurrentPlayer
 *  HISTORY: 
 *     17.08.94  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void GUI_SetColorOfCurrentPlayer(Int32 iColor)
{
  Int32 c, d;

  c = COLOR_QueryColor(iColor);
  XtVaGetValues(wCurrentPlayer, XtNbackground, &d, NULL);
  if (d != c)
      XtVaSetValues(wCurrentPlayer, XtNbackground, c, NULL);
}
