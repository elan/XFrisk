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
 *   $Id: gui-vars.h,v 1.4 1999/11/13 21:58:31 morphy Exp $
 */

#ifndef _GUI_VARS
#define _GUI_VARS

#include <X11/X.h>
#include <X11/Intrinsic.h>

#include "riskgame.h"
#include "colormap.h"

extern Widget wToplevel, wForm;
extern Widget wControls;
extern Widget wAttackLabel, wAttackList;
extern Widget wActionLabel, wActionList;
extern Widget wMsgDestLabel, wMsgDestList, wMsgDestViewport;
extern Widget wSendMsgText;
extern Widget wMsgText;
extern Widget wPlayField;
extern Widget wCommentLabel, wQuitButton, wRepeatButton, wCancelAttackButton;
extern Widget wHelpButton, wShowCardsButton;
extern Widget wEndTurnButton, wDiceBox;
extern Widget wErrorLabel;

/* Move to cardView.c */
extern Widget wCardShell, wCardForm, wCardTableBox;
extern Widget wCardViewport, wCardToggle[MAX_CARDS];
extern Widget wExchangeButton, wCancelCardsButton;

/* Move somewhere... */ 
extern Widget wArmiesShell, wArmiesForm, wArmiesLabel, wArmiesText;
extern Widget wFinishArmiesButton, wCancelArmiesButton;

/* Move to helpView.c */
extern Widget wHelpShell, wHelpForm, wHelpTopicViewport, wHelpTopicList;
extern Widget wHelpTopicLabel, wHelpText, wHelpOkButton;
extern Widget wHelpLabel;

/* Move to dialog.c */
extern Widget wDialogShell, wDialogLabel, wDialogButton[3], wDialogForm;
extern Widget wAboutButton;

extern XImage         *pMapImage;
extern Pixmap          pixMapImage;
extern Display        *hDisplay;
extern Window          hWindow;
extern int             iScreen;
extern GC              hGC, hGC_XOR;
extern XFontStruct    *pFont;
extern XtAppContext    appContext;
extern Visual         *pVisual;
extern Int32           iVisualCount;
extern Arg             pVisualArgs[3];

#endif
