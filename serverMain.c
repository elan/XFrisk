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
 *   $Id: serverMain.c,v 1.5 1999/12/26 21:49:35 morphy Exp $
 *
 *   $Log: serverMain.c,v $
 *   Revision 1.5  1999/12/26 21:49:35  morphy
 *   Doxygen comments.
 *
 */

/** \file
 * Server startup.
 */

#include <stdlib.h>
#include <stdio.h>

#include "server.h"
#include "riskgame.h"
#include "debug.h"


/**
 * Server startup function.
 *
 * \b History:
 * \arg 08.16.94  ESF  Created.
 */
int main(void)
{
  FILE *hLogFile=NULL; 

  /* Startup the memory debugging system */
  MEM_BootStrap("server-memory.log");

#ifdef LOGGING
  /* Open debugging log file */
  if ((hLogFile = fopen("server.log", "w")) == NULL)
    {
      printf("Could not open \"server.log\" for writing, exiting...");
      exit(-1);
    }
#endif

  /* Initialize the distributed object */
  RISK_InitObject(SRV_Replicate, NULL, NULL, SRV_LogFailure, hLogFile);

  /* Initialize the server and start things going */
  SRV_Init();
  SRV_PlayGame();
  return(0);
}

/* EOF */
