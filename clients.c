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
 *   $Id: clients.c,v 1.11 2000/01/20 17:58:49 morphy Exp $
 *
 *   $Log: clients.c,v $
 *   Revision 1.11  2000/01/20 17:58:49  morphy
 *   Removed dead code (static decl. of previously removed function)
 *   Added inclusion of string.h for memset() prototype
 *
 *   Revision 1.10  2000/01/17 22:31:54  tony
 *   removed "lost code"
 *
 *   Revision 1.9  1999/12/26 01:48:17  morphy
 *   Added missing doxygen file tag.
 *
 *   Revision 1.8  1999/12/26 01:43:22  morphy
 *   Converted _CLIENTS_GetNthClient() to static
 *
 *   Revision 1.7  1999/12/26 01:01:29  morphy
 *   Corrected layout of CVS Log tag in header.
 *
 *   Revision 1.6  1999/12/26 00:58:45  morphy
 *   Doxygen comments, minor layout changes.
 */

/** \file
 * Client list.
 */

#include <unistd.h>
#include <string.h>

#include "riskgame.h"
#include "clients.h"
#include "debug.h"
#include "language.h"


/* Private data types */

/**
 * Structure to track client failures
 */
typedef struct _Failure
{
  CString   strReason;
  Int32     iCount;
} Failure;


/**
 * Client data that doesn't get replicated to clients themselves.
 */
typedef struct _Client
{
  Int32     iCommLink;
  CString   strAddress;
  Int32     iState, iType, iSpecies;
  Flag      fStarted;
  Failure   failure;
} Client;


/* Private data structures */
static Client    pClients[MAX_CLIENTS]; /**< Client list */
static Int32     iNumClients  = 0;      /**< Number of clients actually on list */
static Int32     iNumFailures = 0;      /**< Total count of failures */


/**
 * Finish allocation of new client.
 *
 * \b History:
 * \arg 05.12.94  ESF  Created.
 * \arg 08.08.94  ESF  Moved here from server.c
 * \arg 08.21.94  ESF  Fixed to work.
 */
Int32 CLIENTS_Alloc(void)
{
  Int32 i;

  /* Since we wouldn't have accepted the connection if there weren't
   * a slot available, this call cannot fail.
   */

  D_Assert(CLIENTS_GetNumClients() < MAX_CLIENTS, "Not good!");
  CLIENTS_SetNumClients(CLIENTS_GetNumClients()+1);
  
  /* Find an available client */
  for (i=0; i!=MAX_CLIENTS; i++)
    if (CLIENTS_GetAllocationState(i) == ALLOC_NONE)
      {
	CLIENTS_SetAllocationState(i, ALLOC_COMPLETE);
	CLIENTS_SetCommLink(i, -1);
	CLIENTS_SetStartState(i, FALSE);
	return (i);
      }

  D_Assert(FALSE, "Something wierd happened!");
  
  /* For the compiler */
  return (0);
}


/**
 * Return failure count of given client.
 */
Int32 CLIENTS_GetFailureCount(Int32 iClient)
{
    return pClients[iClient].failure.iCount;
}


/**
 * Reset failure count of given client.
 */
void CLIENTS_ResetFailure(Int32 iNumClient)
{
    pClients[iNumClient].failure.iCount = 0;
    pClients[iNumClient].failure.strReason = 0;
}


/**
 * Set printable address of client
 *
 * \bug (MSH 25.12.99) Obsolete since client addresses are not disclosed to others anymore!
 */
void CLIENTS_SetAddress(Int32 iNumClient, CString strAddress)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  if (pClients[iNumClient].strAddress != NULL)
    MEM_Free(pClients[iNumClient].strAddress);
  pClients[iNumClient].strAddress = (CString)MEM_Alloc(strlen(strAddress)+1);
  strcpy(pClients[iNumClient].strAddress, strAddress);
}


/**
 * Set allocation state of given client.
 */
void CLIENTS_SetAllocationState(Int32 iNumClient, Int32 iState)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  pClients[iNumClient].iState = iState;
}


/**
 * Set communication socket of given client.
 */
void CLIENTS_SetCommLink(Int32 iNumClient, Int32 iCommLink)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");
  
  pClients[iNumClient].iCommLink = iCommLink;
}


/**
 * Increase failure count and set failure reason string for given client.
 */
void CLIENTS_SetFailure(Int32 iClient, CString strReason)
{
    pClients[iClient].failure.strReason = strReason;
    pClients[iClient].failure.iCount++;
    iNumFailures++;
}

/**
 * Set client type.
 */
void CLIENTS_SetType(Int32 iNumClient, Int32 iType)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");
  
  pClients[iNumClient].iType = iType;
}

/**
 * Set client species.
 */
void CLIENTS_SetSpecies(Int32 iNumClient, Int32 iSpecies)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");
  
  pClients[iNumClient].iSpecies = iSpecies;
}

