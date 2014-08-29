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
 *   $Id: aiClientMain.c,v 1.8 1999/11/27 18:19:33 tony Exp $
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef _AIX
#include <sys/select.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "types.h" 
#include "utils.h"
#include "client.h"
#include "version.h"
#include "network.h"
#include "riskgame.h"
#include "game.h"
#include "debug.h"
#include "aiClient.h"
#include "aiController.h"

/* Private functions */
void  AI_Start(Int32 argc, CString *argv);
void  AI_RecoverFailure(CString strReason, Int32 iNumFailures);
void  AI_MainLoop(void);
void  AI_Replicate(Int32 iMessType, void *pvMess, Int32 iType, Int32 iSource);

/* Dummies or stubs or fill-ins */
Int32 CLNT_GetCommLinkOfClient(Int32 iThisClient);
/* EEEK!!! this same function(name) has been defined in callbacks.c
 * callbacks is included, what is going on? which one called?
 */
void  CBK_IncomingMessage(Int32 iMessType, void *pvMess);
void  FatalError(CString strError, Int32 iExitValue);
CString  strClientName;

Int32    iCommLink, iReply;
Int32    iSpeciesID, iCurrentClient, iCurrentPlayer, iState;
Flag     fGameInitialized = FALSE;
Flag     fGameStarted = FALSE;


extern CString  __strName;
extern CString  __strAuthor;
extern CString  __strVersion;
extern CString  __strDesc;
extern void  *(*__AI_Callback)(void *, Int32, void *);

void    *pvContext[MAX_PLAYERS];

/* Move this to the Imakefile!! */
#ifdef __hpux
#define FDSET int 
#else
#define FDSET fd_set
#endif


/************************************************************************ 
 *  FUNCTION: main
 *  HISTORY: 
 *     02.20.95  ESF  Created from clientMain.c`main
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
int main(int argc, char **argv)
{
  Int32 i;

  /* Check args */
  if (argc != 2)
    {
      printf("Usage: %s <server_host>\n", argv[0]);
      exit(-1);
    }

  for (i=0; i!=MAX_PLAYERS; i++)
    pvContext[i] = (void *)NULL;

  /* Setup memory debugging library */
  MEM_BootStrap("aiClient-memory.log");

  /* Initialize everything */
  RISK_InitObject(AI_Replicate, NULL, NULL, AI_RecoverFailure, NULL);
  AI_Start(argc, argv);

  /* Let the server know that we're ready to go as soon as it is... */
  (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			 MSG_STARTGAME, NULL);
  iState = STATE_FORTIFY;

  /* And we're off */
  AI_MainLoop();
  return(0);
}


/************************************************************************ 
 *  FUNCTION: AI_Start
 *  HISTORY: 
 *     02.20.95  ESF  Created from CLNT_Init.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_Start(Int32 argc, CString *argv)
{
  struct sockaddr_in   server;
  struct hostent      *hp;
  char                 strHostName[MAXHOSTNAMELEN + 1];
  Int32                iMessageType, i;
  void                *pvMessage;
  MsgRegisterClient    msgRegisterClient;

  UNUSED(argc);

  /*
   * We want to ignore this signal, because we don't want a I/O
   * failure to terminate the program.
   */
  
  signal(SIGPIPE, SIG_IGN);
  
  /* Create sockets */
  if ((iCommLink = socket(AF_INET, SOCK_STREAM, 0)) < 0)
#ifdef ENGLISH
    FatalError("Cannot create CommLink.", 1);
#endif
#ifdef FRENCH
    FatalError("CLIENT: Ne peut créer la liaison.", 1);
