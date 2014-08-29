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
 *   $Id: client.c,v 1.14 2000/01/23 20:08:37 tony Exp $
 *
 *   $Log: client.c,v $
 *   Revision 1.14  2000/01/23 20:08:37  tony
 *   doxygen fixes
 *
 *   Revision 1.13  2000/01/07 21:37:07  tony
 *   a line after $Id: client.c,v 1.14 2000/01/23 20:08:37 tony Exp $
 *
 *   Revision 1.12  2000/01/04 21:41:53  tony
 *   removed redundant stuff for jokers
 *
 */

/** \file
 * Client functions and data, mainly about communication
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "utils.h"
#include "network.h"
#include "client.h"
#include "callbacks.h"
#include "riskgame.h"
#include "gui-func.h"
#include "types.h"
#include "dice.h"
#include "debug.h"
#include "registerPlayers.h"
#include "viewStats.h"
#include "addPlayer.h"
#include "language.h"

/* Private data */
/** \struct _Client
 * To store client info
 */
typedef struct _Client
{
  Int32  iCommLink;/**< just this commlink now */
} Client;


static Client   pClients[MAX_CLIENTS];/**< to keep track of others? */

/* Used for the UTIL_*Notify routines */ 
static Flag     piCountryLightCount[NUM_COUNTRIES];

static CString  strClientName;

extern  Int32   iReply;

static Int32    iCurrentClient;


/* Do something with these... */
void    FatalError(CString strError, Int32 iRetVal);


/**
 * Sets up communication stuff
 *
 * \b  History:
 * \tag     01.23.94  ESF  Created.
 * \tag     02.22.94  ESF  Cleaned up a bit, removing warnings.
 * \tag     08.10.94  ESF  Cleanup up to make more OS independant.
 * \tag     01.17.95  ESF  Fixed memory leak.
 */
