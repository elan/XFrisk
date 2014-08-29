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
 *   $Id: help.c,v 1.4 1999/11/13 21:58:31 morphy Exp $
 */

#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/List.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#include "riskgame.h"
#include "network.h"
#include "types.h"
#include "help.h"
#include "client.h"
#include "gui-vars.h"
#include "callbacks.h"
#include "utils.h"
#include "debug.h"

static CString   *pstrIndexCStrings;
static CString   *pstrHelpCStrings;
static Int32      iNumHelpTopics=0;

/* The first is the default number of topics, and the latter is the 
 * number of new topics to allocate if we run out room.
 */

#define HELP_TOPICS      32
#define HELP_MORETOPICS  8

/************************************************************************ 
 *  FUNCTION: HELP_Init
 *  HISTORY: 
 *     04.01.94  ESF  Created 
 *     04.02.94  ESF  Added allocation of more memory if needed.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void HELP_Init(CString strHelpFile)
{
  FILE             *hHelpFile;
  struct stat       statBuf;
  char             *pHelpText;
  Int32             i, j, iTopicsMemory = HELP_TOPICS;
  char buf[256];

  /* Allocate the default amount of room for the help topics */
  pstrIndexCStrings = (CString *)MEM_Alloc(sizeof(CString)*iTopicsMemory);
  pstrHelpCStrings = (CString *)MEM_Alloc(sizeof(CString)*iTopicsMemory);

  /* Open the file, and find out its size to allocate memory for it */
  if ((hHelpFile=UTIL_OpenFile(HELPFILE, "r"))==NULL)
    {
#ifdef ENGLISH
      snprintf(buf, sizeof(buf), "HELP: cannot open %s!", HELPFILE);
      UTIL_PopupDialog("Fatal Error", buf, 1, "Ok", NULL, NULL);
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf), "HELP: impossible d'ouvrir %s!", HELPFILE);
      UTIL_PopupDialog("Erreur fatale", buf, 1, "Ok", NULL, NULL);
#endif
      UTIL_ExitProgram(-1);
    }
  
  if (fstat(fileno(hHelpFile), &statBuf)!=0)
    {
#ifdef ENGLISH
      UTIL_PopupDialog("Fatal Error", "HELP: Couldn't get size of file", 
#endif
#ifdef FRENCH
      UTIL_PopupDialog("Erreur fatale", "HELP: Impossible d'obtenir la "
                       "taille du fichier", 
#endif
		       1, "Ok", NULL, NULL);
      UTIL_ExitProgram(-1);
    }

  /* Read in the file after allocating room for it */
  pHelpText = strHelpFile = (CString)MEM_Alloc(statBuf.st_size);
  fread(strHelpFile, statBuf.st_size, 1, hHelpFile);

  /* Search through the file for topics, filling out the tables of topic
   * pointers as we go, and allocating more memory as needed.  When we
   * find an end of topic, stick a '\0' in, and erase '\n' since the
   * text widget will do the word wrapping for us.  This can be sped up!
   */

  for (i=0; i!=statBuf.st_size; i++)
    {
      if (pHelpText[i] == '\n' && 
	  (pHelpText[i+1] == '\n' || pHelpText[i+1] == '\t')) 
	  i++;
      /* Stick in a blank as long as the last character wasn't a space */
      else if (pHelpText[i] == '\n' && i>0 && pHelpText[i-1] != ' ')
	pHelpText[i] = ' ';
      else if (pHelpText[i] == '%' && pHelpText[i+1] == '%')
	{
	  /* We've found the end of a topic */
	  pHelpText[i] = '\0';
	  i+=2;
	}
      else if (pHelpText[i] == '%')
	{
	  /* We've found the beginning of a topic */
	  pstrIndexCStrings[iNumHelpTopics] = &pHelpText[i+1];

	  /* Find the beginning of the topic */
	  for (j=i; pHelpText[j]!='\n'; j++)
	    ; /* TwiddleThumbs() */

	  pHelpText[j] = '\0';
	  pstrHelpCStrings[iNumHelpTopics++] = &pHelpText[j+1];

	  /* Allocate more memory? */
	  if (iNumHelpTopics > iTopicsMemory)
	    {
	      iTopicsMemory += HELP_MORETOPICS;
	      pstrHelpCStrings = (CString *)realloc(pstrHelpCStrings, 
						  sizeof(CString)*
						  iTopicsMemory);
	      pstrIndexCStrings = (CString *)realloc(pstrIndexCStrings, 
						   sizeof(CString)*
						   iTopicsMemory);
	    }
	}
    }
	
  /* Put the index strings in the list widget */
  XtVaSetValues(wHelpTopicList, 
		XtNlist, pstrIndexCStrings, 
		XtNnumberStrings, iNumHelpTopics, 
		NULL);
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     04.01.94  ESF  Created 
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void HELP_IndexPopupHelp(Int32 iTopic)
{
  if (iTopic<0 || iTopic>=iNumHelpTopics)
    {
      UTIL_PopupDialog("Warning", "HELP: Topic not available!", 
		       1, "Ok", NULL, NULL);
      return;
    }

  XtVaSetValues(wHelpLabel, 
		XtNlabel, pstrIndexCStrings[iTopic],
		NULL);
  XtVaSetValues(wHelpText, 
		XtNstring, pstrHelpCStrings[iTopic],
		NULL);
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     04.01.94  ESF  Created 
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void HELP_CStringPopupHelp(CString strTopic)
{
  UNUSED(strTopic);
  D_Assert(FALSE, "Not implemented!");
}