#endif

  /* Connect socket using name specified by command line */
  server.sin_family = AF_INET;
  
  if ((hp = gethostbyname(argv[1])) == 0)
    {
      char buf[256];
#ifdef ENGLISH
      snprintf(buf, sizeof(buf),
	       "AI-CLIENT: The host `%s' is unknown.", argv[1]);
#endif
#ifdef FRENCH
      snprintf(buf, sizeof(buf),
	       "IA-CLIENT: La machine `%s' est inconnue.", argv[1]);
#endif
      FatalError(buf, 1);
    }
  
  memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
  server.sin_port = htons(RISK_PORT);
  
  /* Send the server name information through the sockets so it can ID
   * the sending clients, and discover the pair of sockets that belong
   * to each client.  The server sends back the client ID.
   */
  
  gethostname(strHostName, sizeof(strHostName));
  msgRegisterClient.strClientAddress = 
    (CString)MEM_Alloc(strlen(strHostName)+32);
  snprintf(msgRegisterClient.strClientAddress, strlen(strHostName)+32,
	   "%s[%lu]", strHostName, (unsigned long)getpid());
  strClientName = msgRegisterClient.strClientAddress;
  msgRegisterClient.iClientType = CLIENT_AI;

  /* The Protocol: 
   *
   * Connect to the server.  Then do the following:
   *
   *  Send MSG_HELLO.
   *                      Receive MSG_VERSION     --> OK!
   *                              Other           --> Protocol bogosity.
   *  Send MSG_REGISTERCLIENT if version is correct.
   *                      Receive MSG_CLIENTIDENT --> OK!
   *                              MSG_CLIENTEXIT  --> Server is full.
   *                              Other           --> Protocol bogosity.
   *  Send MSG_VERSION if version is incorrect.
   */

  if (connect(iCommLink, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
#ifdef ENGLISH
      printf("AI-CLIENT: Cannot connect to a Frisk server on `%s'.\n"
	     "        One probably needs to be started by running\n"
	     "        `friskserver' on the machine.\n", argv[1]);
#endif
#ifdef FRENCH
      printf("IA-CLIENT: Impossible de se connecter au serveur Frisk sur `%s'.\n"
	     "           Quelqu'un doit commencer par lancer `friskserver' \n"
	     "           sur cette machine.\n", argv[1]);
#endif
      FatalError(NULL, 0);
    }
  
  /* Send the greeting */
#ifdef ENGLISH
  printf("AI-CLIENT: Establishing CommLink to server.\n");
#endif
#ifdef FRENCH
  fflush(stdout);
  printf("IA-CLIENT: Établissement d'une communication avec le serveur.\n");
#endif
  (void)RISK_SendMessage(iCommLink, MSG_HELLO, NULL);

  /* Get the version and check it. */
  (void)RISK_ReceiveMessage(iCommLink, &iMessageType, &pvMessage);

  if (iMessageType != MSG_VERSION)
    {
#ifdef ENGLISH
      printf("AI-CLIENT: Server is not following protocol (%d)!  Exiting.\n", 
#endif
#ifdef FRENCH
      printf("IA-CLIENT: Le serveur n'utilise pas le même protocol (%d)!  Quit.\n", 
#endif
	     iMessageType);
      FatalError(NULL, 0);
    }

  if (strcmp(((MsgVersion *)pvMessage)->strVersion, VERSION))
    {
#ifdef ENGLISH
      printf("AI-CLIENT: Version mismatch (server running %s)!  Exiting.\n",
#endif
#ifdef FRENCH
      printf("IA-CLIENT: Mauvaise version (le server utilise %s)!  Quit.\n",
#endif
	     ((MsgVersion *)pvMessage)->strVersion);
      FatalError(NULL, 0);
    }
  
  NET_DeleteMessage(iMessageType, pvMessage);

  /* Send back the registration message */
  (void)RISK_SendMessage(iCommLink, MSG_REGISTERCLIENT, &msgRegisterClient);

  /* Get the species ID */
#ifdef ENGLISH
  printf("AI-CLIENT: Waiting for server to send species ID...");
#endif
#ifdef FRENCH
  printf("IA-CLIENT: Attente du server pour obtenir l'ID de l'espèce...");
#endif
  (void)RISK_ReceiveMessage(iCommLink, &iMessageType, &pvMessage);
  
  if (iMessageType != MSG_SPECIESIDENT)
    {
#ifdef ENGLISH
      printf("Server is not following protocol (%d)!", iMessageType);
      FatalError("Protocol error.", -1);
#endif
#ifdef FRENCH
      printf("IA-CLIENT: Le serveur n'utilise pas le même protocol (%d)!\n", 
             iMessageType);
      FatalError("Erreur de protocol.", -1);
#endif
    }
  else
#ifdef ENGLISH
    printf("Done.\n");  
#endif
#ifdef FRENCH
    printf("Reçu.\n");  
#endif

  iSpeciesID = ((MsgSpeciesIdent *)pvMessage)->iSpeciesID;
  NET_DeleteMessage(iMessageType, pvMessage);

  /* Wait for the response */
#ifdef ENGLISH
  printf("AI-CLIENT: Waiting for server to send client ID...");
#endif
#ifdef FRENCH
  printf("IA-CLIENT: Attend que le serveur émette l'ID client...");
#endif
  (void)RISK_ReceiveMessage(iCommLink, &iMessageType, &pvMessage);
  
  if (iMessageType == MSG_EXIT)
    FatalError("Can't connect, server is full -- I'm impressed!", -1);
  else if (iMessageType == MSG_CLIENTIDENT)
    {
#ifdef ENGLISH
      printf("Done.\n");  
#endif
#ifdef FRENCH
      printf("Reçu.\n");  
#endif

      /* Set the ID of the client, and then set the sockets of the client */
      iCurrentClient = ((MsgClientIdent *)pvMessage)->iClientID;
    }
  else
    {
#ifdef ENGLISH
      printf("Server is not following protocol (%d)!", iMessageType);
      FatalError("Protocol error!", -1);
#endif
#ifdef FRENCH
      printf("IA-CLIENT: Le serveur n'utilise pas le même protocol (%d)!\n", 
             iMessageType);
      FatalError("Erreur de protocol.", -1);
#endif
    }

  /* Free up memory */
  NET_DeleteMessage(MSG_CLIENTIDENT, pvMessage);

  /* Get the species ID */
  i = iSpeciesID;
#ifdef ENGLISH
  printf("AI-CLIENT: Me species #%d.  You Jane.\n", i);
#endif
#ifdef FRENCH
  printf("IA-CLIENT: Mon espèce est %d.\n", i);
#endif

  /* Init the species; afterwards, indicate the species is finished. */
  RISK_SetNameOfSpecies(i, __strName);
  RISK_SetClientOfSpecies(i, CLNT_GetThisClientID());
  RISK_SetAuthorOfSpecies(i, __strAuthor);
  RISK_SetVersionOfSpecies(i, __strVersion);
  RISK_SetDescriptionOfSpecies(i, __strDesc);
  RISK_SetAllocationStateOfSpecies(i, ALLOC_COMPLETE);

  /* Let the player do one-time initializations */
  __AI_Callback(NULL, AI_INIT_ONCE, (void *)iSpeciesID); 
}
     


/************************************************************************ 
 *  FUNCTION: CLNT_GetThisClientID
 *  HISTORY:
 *     30.10.99  MSH  Created.
 *  PURPOSE:
 *     Returns client ID of 'this' client
 *  NOTES: See below
 ************************************************************************/
Int32 CLNT_GetThisClientID(void)
{
	return iCurrentClient;
}


/************************************************************************ 
 *  FUNCTION: CLNT_GetCommLinkOfClient
 *  HISTORY: 
 *     02.20.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: DUMMY!!
 ************************************************************************/
Int32 CLNT_GetCommLinkOfClient(Int32 iThisClient)
{
  UNUSED(iThisClient);
  return iCommLink;
}


/************************************************************************ 
 *  FUNCTION: AI_RecoverFailure
 *  HISTORY: 
 *     02.21.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_RecoverFailure(CString strReason, Int32 iFailureCommlink)
{
#ifdef ENGLISH
  printf("AI-CLIENT: Fatal Error (%s) on CommLink %d.  Not equipped to\n"
	 "recover from failure yet.  I'm exiting.\n", 
#endif
#ifdef FRENCH
  printf("IA-CLIENT: Erreur fatale (%s) sur la ligne %d.\n"
         "           Pas equippé pour récupérer cette erreur.\n"
         "           Je quitte.\n", 
#endif
	 strReason, iFailureCommlink);
  exit(-1);
}


/************************************************************************ 
 *  FUNCTION: FatalError
 *  HISTORY: 
 *     02.21.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void FatalError(CString strError, Int32 iExitValue)
{
  if (strError)
#ifdef ENGLISH
    printf("AI-CLIENT: Fatal error (%s)\n", strError);
#endif
#ifdef ENGLISH
    printf("IA-CLIENT: Erreur fatale (%s)\n", strError);
#endif
  exit(iExitValue);
}


/************************************************************************ 
 *  FUNCTION: AI_MainLoop
 *  HISTORY: 
 *     02.21.95  ESF  Created from CLNT_Init.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_MainLoop(void)
{
  void   *pvMessage;
  Int32   iMessageType;
  fd_set  fdSet, fdBackup;

  FD_ZERO(&fdBackup);
  FD_SET(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), &fdBackup);

  /* Initialize everything */
  AI_Init();

  /* Loop forever */
  for(;;)
    {
      fdSet = fdBackup;

      /* Wait for a message */
      if (select(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID())+1, 
		 (FDSET *)&fdSet, (FDSET *)0, (FDSET *)0, 
		 NULL) < 0)
	perror("Select");
      
      /* Receive the message */
      if (!RISK_ReceiveMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			       &iMessageType, &pvMessage))
	AI_RecoverFailure("Server failed.", -1);
     
      /* Process it */
      CBK_IncomingMessage(iMessageType, pvMessage);
    }
}