void CLNT_Init(int argc, char **argv)
{
  struct sockaddr_in   server;
  struct hostent      *hp;
  char                 strHostName[MAXHOSTNAMELEN + 1];
  Int32                iCommLink, i;
  Int32                iMessageType;
  void                *pvMessage;
  MsgRegisterClient    msgRegisterClient;
  char buf[256];
  int len;

  UNUSED(argc);

  /* Seed the random number generator */
  srand(time(NULL));

  /* Init the client structures */
  for (i=0; i!=NUM_COUNTRIES; i++)
    CLNT_SetLightCountOfCountry(i, 0);

  for (i=0; i!=MAX_CLIENTS; i++)
    CLNT_SetCommLinkOfClient(i, -1);
  
  /* We want to ignore this signal, because we don't want a I/O
   * failure to terminate the program.
   */

  signal(SIGPIPE, SIG_IGN);

  /* Create Commlink */
  if ((iCommLink = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    FatalError(ERR_COMMLINK, 1);

  /* Connect socket using name specified by command line */
  server.sin_family = AF_INET;
  
  if ((hp = gethostbyname(argv[1])) == 0)
    {
      snprintf(buf, sizeof(buf), ERR_UNKNOWN_HOST, argv[1]); 
      FatalError(buf, 1);
    }
  
  memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
  server.sin_port = htons(RISK_PORT);

  /* Send the server name information through the sockets so it can ID
   * the sending clients, and discover the pair of sockets that belong
   * to each client.  The server sends back the client ID.
   */

  gethostname(strHostName, sizeof(strHostName));
  len = strlen(strHostName)+32;;
  msgRegisterClient.strClientAddress = (CString)MEM_Alloc(len);
  snprintf(msgRegisterClient.strClientAddress, len, "%s[%lu]", 
	  strHostName, (unsigned long)getpid());
  strClientName = msgRegisterClient.strClientAddress;
  
  /* Connect to the server.  Connect the socket, send 
   * MSG_REGISTER and then wait for a MSG_CLIENTIDENT 
   * message to be send back.
   */

  if (connect(iCommLink, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
      printf(ERR_COULD_NOT_CONNECT, argv[1]);
      FatalError(NULL, 0);
    }

  /* Set the options on the CommLink */
  NET_SetCommLinkOptions(iCommLink);
  
  (void)RISK_SendMessage(iCommLink, MSG_OLDREGISTERCLIENT, &msgRegisterClient);
  printf(MSG_CONNECTED);
  printf(MSG_WAITING_SRV_ID);
  fflush(stdout);
  (void)RISK_ReceiveMessage(iCommLink, &iMessageType, &pvMessage);
  printf(MSG_DONE);

  if (iMessageType == MSG_EXIT)
    FatalError(ERR_SERVER_FULL, -1);
  else if (iMessageType == MSG_CLIENTIDENT)
    {
      /* Set the ID of the client, and then set the sockets of the client */
      iCurrentClient = ((MsgClientIdent *)pvMessage)->iClientID;
      CLNT_SetCommLinkOfClient(CLNT_GetThisClientID(), iCommLink);
    }
  else
    FatalError(ERR_PROTOCOL_MISMATCH, -1); 

  /* Free up memory */
  NET_DeleteMessage(MSG_CLIENTIDENT, pvMessage);
}


/************************************************************************ 
 *  FUNCTION: CLNT_PreViewVector
 *  HISTORY: 
 *     02.19.95  ESF  Created.
 *     02.23.95  ESF  Added PLAYER_callback.
 *  PURPOSE: 
 *     All incoming and outgoing messages notify this function once and
 *   only once, for the purpose of notifying the views' controllers, 
 *   BEFORE(!!!) they have taken effect on the local dist. object.
 *  NOTES: 
 ************************************************************************/
void CLNT_PreViewVector(Int32 iMessType, void *pvMess)
{
  /* Ideally here I could spawn a thread per callback... */
  STAT_Callback(iMessType, pvMess);
}


/************************************************************************ 
 *  FUNCTION: CLNT_PostViewVector
 *  HISTORY: 
 *     03.30.95  ESF  Created.
 *  PURPOSE: 
 *     All incoming and outgoing messages notify this function once and
 *   only once, for the purpose of notifying the views' controllers, 
 *   AFTER(!!!) they have taken effect on the local dist. object.
 *  NOTES: 
 ************************************************************************/
void CLNT_PostViewVector(Int32 iMessType, void *pvMess)
{
  /* Ideally here I could spawn a thread per callback... */
  PLAYER_Callback(iMessType, pvMess);
  REG_Callback(iMessType, pvMess);
  CBK_Callback(iMessType, pvMess);
}


/************************************************************************ 
 *  FUNCTION: CLNT_RecoverFailure
 *  HISTORY: 
 *     08.28.94  ESF  Created.
 *     09.28.94  ESF  Fixed to popup a message before exiting.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CLNT_RecoverFailure(CString strReason, Int32 iCommLink)
{
  UNUSED(iCommLink);

  printf(ERR_SERVER_FAILED, strReason);
  printf(ERR_NONRECOVERABLE);

  (void)UTIL_PopupDialog(ERR_OBJECT_FAILURE, ERR_SRV_NONRECOVERABLE, 1, "Ok", NULL, NULL);
  exit(0);
}


/************************************************************************ 
 *  FUNCTION: CLNT_AllocPlayer
 *  HISTORY: 
 *     05.12.94  ESF  Created.
 *     05.15.94  ESF  Fixed race condition, go to server for player.
 *  PURPOSE:
 *  NOTES: 
 ************************************************************************/
Int32 CLNT_AllocPlayer(void (*pfMsgHandler)(Int32, void *))
{
  (void)RISK_SendSyncMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			     MSG_ALLOCPLAYER, NULL,
			     MSG_REPLYPACKET, pfMsgHandler);
  return(iReply);
}


/************************************************************************ 
 *  FUNCTION: CLNT_FreePlayer
 *  HISTORY: 
 *     05.12.94  ESF  Created.
 *     05.15.94  ESF  Fixed race condition, go to server.
 *     07.16.94  ESF  Added assert.
 *  PURPOSE:
 *  NOTES: 
 ************************************************************************/
void CLNT_FreePlayer(Int32 i)
{
  MsgFreePlayer mess;

  /* Tell the server that this player ID is free */
  mess.iPlayer = i;

  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			 MSG_FREEPLAYER, &mess);
}


/***************************************/
void CLNT_SetCommLinkOfClient(Int32 iNumClient, Int32 iCommLink)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");
  
  pClients[iNumClient].iCommLink = iCommLink;
}

/***************************************/
Int32 CLNT_GetCommLinkOfClient(Int32 iNumClient)
{
  D_Assert(iNumClient>=0 && iNumClient<MAX_CLIENTS, 
	   "Client out of range!");

  return pClients[iNumClient].iCommLink;
}

/***************************************/
Int32 CLNT_GetThisClientID(void)
{
	return iCurrentClient;
}

/***************************************/
Int32 CLNT_GetLightCountOfCountry(Int32 iCountry)
{
  D_Assert(iCountry >= 0 && iCountry < NUM_COUNTRIES, "Country out of range");
  return piCountryLightCount[iCountry];
}

/***************************************/
void CLNT_SetLightCountOfCountry(Int32 iCountry, Int32 iLightCount)
{
  D_Assert(iCountry >= 0 && iCountry < NUM_COUNTRIES, "Country out of range");
  D_Assert(iLightCount>=0, "Bogus LightCount");
  piCountryLightCount[iCountry] = iLightCount;
}

/********************************************/
void FatalError(CString strError, Int32 iRetVal)
{
  if (strError)
    printf("%s\n", strError);
  exit(iRetVal);
}