/**
 * Set number of clients.
 */
void CLIENTS_SetNumClients(Int32 iClients)
{
  iNumClients = iClients;
}

/**
 * Tell about other clients to given client.
 */
void CLIENTS_TellAboutOthers(Int32 iCommLink, Int32 iClientType)
{
  char buf[256];
  int n;
  MsgMessagePacket      msgMess;

  if (CLIENTS_GetNumClients() > 1 && iClientType != CLIENT_AI)
    {
      /* Let the new client know about old clients */
      msgMess.strMessage = buf;
      msgMess.iFrom      = FROM_SERVER;
      msgMess.iTo        = DST_OTHER;

      /* Send info about everybody but the new client. */
      for (n=0; n!=MAX_CLIENTS; n++)
	if (CLIENTS_GetAllocationState(n) == ALLOC_COMPLETE &&
	    CLIENTS_GetCommLinkOfClient(n) != iCommLink)
	  {
            snprintf(buf, sizeof(buf),"%s %s.",CLIENT,IS_REGISTERED);
            /* Return value not wanted */
            (void) RISK_SendMessage(iCommLink, MSG_MESSAGEPACKET, &msgMess);
	  }
    }
}


/**
 * Return address of given client.
 */
CString CLIENTS_GetAddress(Int32 iNumClient)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  return pClients[iNumClient].strAddress;
}


/**
 * Return type of given client.
 */
Int32 CLIENTS_GetType(Int32 iNumClient)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  return pClients[iNumClient].iType;
}


/**
 * Return communication socket of given client.
 */
Int32 CLIENTS_GetCommLinkOfClient(Int32 iNumClient)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  return pClients[iNumClient].iCommLink;
}


/**
 * Return allocation state of given client.
 */
Int32 CLIENTS_GetAllocationState(Int32 iNumClient)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  return pClients[iNumClient].iState;
}


/**
 * Return species of given client.
 */
Int32 CLIENTS_GetSpecies(Int32 iNumClient)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  return pClients[iNumClient].iSpecies;
}


/**
 * Return number of clients.
 */
Int32 CLIENTS_GetNumClients(void)
{
  return iNumClients;
}


/**
 * Return the client using given commucation socket.
 *
 * \b History:
 * \arg 09.01.94  ESF  Created.
 */
Int32 CLIENTS_GetCommLinkToClient(Int32 iCommLink)
{
  Int32 i;

  D_Assert(iCommLink>=0 && iCommLink < MAX_DESCRIPTORS, "Bogus CommLink!");

  for (i=0; i!=MAX_CLIENTS; i++)
    if ((CLIENTS_GetAllocationState(i) == ALLOC_COMPLETE) &&
	(CLIENTS_GetCommLinkOfClient(i) == iCommLink))
      return (i);

  return (-1);
}


/**
 * Return failure reason string for given client.
 */
CString CLIENTS_GetFailureReason(Int32 iNumClient)
{
  D_Assert( iNumClient >= 0 && iNumClient < MAX_CLIENTS,"wrong client");

  return (pClients[iNumClient].failure.strReason);
}


/**
 * Return failure count for given client.
 */
Int32 CLIENTS_GetNumFailures(void)
{
    return iNumFailures;
}

/**
 * Return number of clients that have finished registering players.
 *
 * \b History
 * \arg 04.06.95  ESF  Created.
 */
Int32 CLIENTS_GetNumClientsStarted(void)
{
  Int32 i, iCount;

  for (i=iCount=0; i!=MAX_CLIENTS; i++)
    if (CLIENTS_GetAllocationState(i) == ALLOC_COMPLETE &&
	CLIENTS_GetStartState(i) == TRUE)
      iCount++;

  return iCount;
}


/**
 * Return start state of given client.
 *
 * \b History:
 * \arg 04.06.95  ESF  Created.
 */
Int32 CLIENTS_GetStartState(Int32 iNumClient)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");
  
  return pClients[iNumClient].fStarted;
}


/**
 * Initialize client list.
 *
 * \b History:
 * \arg 11.11.99 TdH Created
 * \arg 25.12.99 MSH Changed to use CLIENTS_SetNumFailures()
 */
void CLIENTS_Init(void)
{
    CLIENTS_SetNumClients(0);
    CLIENTS_SetNumFailures(0);
    memset(pClients, 0, sizeof(Client) * MAX_CLIENTS);
}


/**
 * Set start state of given client.
 *
 * \b History:
 * \arg 04.06.95  ESF  Created.
 */
void CLIENTS_SetStartState(Int32 iNumClient, Int32 iState)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");
  
  pClients[iNumClient].fStarted = iState;
}


/**
 * Set number of failures for given client.
 */
void CLIENTS_SetNumFailures(Int32 num)
{
    iNumFailures = num;
}

/* EOF */