/************************************************************************ 
 *  FUNCTION: CBK_IncomingMessage
 *  HISTORY: 
 *     04.30.95  ESF  Created.
 *     25.08.95  JC   Added MSG_EXIT.
 *     25.08.95  JC   Modified MSG_ENDOFGAME, if computer is the winner and
 *                    if more than one player, then the computer decide to 
 *                    continue.
 *     25.08.95  JC   Start the game only after a fortification.
 *     28.08.95  JC   Added MSG_ENDOFMISSION and MSG_VICTORY.
 *     30.08.95  JC   Added MSG_FORCEEXCHANGECARDS.
 *     04.09.95  JC   Added AI_SERVER_MESSAGE.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void CBK_IncomingMessage(Int32 iMessType, void *pvMessage)
{
  switch(iMessType)
    {
    case MSG_TURNNOTIFY:
      {
	MsgTurnNotify *pvMess = (MsgTurnNotify *)pvMessage;

	/* Set the current player */
	iCurrentPlayer = pvMess->iPlayer;

	if (!fGameInitialized)
	  {
            fGameInitialized = TRUE;
            /* Let the player do initializations of game */
            __AI_Callback(NULL, AI_INIT_GAME, (void *)iSpeciesID);
          }

	/* Check to see if it's this client. */
	if (pvMess->iClient == CLNT_GetThisClientID())
	  {
	    pvContext[iCurrentPlayer] = 
	      __AI_Callback(pvContext[iCurrentPlayer], AI_INIT_TURN, NULL);

	    /* See if the fortification is done */
	    if (!fGameStarted && (iState == STATE_FORTIFY) && 
		(RISK_GetNumArmiesOfPlayer(iCurrentPlayer) == 0))
	      fGameStarted = TRUE;

	    if (fGameStarted)
	      {
		if (RISK_GetNumCardsOfPlayer(iCurrentPlayer) >= 3)
		  {
		    pvContext[iCurrentPlayer] = 
		      __AI_Callback(pvContext[iCurrentPlayer],
				    AI_EXCHANGE_CARDS,
				    NULL);
		    AI_CheckCards();
		  }

		/* Give the players the initial armies */
		GAME_SetTurnArmiesOfPlayer(iCurrentPlayer);
                if (iState == STATE_REGISTER)
                  return;

		iState = STATE_PLACE;
                do
                  {
		    /* Send the commands */
		    pvContext[iCurrentPlayer] = 
		      __AI_Callback(pvContext[iCurrentPlayer],
		    		    AI_PLACE,
		    		    NULL);
                    if (iState == STATE_ATTACK)
                      {
                        AI_CheckPlacement();
		        pvContext[iCurrentPlayer] = 
		          __AI_Callback(pvContext[iCurrentPlayer], 
				        AI_ATTACK,
				        NULL);
                      }
                  }
                while (iState == STATE_PLACE);

                if (iState == STATE_ATTACK)
                  {
		    iState = STATE_MOVE;
		    pvContext[iCurrentPlayer] = 
		      __AI_Callback(pvContext[iCurrentPlayer], 
				    AI_MOVE,
				    NULL);
	          }
	      }
	    else if (iState == STATE_FORTIFY)
	      {
		pvContext[iCurrentPlayer] = 
		  __AI_Callback(pvContext[iCurrentPlayer], 
				AI_FORTIFY, 
				(void *)1);
                AI_CheckFortification();
	      }

            if (iState != STATE_REGISTER)
              {
	        /* End the turn */
	        AI_EndTurn();
	      }
	  }
      }
      break;

    case MSG_MESSAGEPACKET:
      {
        MsgMessagePacket *msgMessagePacket = (MsgMessagePacket *)pvMessage;

        if (msgMessagePacket->iFrom >= FROM_SERVER)
          {
            Int32 iOldPlayer = iCurrentPlayer;

            if (msgMessagePacket->iTo >= 0)
                iCurrentPlayer = msgMessagePacket->iTo;
            else
                iCurrentPlayer = RISK_GetNthPlayerAtClient(CLNT_GetThisClientID(), 0);
	    (void)__AI_Callback(NULL, AI_SERVER_MESSAGE, 
			        (void *)(msgMessagePacket->strMessage));
            iCurrentPlayer = iOldPlayer;
          }
        else if (   (msgMessagePacket->iFrom >= 0)
            && (msgMessagePacket->iTo   >= 0))
          {
            Int32 iOldPlayer = iCurrentPlayer;

            iCurrentPlayer = msgMessagePacket->iTo;
	    pvContext[iCurrentPlayer] = 
	      __AI_Callback(pvContext[iCurrentPlayer],
			    AI_MESSAGE, 
			    (void *)msgMessagePacket);
            iCurrentPlayer = iOldPlayer;
          }
      }
      break;

    case MSG_FORCEEXCHANGECARDS:
      {
        MsgForceExchangeCards *msg = (MsgForceExchangeCards *)pvMessage;

        if (msg->iPlayer >= 0)
          {
            if (iState != STATE_ATTACK)
              return;
            iCurrentPlayer = msg->iPlayer;
            pvContext[iCurrentPlayer] = 
	    __AI_Callback(pvContext[iCurrentPlayer],
		          AI_EXCHANGE_CARDS, NULL);
	    AI_CheckCards();
            if (iState != STATE_ATTACK)
              return;
	    iState = STATE_PLACE;
	  }
      }
      break;

    case MSG_ENDOFMISSION:
      {
	Int32 iWinner = ((MsgEndOfMission *)pvMessage)->iWinner;

	D_Assert(iWinner >= -1 && iWinner < MAX_PLAYERS, "Bogus Winner!");

	if (RISK_GetClientOfPlayer(iWinner) == CLNT_GetThisClientID())
          {
	    if (RISK_GetNumLivePlayers() <= 1)
	      {
	        fGameStarted = FALSE;
	        iState = STATE_REGISTER;
                (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			               MSG_ENDOFGAME, NULL);
              }
          }
      }
      break;

    case MSG_VICTORY:
      {
	Int32 iWinner = ((MsgVictory *)pvMessage)->iWinner;

	D_Assert(iWinner >= -1 && iWinner < MAX_PLAYERS, "Bogus Winner!");

	if (RISK_GetClientOfPlayer(iWinner) == CLNT_GetThisClientID())
          {
	    fGameStarted = FALSE;
	    iState = STATE_REGISTER;
            (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			           MSG_ENDOFGAME, NULL);
          }
      }
      break;

    case MSG_ENDOFGAME:
      {
	fGameStarted = FALSE;
	iState = STATE_FORTIFY;

        /* Let the server know that this client is ready to play */
        (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()), 
			       MSG_STARTGAME, NULL);

        fGameInitialized = FALSE;
        AI_Init();
      }

    case MSG_NOMESSAGE:
      break;

    case MSG_REPLYPACKET:
      iReply = ((MsgReplyPacket *)pvMessage)->iReply; 
      break;

    case MSG_EXIT:
      UTIL_ExitProgram(0);
      break;
    default:
      ; /* Nothing... */
    }
}


