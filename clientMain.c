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
 *   $Id: clientMain.c,v 1.11 2000/01/05 23:20:44 tony Exp $
 *
 *   $Log: clientMain.c,v $
 *   Revision 1.11  2000/01/05 23:20:44  tony
 *   change CARD_ to CARDS_
 *
 *   Revision 1.10  1999/12/26 15:39:33  tony
 *   oops, that was borked
 *
 *   Revision 1.9  1999/12/26 15:09:24  tony
 *   merged two versions, conflicts in doxygen
 *
 *   Revision 1.8  1999/12/26 01:35:28  morphy
 *   Doxygen comments, CVS Log tag, corrected comment usage before main() - removed warning about comment start tag inside comment
 *
 */
/** \file
 * The client's main()
 *
 */


#include "cards.h"
#include "callbacks.h"
#include "client.h"
#include "colormap.h"
#include "network.h"
#include "riskgame.h"
#include "dice.h"
#include "help.h"
#include "gui-func.h"
#include "debug.h"

/**
 * Client main function.
 *
 * \b History:
 * \arg 01.23.94  ESF  Created.
 * \arg 02.22.94  ESF  Cleaned up a bit, removing warnings.
 * \arg 08.10.94  ESF  Cleanup up to make more OS independant.
 * \arg 08.16.94  ESF  Moved to its own file, cleaned up.
 * \arg 10.30.94  ESF  Fixed a bug, have to init dist. obj. first!
 * \arg 12.30.94  ESF  Added more robust startup checks.
 */

int main(int argc, char **argv) {
  FILE *hLogFile=NULL;

  /* Check args */
  if (argc < 2)
    {
      printf("Usage: %s <server_host> [<xargs>]\n", argv[0]);
      exit(-1);
    }

  /* Setup memory debugging library */
  MEM_BootStrap("client-memory.log");

#ifdef LOGGING
  /* Open debugging log file */
  if ((hLogFile = fopen("client.log", "w")) == NULL)
    {
#ifdef ENGLISH
      printf("Could not open \"client.log\" for writing, exiting...");
#endif
#ifdef FRENCH
      printf("Impossible d'ouvrir \"client.log\" en écriture, quit...");
#endif
      exit(-1);
    }
#endif

  /* Initialize everything */
  RISK_InitObject(CBK_Replicate, CLNT_PreViewVector, CLNT_PostViewVector,
		  CLNT_RecoverFailure, hLogFile);/* callbacks */
  CLNT_Init(argc, argv);/* communications */
  GUI_Setup(argc, argv);/* */
  COLOR_Init();
  GUI_AddCallbacks(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()));
  DICE_Init();
  HELP_Init(HELPFILE);
  CARDS_Init();/* The graphical part of cards */

  /* And we're off... */
  GUI_Start();
  return 0;
}

/* EOF */
