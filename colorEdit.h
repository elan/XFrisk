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
 *   $Id: colorEdit.h,v 1.4 2000/01/10 22:47:40 tony Exp $
 */

#ifndef _COLOREDIT
#define _COLOREDIT

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "types.h"

void    COLEDIT_UpdateColorWithInput(void);/* red info from inputbox */
void    COLEDIT_MapShiftClick(void); /* edit color country selected from map */
void    COLEDIT_BuildDialog(void); /* called once from gui */
String  COLEDIT_EditColor(Int32 iColor, Flag fStoreColor);/* call the dialog */

#endif

