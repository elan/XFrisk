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
 *   $Id: gui-func.h,v 1.4 1999/11/13 21:58:31 morphy Exp $
 */

#ifndef _GUI
#define _GUI

#include "types.h"

void GUI_Setup(Int32 argc, CString *argv);
void GUI_Start(void);
void GUI_AddCallbacks(Int32 iReadSock);
void GUI_LoadMap(CString strMapFile);
void GUI_SetColorOfCurrentPlayer(Int32 iColor);

#endif