/************************************************************************ 
 *  FUNCTION: UTIL_GetArmyNumber
 *  HISTORY: 
 *     02.25.95  ESF  Created.
 *     30.08.95  JC   Computer can play like a human.
 *     31.08.95  JC   if (ARMIES_MOVE_MIN == ARMIES_MOVE_MAX) ->
 *                    if (iMoveMode == ARMIES_MOVE_MIN).
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 UTIL_GetArmyNumber(Int32 iMin, Int32 iMax, Flag fLetCancel) 
{
  extern Int32 iMoveMode;
  UNUSED(fLetCancel);

  if (iMoveMode == ARMIES_MOVE_MAX)
      return iMax;
  if (iMoveMode == ARMIES_MOVE_MIN)
      return iMin;
  if (iMoveMode == ARMIES_MOVE_MANUAL)
    {
      Int32 iNumArmies = iMax-iMin;

      pvContext[iCurrentPlayer] =
        __AI_Callback(pvContext[iCurrentPlayer], AI_MOVE_MANUAL, 
                      (void *)&iNumArmies);
      return iNumArmies+iMin;
    }
  return (iMax-iMin)/2;
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.25.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_RegisterPreObserver(void (*PreCallback)(Int32, void *))
{
  UNUSED(PreCallback);
  /* A bit of a hack, for now... */

}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.25.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_RegisterPostObserver(void (*PostCallback)(Int32, void *))
{
  UNUSED(PostCallback);
  /* A bit of a hack, for now... */

}


/************************************************************************ 
 *  FUNCTION: AI_Replicate
 *  HISTORY: 
 *     02.25.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AI_Replicate(Int32 iMessType, void *pvMess, Int32 iType, Int32 iSource)
{
  UNUSED(iSource);
  /*
   * Do the physical replication (the server will broadcast it),
   * but only if this callback is being called for an outgoing
   * message (i.e. a replication).  Otherwise, a message just
   * came in.
   */

  if (iType == MESS_OUTGOING)
    {
      (void)RISK_SendMessage(CLNT_GetCommLinkOfClient(CLNT_GetThisClientID()),
			     iMessType, pvMess);
    }
}
