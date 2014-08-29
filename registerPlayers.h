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
 *   $Id: registerPlayers.h,v 1.3 1999/11/13 21:58:32 morphy Exp $
 */

#ifndef _REGISTER
#define _REGISTER 

#include <X11/Intrinsic.h>
#include "types.h"

void REG_BuildDialog(void);
void REG_PopupDialog(void);
void REG_Callback(Int32 iMessType, void *pvMess);

/* These should be private, called from REG_Callback */
void REG_MouseClick(Widget w, XEvent *pEvent, CString *str, Cardinal *card);
void REG_MouseShiftClick(Widget w, XEvent *pEvent, String *str, 
			 Cardinal *card);

#endif
