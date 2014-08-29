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
 *   $Id: server.c,v 1.24 2000/01/02 22:52:17 tony Exp $
 *
 *   $Log: server.c,v $
 *   Revision 1.24  2000/01/02 22:52:17  tony
 *   and a typo
 *
 *   Revision 1.23  2000/01/02 22:51:23  tony
 *   oops :-) still told hostname to clients
 *
 *   Revision 1.22  1999/12/25 23:19:01  morphy
 *   Yet more comments.
 *
 *   Revision 1.21  1999/12/25 23:12:20  morphy
 *   More comments on functions
 *
 *   Revision 1.20  1999/12/25 22:03:36  morphy
 *   Doxygen comments, removed 3 commented out functions
 *
 *
 */

/** \file
 * Server main loop and associated functionality.
 */
		   
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
#include <errno.h>

#include "language.h"
#include "types.h"
#include "riskgame.h"
#include "network.h"
#include "server.h"
#include "deck.h"
#include "debug.h"
#include "version.h"
#include "clients.h"

/* Move this to the Imakefile!! */
#ifdef __hpux
#define FDSET int 
#else
#define FDSET fd_set
#endif

/* This may be a static value (i.e. OPEN_MAX), or a dynamic value,
 * as in OSF/1, accessed through sysconf().  I'm not sure that all 
 * systems have these, so we'll just guess.  I don't think this is
 * exceptionally evil, since if we run out of descriptors, the socket
 * or accept calls will fail.
 */
#if 0
#define MAX_DESCRIPTORS 128
#endif

static Int32      iServerCommLink;                  /**< Server socket */
static Int32      iState;                           /**< */
static fd_set     fdSet;                            /**< Client socket states */
static fd_set     fdBackup;                         /**< Client socket states backup */
static Deck      *pPlayerDeck = NULL;               /**< Deck of free player ids */
static Deck      *pCardDeck;                        /**< Card deck */
static Int32      iReply;                           /**< ??? */
static Int32      iMaxFileDescUsed = -1;            /**< ??? */
static Int32      iServerMode = SERVER_REGISTERING; /**< ??? */
static Flag       fGameReset = TRUE;                /**< ??? */
static Flag       fRememberKilled = FALSE;          /**< ??? */
static Int32      iTurn;                            /**< ??? */
static Int32	  iFirstPlayer;                     /**< ??? */



/* Private functions */
void     SRV_ResetGame(void);
void     SRV_ReplicateRegistrationData(Int32 iCommLinkDest);
void     SRV_ReplicateAllData(Int32 iCommLinkDest);
void     SRV_AttemptNewGame(void);
void     SRV_DistributeCountries(void);
Flag     SRV_DistributeMissions(void);
void     SRV_SetInitialArmiesOfPlayers(void);
void     SRV_SetInitialMissionOfPlayers(void);
void     SRV_NotifyClientsOfTurn(Int32 iTurn);
void     SRV_HandleRegistration(Int32 iCommLink);
void     SRV_HandleSignals(Int32 iParam);
void     UTIL_ExitProgram(Int32 iExitValue);
Int32    SRV_IterateTurn(void);
 
/* Server message handlers */
void SRV_HandleEXIT(Int32 iExitValue);
void SRV_HandleALLOCPLAYER(Int32 iClient);
void SRV_HandleFREEPLAYER(void *pvMessage);
void SRV_HandleREPLYPACKET(void *pvMessage);
void SRV_HandleMESSAGEPACKET(Int32 iClient, void *pvMessage);
void SRV_HandleENTERSTATE(void *pvMessage);
void SRV_HandleDEREGISTERCLIENT(Int32 iClient);


/**
 * Server initialization (signal handling, creating server socket, ...)
 *
 * \b History:
 * \arg 01.23.94  ESF  Created.
 * \arg 02.22.94  ESF  Cleaned up a bit, removing warnings.
 * \arg 05.08.94  ESF  Fixed MSG_MESSAGEPACKET handling.
 * \arg 05.10.94  ESF  Fixed, not registering needed callback with DistObj.
 * \arg 05.12.94  ESF  Added fd usage map.
 * \arg 05.19.94  ESF  Fixed bug, passing &pvMessage instead of pvMessage.
 * \arg 08.16.94  ESF  Cleaned up.
 * \arg 10.01.94  ESF  Added initialization of failures.
 * \arg 11.11.99  TdH  Added call to CLIENTS_Init
 */
void SRV_Init(void)
{
  struct sockaddr_in   server;

  /* Print an informative message */
  printf("%s: %s %s\n",SERVERNAME,SERVER_STARTING ,VERSION);

  /* Catch signals so that we can clean up upon termination */
  signal(SIGHUP,  SRV_HandleSignals);
  signal(SIGINT,  SRV_HandleSignals);
  signal(SIGQUIT, SRV_HandleSignals);
  signal(SIGILL,  SRV_HandleSignals);
  signal(SIGTRAP, SRV_HandleSignals);
  signal(SIGFPE,  SRV_HandleSignals);
  signal(SIGKILL, SRV_HandleSignals);
  signal(SIGBUS,  SRV_HandleSignals);
  signal(SIGTERM, SRV_HandleSignals);

  /* We want to ignore this signal, because we don't want a I/O
   * failure to terminate the program.
   */

  signal(SIGPIPE, SIG_IGN);

  /* Init. clients and players */
  CLIENTS_Init();

  /* Initialize the pool of free players */
  pPlayerDeck = DECK_Create(MAX_PLAYERS);

  /* Create server socket */
  if ((iServerCommLink = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("Creating CommLink");
      UTIL_ExitProgram(1);
    }
  
  /* Update the max */
  iMaxFileDescUsed = MAX(iMaxFileDescUsed, iServerCommLink);
  
  /* Name sockets using wildcards */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(RISK_PORT);
  
  /* Bind the socket to the port */
  if (bind(iServerCommLink, (struct sockaddr *)&server, sizeof(server)))  {
        printf("%s: Frisk port %d %s\n",SERVERNAME,RISK_PORT,ERR_PORT_IN_USE);
        UTIL_ExitProgram(1);
    }

  /* Add the socket options to the socket */
  NET_SetCommLinkOptions(iServerCommLink);

}


/**
 * Broadcasts the given textual message to all clients
 *
 * \b History:
 * \arg 01.26.94  ESF  Created.
 * \arg 01.15.95  ESF  Initialized .iTo field to avoid uninitialized memory.
 */
void SRV_BroadcastTextMessage(CString strMessage)
{
  MsgMessagePacket   msgMess;

  msgMess.strMessage  = strMessage;
  msgMess.iFrom       = FROM_SERVER;
  msgMess.iTo         = DST_ALLPLAYERS;

  /* Send the message out to all clients */
  SRV_BroadcastMessage(MSG_MESSAGEPACKET, &msgMess);
}


/**
 * Main server message handler. Branches to message specific handlers.
 *
 * \b History:
 * \arg x.10.99 TdH took this bit out of SRV_PlayGame
 *
 * \bug morphy got -1 cards :-) has to do with greenland being #0
 * \bug (MSH 25.12.99) This function is humongous - perhaps a better way exists?
 */
int SRV_HandleMessage(Int32 iClient, Int32 iMessageType, void * pvMessage)
{
    Int32 n;
    char buf[256];

    /* Depending on the message type, dispatch it to one
     * of the handlers.  They usually take just a message,
     * but depending on what they do they could take the
     * and/or the client index.
     */
    D_Assert(iClient <= MAX_CLIENTS, "client > MAX_CLIENTS");
    switch (iMessageType) {
    case MSG_NETMESSAGE:
    case MSG_DICEROLL:
    case MSG_PLACENOTIFY:
    case MSG_ATTACKNOTIFY:
    case MSG_MOVENOTIFY:
        SRV_CompleteBroadcast(iClient, iMessageType, pvMessage);
        break;
    case MSG_ENDOFMISSION:
    case MSG_VICTORY:
        SRV_BroadcastMessage(iMessageType, pvMessage);
        break;
    case MSG_MISSION:
        if (SRV_DistributeMissions())
            SRV_BroadcastMessage(iMessageType, pvMessage);
        break;

    case MSG_ENDOFGAME:
        iServerMode = SERVER_REGISTERING;
        /* Let everyone else know */
        SRV_BroadcastMessage(MSG_ENDOFGAME, NULL);
        SRV_ResetGame();
        break;

    case MSG_ALLOCPLAYER:
        SRV_HandleALLOCPLAYER(iClient);
        break;

    case MSG_FREEPLAYER:
        SRV_HandleFREEPLAYER(pvMessage);
        break;

    case MSG_REPLYPACKET:
        SRV_HandleREPLYPACKET(pvMessage);
        break;

    case MSG_MESSAGEPACKET:
        SRV_HandleMESSAGEPACKET(iClient, pvMessage);
        break;

        /* Mark the client as started */
    case MSG_STARTGAME:
        
        CLIENTS_SetStartState(iClient, TRUE);
        /* Informative message */
        /* should tell which player, not easy*/
        printf("%s: %s %s\n",SERVERNAME,CLIENTS_GetAddress(iClient),FINISHED_REGISTRY);
        snprintf(buf,sizeof(buf),"%s: %s %s", SERVERNAME,NEW_CLIENT,FINISHED_REGISTRY);
        SRV_BroadcastTextMessage(buf);

        if (iServerMode == SERVER_REGISTERING) {
            /* Reset the game, and possible start it */
            SRV_ResetGame();/*will only reset if not already done...*/
            /* The first AI-Client can't start the game */
            if (    (CLIENTS_GetType(iClient) != CLIENT_AI)
                    || (CLIENTS_GetNumClientsStarted() > 1))
                SRV_AttemptNewGame();
        }
        else {
            /* Some random client tried to start in the middle
             * of a game, probably an overeager AI client.
             */
            SRV_BroadcastTextMessage("somebody tried to join after game was started");
            printf("client %s tried to join after game started\n",CLIENTS_GetAddress(iClient));
        }
        break;

    case MSG_DEREGISTERCLIENT:
        /* Do the actual deregistration */
        SRV_HandleDEREGISTERCLIENT(iClient);

        /* Informative message */
        sprintf(buf,"%s: %s %s\n",SERVERNAME,CLIENTS_GetAddress(iClient),HAS_DEREGISTERED);
        printf(buf);
        SRV_BroadcastTextMessage("A client has deregistered");
        break;

    case MSG_ENDTURN: {
        Int32 iPlayer;

        /* Sanity check */
        D_Assert(RISK_GetNumLivePlayers(), "Modulo by zero!");

        if (iServerMode == SERVER_FORTIFYING) {
            Int32  i;
            Flag   fFoundPlayer;

            /* If noone has any more armies, then move to
             * SERVER_PLAYING mode.  Otherwise, go along
             * the list of players, looking for a player who
             * still has armies to place.
             */

            for (i=0, fFoundPlayer=FALSE;
                 i!=RISK_GetNumLivePlayers() && !fFoundPlayer;
                 i++)  {
                /* The next player who's turn it is */
                iPlayer = SRV_IterateTurn();

                /* Is there a player with armies left? */
                if (RISK_GetNumArmiesOfPlayer(iPlayer) != 0) {
                    SRV_NotifyClientsOfTurn(iPlayer);
                    fFoundPlayer = TRUE;
                }
            }

            /* If we have not found a player with armies,
             * then there are no longer any players with any
             * armies to fortify with.  Let the game begin!!
             * Theorem (proof left to reader):  When some
             * players have more armies to to fortify with
             * than others (because they got less countries),
             * then the last player to fortify here will be
             * the last player in relation to the first player
             * who fortified.  So if we move to the next
             * player, it will be the player who fortified
             * first, which is what we want.
             *
             * This theorem is false when you use bpk's
             * multiple-fortify hack. See my comment in
             * SRV_AttemptNewGame. --Pac.
             */

            if (!fFoundPlayer)  {
                iServerMode = SERVER_PLAYING;
                iPlayer = iTurn = iFirstPlayer;
                SRV_NotifyClientsOfTurn(iPlayer);
            }
        }
        else { /* iServerMode == SERVER_PLAYING */
            /* Get the next player */
            iPlayer = SRV_IterateTurn();
            SRV_NotifyClientsOfTurn(iPlayer);
        }
    }
    break;

    case MSG_ENTERSTATE:
        SRV_HandleENTERSTATE(pvMessage);
        break;

    case MSG_EXCHANGECARDS: {/*went wrong for morphy, he got -1 cards */
        MsgExchangeCards *pMess = (MsgExchangeCards *)pvMessage;
        MsgReplyPacket    msgMess;
        Int32             iNumJokers;

        /* Put cards back on the deck and change them to type */
        for (n=iNumJokers=0; n!=3; n++) {
            DECK_PutCard(pCardDeck, pMess->piCards[n]);

            if (pMess->piCards[n] < NUM_COUNTRIES)
                pMess->piCards[n] %= 3;
            else
                pMess->piCards[n] = -1, iNumJokers++;
        }

        /* Find out how many armies the player gets in
         * exchange for the cards and send them to him or
         * her, in an _UPDATEARMIES message.  Right now
         * the only option is fixed return values for card
         * exchanges.
         */

        /* Do we have one of each (possibly with jokers)? */
        if ((pMess->piCards[0] != pMess->piCards[1] &&
             pMess->piCards[1] != pMess->piCards[2] &&
             pMess->piCards[0] != pMess->piCards[2]) ||
            iNumJokers >= 2) {
            msgMess.iReply = 10;
        }
        else if (pMess->piCards[0]==0 ||
                 pMess->piCards[1]==0 ||
                 pMess->piCards[2]==0) {
            msgMess.iReply = 8;
        }
        else if (pMess->piCards[0]==1 ||
                 pMess->piCards[1]==1 ||
                 pMess->piCards[2]==1) {
            msgMess.iReply = 6;
        }
        else {
            msgMess.iReply = 4;
        }

        (void)RISK_SendMessage(CLIENTS_GetCommLinkOfClient(iClient),
                               MSG_REPLYPACKET, &msgMess);
    }
    break;

    case MSG_REQUESTCARD: {
        MsgRequestCard *pMess = (MsgRequestCard *)pvMessage;

        RISK_SetCardOfPlayer(pMess->iPlayer,
                             RISK_GetNumCardsOfPlayer
                             (pMess->iPlayer),
                             DECK_GetCard(pCardDeck));
        RISK_SetNumCardsOfPlayer(pMess->iPlayer,
                                 RISK_GetNumCardsOfPlayer
                                 (pMess->iPlayer)+1);
    }
    break;

    case MSG_FORCEEXCHANGECARDS: {
        Int32 iPlayer ;

        iPlayer = ((MsgForceExchangeCards *)pvMessage)->iPlayer;
        if (RISK_GetNumCardsOfPlayer(iPlayer) < 5)
            ((MsgForceExchangeCards *)pvMessage)->iPlayer = -1;
        (void)RISK_SendMessage(CLIENTS_GetCommLinkOfClient(iClient),
                               MSG_FORCEEXCHANGECARDS,
                               pvMessage);
    }
    break;

    case MSG_EXIT:
        SRV_HandleEXIT(0);
        break;

    case MSG_NOMESSAGE:
        break;

    default: {
        MsgNetPopup msg;

        /* Assume that client is messed up.  Consider it
         * a failure and kill the client.
         */
        printf("%s: %s %s: %s",SERVERNAME,CLIENT,CLIENT_DEAD,INVALID_MESSAGE);
        msg.strMessage = buf;
        (void)RISK_SendMessage(CLIENTS_GetCommLinkOfClient(iClient),
                               MSG_NETPOPUP, &msg);

        /* Log the failure */
        SRV_LogFailure("Sent bogus message",
                       CLIENTS_GetCommLinkOfClient(iClient));
    }

    /* Free up the memory the message was taking */
    NET_DeleteMessage(iMessageType, pvMessage);

    /* broken
     D_Assert(iClient != MAX_CLIENTS && fHandledMessage == TRUE,
 "Message received from unknown source!!");
 */
    }
    return(0);
}


/**
 * Server main loop - 'Play the game'
 *
 * \b History:
 * \arg 02.04.94  ESF  Created.
 * \arg 02.05.94  ESF  Fixed broadcast loop bug. 
 * \arg 02.05.94  ESF  Fixed message receive bug.
 * \arg 03.03.94  ESF  Changed to send _UPDATE to all clients but sender.
 * \arg 03.28.94  ESF  Added _DEADPLAYER & _ENDOFGAME.
 * \arg 03.29.94  ESF  Added _REQUESTCARD.
 * \arg 04.01.94  ESF  Fixed card exchange to work right with jokers.
 * \arg 04.11.94  ESF  Fixed CARDPACKET to broadcast the card.
 * \arg 05.05.94  ESF  Added MSG_OBJ* msgs.
 * \arg 05.06.94  ESF  Factored out dealing cards code.
 * \arg 05.15.94  ESF  Added MSG_[ALLOC|FREE]PLAYER.
 * \arg 05.15.94  ESF  Added MSG_REPLYPACKET.
 * \arg 05.17.94  ESF  Added MSG_NETMESSAGE.
 * \arg 06.24.94  ESF  Fixed memory leak bug.
 * \arg 07.27.94  ESF  Completely revamped, combined with CollectPlayers().
 * \arg 09.31.94  ESF  Fixed so that a new deck is created upon reset.
 * \arg 10.01.94  ESF  Fixed MSG_ENDOFGAME to pass message and not NULL.
 * \arg 10.02.94  ESF  Fixed so in case of bogus message, client is killed.
 * \arg 10.03.94  ESF  Fixed bug, excessive processing of MSG_DEREGISTERCLIENT.
 * \arg 10.08.94  ESF  Added SERVER_FORTIFYING mode to fix a bug.
 * \arg 10.29.94  ESF  Added handling for MSG_DICEROLL.
 * \arg 10.30.94  ESF  Fixed serious bug: SERVER[_REGISTERING -> _FORTIFYING]. 
 * \arg 25.08.95  JC   Don't call perror if errno is equal to 4.
 * \arg 28.08.95  JC   Added handling for MSG_ENDOFMISSION and MSG_VICTORY.
 * \arg 30.08.95  JC   Added handling for MSG_FORCEEXCHANGECARDS.
 * \arg 30.08.95  JC   The first AI-Client can't start the game, but other
 *                     AI-Client can do this for computer's battles.
 * \arg 14.06.97  DAH  Don't use errno "4", use EINTR
 */
void SRV_PlayGame(void)
{
  Int32             iClient,  iMessageType, iError;
  void             *pvMessage;

  /* Create the card deck */
  pCardDeck = DECK_Create(NUM_COUNTRIES + 2);

  /* Add the initial fd to keep an eye on -- the connect socket */
  FD_ZERO(&fdBackup);
  FD_SET(iServerCommLink, &fdBackup);
  
  /* Start accepting connections */
  listen(iServerCommLink, 5);
  
  /* Loop for the entirety of the game */
  for(;;)
  {
      fdSet = fdBackup;

      /* If there have been any failures, then deal with them */
      SRV_RecoverFailures();

      /* Wait for a message to come in */
      if (select(iMaxFileDescUsed+1, (FDSET *)&fdSet, (FDSET *)0, (FDSET *)0,
                 NULL) < 0)
          /* errno = EINTR is caused by SRV_HandleSignals */
          if (errno != EINTR)
              perror("Select");

      /* Two things might have happened here.  Either an existing client
       * sent a message to the server, or a new client sent a message to
       * the connect port, trying to join the game.  If the former occurred
       * process it normally.  If the latter occurred, let the client
       * connect, and add its fd to the fd map.  If we are in the middle
       * of a game, send it a message saying this, and then perform a
       * RISK_SelectiveReplicate() so as to get the new client in the
       * same state as the others.  Also send a message to the other
       * clients telling them what is happening.
       */

      if (FD_ISSET(iServerCommLink, &fdSet))
      { /* new client trying to connect */
          Int32 iNewCommLink;

          /* Try to accept the new connection */
          if ((iNewCommLink = accept(iServerCommLink, 0, 0)) < 0) {
              /* Couldn't do it, go back to top */
              printf("%s %s\n",SERVERNAME,SERVER_CONNECT_FAILED);
              continue;
          } else {/* connection went fine */
              /* Does Frisk have enough resources to hold this client? */
              if (CLIENTS_GetNumClients() >= MAX_CLIENTS
                  || iNewCommLink >= MAX_DESCRIPTORS) {
                  (void)RISK_SendMessage(iNewCommLink, MSG_EXIT, NULL);
                  close(iNewCommLink);
                  continue;
              }
          }

          /* Assert: At this point, connection is complete and we have
           * enough resources to keep it around.  Begin connection protocol.
           */

          /* Set options on new CommLink */
          NET_SetCommLinkOptions(iNewCommLink);
          SRV_HandleRegistration(iNewCommLink);
      }
      else  {/* no FD_ISSET,   there is a client trying to send a message */
          for (iClient=0; iClient < MAX_CLIENTS ; iClient++) {
              if (CLIENTS_GetAllocationState(iClient) == ALLOC_COMPLETE &&
                  FD_ISSET(CLIENTS_GetCommLinkOfClient(iClient), &fdSet)) {
                  if (!RISK_ReceiveMessage(CLIENTS_GetCommLinkOfClient(iClient),
                                           &iMessageType, &pvMessage))
                      continue;
                  if ( (iError = SRV_HandleMessage(iClient,iMessageType,pvMessage)) != 0) {
                      printf("error %d when handling messagetype %d from %s\n",iError,iMessageType,CLIENTS_GetAddress(iClient));
                  }
                  break;
              }
          }
      }
    }
}


/**
 * Notify all clients of turn change
 *
 * \b History:
 * \arg 08.27.94  ESF  Created.
 * \arg 30.08.95  JC   Send a message if the player have too many cards.
 * \arg 25.12.99  MSH  Functionality of previous entry seems to have vanished
 */
void SRV_NotifyClientsOfTurn(Int32 iPlayer)
{
  MsgTurnNotify msgTurnNotify;

  /* Let all clients know whos turn it is */
  msgTurnNotify.iPlayer = iPlayer;
  msgTurnNotify.iClient = RISK_GetClientOfPlayer(iPlayer);
  SRV_BroadcastMessage(MSG_TURNNOTIFY, &msgTurnNotify);
}
  

/**
 * Distribute countries among live players.
 *
 * \b History:
 * \arg 05.12.94  ESF  Created.
 * \arg 07.25.94  ESF  Changed to support TEST_GAMEs.
 * \arg 10.30.94  ESF  Changed to support TEST_GAMEs better.
 * \arg 10.30.94  ESF  Changed to refer to LivePlayer instead of Player.
 */
void SRV_DistributeCountries(void)
{
  Deck   *pCountryDeck = DECK_Create(NUM_COUNTRIES);
  Int32   iCountry, iPlayer, i;

  /* Dole out the countries */
  for(i=0; i!=NUM_COUNTRIES; i++)
    {
      /* Pick a country, any country */
      iCountry = DECK_GetCard(pCountryDeck);

#ifdef TEST_GAME
      /* Give countries to the first player, leave one for the rest */
      iPlayer = (i <= NUM_COUNTRIES-RISK_GetNumLivePlayers()) 
	  ? RISK_GetNthLivePlayer(0) 
	  : RISK_GetNthLivePlayer(i-(NUM_COUNTRIES-RISK_GetNumLivePlayers()));
#else      
      iPlayer = iTurn;
#endif

      /* Update the game object */
      RISK_SetOwnerOfCountry(iCountry, iPlayer);
      RISK_SetNumCountriesOfPlayer(iPlayer, 
				   RISK_GetNumCountriesOfPlayer(iPlayer)+1);
      RISK_SetNumArmiesOfCountry(iCountry, 1);
      RISK_SetNumArmiesOfPlayer(iPlayer, RISK_GetNumArmiesOfPlayer(iPlayer)-1);

      /* Iterate to next player */
      (void)SRV_IterateTurn();
    }

#ifdef TEST_GAME
  /* Set the number of armies to be small, to let the game start soon. */
  for (i=0; i!=RISK_GetNumLivePlayers(); i++)
    RISK_SetNumArmiesOfPlayer(RISK_GetNthLivePlayer(i), 2);
#endif
  
  DECK_Destroy(pCountryDeck);  
}


/**
 * Calculate number of armies given to each player at game start
 *
 * \b History:
 * \arg 08.27.94  ESF  Created.
 * \arg 12.07.95  ESF  Fixed initial number of armies to be by the rules.
 *
 * \bug Part of rules is hardcoded here
 */
void SRV_SetInitialArmiesOfPlayers(void)
{
  Int32 i, iPlayer, iNumArmies;
  const Int32 iNumPlayers = RISK_GetNumPlayers();

  D_Assert(iNumPlayers >= 2, "Not enough players!");

  /* Calculate the number of armies. */
  iNumArmies = MAX( 50 - iNumPlayers*5, /* According to the rules */
		    NUM_COUNTRIES/iNumPlayers+1 ); /* At least one per country */

  /* Make sure it's enough */
  D_Assert(iNumPlayers*iNumArmies >= NUM_COUNTRIES, "Not enough armies!");

  /* Set the initial number of armies for all the players */
  for (i=0; i<RISK_GetNumPlayers(); i++)
    {
      iPlayer = RISK_GetNthPlayer(i);
      RISK_SetNumArmiesOfPlayer(iPlayer, iNumArmies);

      /* Sanity check */
      D_Assert(RISK_GetNumArmiesOfPlayer(iPlayer) > 0,
	       "Number of armies is negative!");
    }
}


/**
 * Set initial mission of all players to "no mission"
 *
 * \b History:
 * \arg 24.08.95  JC   Created.
 */
void SRV_SetInitialMissionOfPlayers(void)
{
  Int32 i, iPlayer, nb;

  nb = RISK_GetNumPlayers();
  /* Set the initial mission's type for all the players */
  for (i=0; i<nb; i++)
    {
      iPlayer = RISK_GetNthPlayer(i);
      RISK_SetMissionTypeOfPlayer(iPlayer, NO_MISSION);
    }
}


/**
 * ????????
 *
 * \b History:
 * \arg 24.08.94  JC   Created.
 *
 * \bug (MSH 25.12.99) Obscure code with no comments!
 */
Flag SRV_DistributeMissions(void)
{
  Int32 iMission[MAX_PLAYERS];
  Int32 iPlayer, i, j, cont1, cont2, n, nb, m;

  nb = RISK_GetNumLivePlayers();
  for (n = 0; n<nb; n++)
    {
      iPlayer = RISK_GetNthPlayer(n);
      if (RISK_GetMissionTypeOfPlayer(iPlayer) != NO_MISSION)
          return FALSE;
    }
  for (n = 0; n<MAX_PLAYERS; n++)
      iMission[n] = -1;
  for (n = 0; n<nb; n++)
    {
      iPlayer = RISK_GetNthLivePlayer(n);
      i = rand() % (2 + (NUM_CONTINENTS * (NUM_CONTINENTS - 1))/2 + nb - 1);
      j = 0;
      while (j<MAX_PLAYERS)
        {
          if (i == iMission[j])
            {
              j = 0;
              i = (i + 1) %(2 + NUM_CONTINENTS * (NUM_CONTINENTS - 1)
                            + nb - 1);
            }
          else
              j++;
        }
      iMission[iPlayer]=i;
      if (i == 0)
          RISK_SetMissionTypeOfPlayer(iPlayer, CONQUIER_WORLD);
      else if (i == 1)
        {
          RISK_SetMissionTypeOfPlayer(iPlayer, CONQUIER_Nb_COUNTRY);
          RISK_SetMissionNumberOfPlayer(iPlayer, NUM_COUNTRIES / 2);
        }
      else if (i <= ((NUM_CONTINENTS * (NUM_CONTINENTS - 1))/2 + 1))
        {
          i = i - 2;
          cont1 = -1;
          m = 0;
          while (cont1 == -1)
            {
              cont2 = m + i + 1;
              if (cont2 < NUM_CONTINENTS)
                  cont1 = m;
              i = i - (NUM_CONTINENTS - m - 1);
              m++;
            }
          RISK_SetMissionTypeOfPlayer(iPlayer, CONQUIER_TWO_CONTINENTS);
          RISK_SetMissionContinent1OfPlayer(iPlayer, cont1);
          RISK_SetMissionContinent2OfPlayer(iPlayer, cont2);
        }
      else
        {
          i = i - (NUM_CONTINENTS * (NUM_CONTINENTS - 1))/2 - 2;
          j = RISK_GetNthLivePlayer(i);
          if (j == iPlayer)
            {
              i++;
              i = i % nb;
              j = RISK_GetNthLivePlayer(i);
            }
          RISK_SetMissionTypeOfPlayer(iPlayer, KILL_A_PLAYER);
          RISK_SetMissionMissionPlayerToKillOfPlayer(iPlayer, j);
          RISK_SetMissionPlayerIsKilledOfPlayer(iPlayer, FALSE);
        }
    }
  return TRUE;
}


/**
 * Broadcast given message to all active clients.
 *
 * \b History:
 * \arg 02.05.94  ESF  Created.
 * \arg 08.16.94  ESF  Rewrote a bit.
 */
void SRV_BroadcastMessage(Int32 iMessType, void *pvMessage)
{
  Int32 i;
  
  /* If there are no clients, then do nothing */
  if (CLIENTS_GetNumClients() == 0)
    return;

  /* Loop through and send the message to each active client */
  for (i=0; i!=MAX_CLIENTS; i++)
    if (CLIENTS_GetAllocationState(i) == ALLOC_COMPLETE)
      (void)RISK_SendMessage(CLIENTS_GetCommLinkOfClient(i), iMessType, pvMessage);
}


/**
 * Broadcast given message to all active clients except originator.
 *
 * \b History:
 * \arg 03.28.94  ESF  Created.
 * \arg 08.16.94  ESF  Rewrote a bit.
 * \arg 10.02.94  ESF  Fixed a heinous bug, was going 'till NumClients.
 */
void SRV_CompleteBroadcast(Int32 iClientExclude, Int32 iMessageType, 
			   void *pvMess)
{
  Int32 i;

  for (i=0; i!=MAX_CLIENTS; i++)
    if (i!=iClientExclude && 
	CLIENTS_GetAllocationState(i) == ALLOC_COMPLETE)
        (void)RISK_SendMessage(CLIENTS_GetCommLinkOfClient(i), iMessageType, pvMess);
}


/**
 * Send message to all clients. Handles server-originated messages and
 * relaying of client-originated messages.
 *
 * \b History:
 * \arg 08.18.94  ESF  Created.
 */
void SRV_Replicate(Int32 iMessType, void *pvMess, Int32 iType, Int32 iSrc)
{
  if (iType == MESS_OUTGOING)
    SRV_BroadcastMessage(iMessType, pvMess);
  else /* (iType == MESS_INCOMING) */
    {
      /* Convert the CommLink to a client */
      SRV_CompleteBroadcast(CLIENTS_GetCommLinkToClient(iSrc), iMessType, pvMess);
    }
}


/**
 * Server exit cleanup.
 *
 * \b History:
 * \arg 06.10.94  ESF  Created.
 */
void SRV_HandleEXIT(Int32 iExitValue)
{
  Int32 n;

  /* close the listening socket */
  close(iServerCommLink);

  /* close all of the player sockets */
  for(n=0; n!=MAX_CLIENTS; n++)
    if (CLIENTS_GetAllocationState(n) == ALLOC_COMPLETE)
      {
        RISK_SendMessage(CLIENTS_GetCommLinkOfClient(n), MSG_EXIT, NULL);
        close(CLIENTS_GetCommLinkOfClient(n));
      }

  UTIL_ExitProgram(iExitValue);
}


/**
 * Handles allocation of new player.
 *
 * \b History:
 * \arg 06.10.94  ESF  Created.
 * \arg 10.04.94  ESF  Fixed a bug, creating player not a transaction.
 * \arg 04.01.95  ESF  Changed to use a more robust method.  Simplified.
 */
void SRV_HandleALLOCPLAYER(Int32 iClient)
{
  MsgReplyPacket  mess;
  Int32           iPlayer;
  
  /* Get a player and return it, or -1 if none available */
  iPlayer = mess.iReply = DECK_GetCard(pPlayerDeck);
  
  /* If there was a valid player, make a note */
  if (iPlayer != -1)
    {
      /* Reset all of the player fields */
      RISK_SetAttackModeOfPlayer(iPlayer, 1);
      RISK_SetStateOfPlayer(iPlayer, PLAYER_ALIVE);
      RISK_SetClientOfPlayer(iPlayer, -1);
      RISK_SetNumCountriesOfPlayer(iPlayer, 0);
      RISK_SetNumArmiesOfPlayer(iPlayer, 0);
      RISK_SetNumCardsOfPlayer(iPlayer, 0);
      RISK_SetMissionTypeOfPlayer(iPlayer, NO_MISSION);

      /* Note that the player is in the process of being allocated.
       * The client will complete the procedure.  The client that
       * sets the allocation state to ALLOC_COMPLETE is the one
       * responsible for increasing the number of players.
       */

      RISK_SetAllocationStateOfPlayer(iPlayer, ALLOC_INPROGRESS);
    }
  
  RISK_SendMessage(CLIENTS_GetCommLinkOfClient(iClient), MSG_REPLYPACKET, &mess);
}


/**
 * (MSH 25.12.99) What is this???
 *
 * \b History:
 * \arg 06.10.94  ESF  Created.
 */
void SRV_HandleREPLYPACKET(void *pvMessage)
{
  iReply = ((MsgReplyPacket *)pvMessage)->iReply; 
}


/**
 * Handles player resignation during game.
 * \bug Terminates the game - recovery procedures needed!
 *
 * \b History:
 * \arg 06.10.94  ESF  Created.
 * \arg 10.15.94  ESF  Fixed a bug, only Set LivePlayers if player is alive.
 * \arg 11.06.94  ESF  Fixed a bug, return player's cards to server.
 */
void SRV_HandleFREEPLAYER(void *pvMessage)
{
  Int32 iPlayer, i;
  char buf[256];

  /* Put the player ID back onto pool of free players */
  iPlayer = ((MsgFreePlayer *)(pvMessage))->iPlayer;
  DECK_PutCard(pPlayerDeck, iPlayer);

  /* If the player has cards at this point, take them away and put
   * them back onto the card deck.
   */

  for (i=0; i!=RISK_GetNumCardsOfPlayer(iPlayer); i++)
    DECK_PutCard(pCardDeck, RISK_GetCardOfPlayer(iPlayer, i));
 
  /* Details, details... */
  RISK_SetAllocationStateOfPlayer(iPlayer, ALLOC_NONE);

  /* If there were no live players at the client then we can let it go
   * without ending the game.  Otherwise, end the game if we're in the
   * PLAY or FORTIFICATION stages of the game.  If the number of live
   * players went down to 0, then we must go back to registering mode.
   * Kind of moot, but safe... 
   */

  /* Was the player alive? */
  if (RISK_GetStateOfPlayer(iPlayer) == PLAYER_ALIVE &&
      (iServerMode == SERVER_PLAYING ||
       iServerMode == SERVER_FORTIFYING))
    {
      MsgNetPopup msgNetPopup;
      
      /* Informative message */
      snprintf(buf, sizeof(buf),GAME_OVER);
      msgNetPopup.strMessage = buf;
      SRV_BroadcastMessage(MSG_NETPOPUP, &msgNetPopup);

      /* There was no winner, reset the game */
      SRV_BroadcastMessage(MSG_ENDOFGAME, NULL);

      /* It was the end of the game, so reset things */
      SRV_ResetGame();
    }
  else if (RISK_GetNumLivePlayers() == 0)
    SRV_ResetGame();
}


/**
 * Sends message from client to all clients, all but originator or
 * specified recipient.
 *
 * \b History:
 *     06.10.94  ESF  Created.
 */
void SRV_HandleMESSAGEPACKET(Int32 iClient, void *pvMessage)
{
  MsgMessagePacket *pMess = (MsgMessagePacket *)pvMessage;
  
  if (pMess->iTo == DST_ALLPLAYERS)
    SRV_BroadcastMessage(MSG_MESSAGEPACKET, pvMessage);
  else if (pMess->iTo == DST_ALLBUTME)
    SRV_CompleteBroadcast(iClient, MSG_MESSAGEPACKET, pvMessage);
  else
    (void)RISK_SendMessage(CLIENTS_GetCommLinkOfClient(
                               RISK_GetClientOfPlayer(pMess->iTo)), 
			   MSG_MESSAGEPACKET, pvMessage);
}


/**
 * Saves distributed server object state received from client.
 * \bug (MSH 25.12.99) Will figure out a better way to do this.
 *
 * \b History:
 * \arg 06.10.94  ESF  Created.
 */
void SRV_HandleENTERSTATE(void *pvMessage)
{
  iState = ((MsgEnterState *)pvMessage)->iState; 
}


/**
 * Helper macro for cleaning up aborted registration process.
 */
#define _SRV_CleanupRegistration() \
  /* NET_DeleteMessage(iMessType, pvMess); */ \
  FD_CLR(iCommLink, &fdBackup);\
    close(iCommLink);


/**
 * Handles registration of a new client.
 * \b Note: contains code for connection re-estabilishment (???)
 * 
 * \b History:
 * \arg 06.10.94  ESF  Created.
 * \arg 08.31.94  ESF  Changed to use only one socket.
 * \arg 09.28.94  ESF  Fixed to handle errant clients correctly (free 'em).
 * \arg 09.28.94  ESF  Fixed the process of filling in the new clients.
 * \arg 09.31.94  ESF  Added notification of current turn for new clients.
 * \arg 10.01.94  ESF  Fixed to check for failure of ReceiveMessage.
 * \arg 10.15.94  ESF  Fixed, was freeing client that I wasn't allocating.
 * \arg 10.30.94  ESF  Added quotes to the "A new client..." message.
 * \arg 01.01.95  ESF  Fixed a bug in notifying clients of current player.
 * \arg 01.15.95  ESF  Initilized iTo field to avoid uninitialized memory.
 * \arg 01.15.95  ESF  Fixed memory leak.
 * \arg 02.21.95  ESF  Add support for computer players.
 * \arg 02.21.95  ESF  Cleaned up, fixed minor bugs.
 * \arg 25.08.95  JC   Add || (RISK_GetNumSpecies() > 1) because the
 *                     presence of AI-Client which have no player.
 */
void SRV_HandleRegistration(Int32 iCommLink)
{
  Int32                 iMessType;
  MsgClientIdent        msgClientIdent;
  MsgVersion            msgVersion;
  MsgMessagePacket      msgMess;
  void                 *pvMess, *pvDeleteMe = NULL;
  Int32                 iNewClient, iClientType = CLIENT_NORMAL;
  Int32                 iClientSpecies = SPECIES_HUMAN;
  Flag                  fOldClient = FALSE;
  CString               strClientAddress;
  char buf[256];

  /* The Protocol:
   *
   *                      Receive MSG_HELLO           --> OK!
   *                              Other               --> Protocol bogosity.
   *  Send MSG_VERSION. 
   *                      Receive MSG_REGISTERCLIENT  --> OK!
   *                              MSG_VERSION         --> Version mismatch.
   *                              Other               --> Protocol bogosity.
   *  Send MSG_CLIENTIDENT if server is not full.
   *  Send MSG_CLIENTEXIT  if server is full.
   */

  /* It should be a MSG_HELLO! */
  if (!RISK_ReceiveMessage(iCommLink, &iMessType, &pvMess) ||
      (iMessType != MSG_HELLO && iMessType != MSG_OLDREGISTERCLIENT))
  {
      printf("%s: %s %d %s\n",SERVERNAME,ERROR_PROTOCOL,iMessType,IGNORE_CLIENT);
      _SRV_CleanupRegistration();
      return;
    }

  /* See if it's an old client */
  fOldClient = (iMessType == MSG_OLDREGISTERCLIENT);
  
  /* If it is an old client, then get its name */
  if (fOldClient)
    {
      strClientAddress = ((MsgOldRegisterClient *)pvMess)->strClientAddress;
      
      /* Mark this message for future deletion */
      pvDeleteMe = pvMess;
    }
  else
    NET_DeleteMessage(MSG_HELLO, pvMess);

  /* If we're talking to a new client, send the version information */
  if (!fOldClient)
    {
      /* Send the version */
      msgVersion.strVersion = VERSION;
      (void)RISK_SendMessage(iCommLink, MSG_VERSION, &msgVersion);
      
      /* It should be a MSG_REGISTERCLIENT or a MSG_VERSION! */
      if (!RISK_ReceiveMessage(iCommLink, &iMessType, &pvMess) ||
	  iMessType != MSG_REGISTERCLIENT)
	{
            if (iMessType == MSG_VERSION)
                printf("%s: %s (%s)\n",SERVERNAME,CLIENT_MISMATCH,((MsgVersion *)pvMess)->strVersion);
            else
                printf("%s: %s (%d) %s\n",SERVERNAME,ERROR_PROTOCOL,iMessType,IGNORE_CLIENT);
	  _SRV_CleanupRegistration();
	  return;
	}

      /* Now we know the name (and type) of the client */
      strClientAddress = ((MsgRegisterClient *)pvMess)->strClientAddress;
      iClientType      = ((MsgRegisterClient *)pvMess)->iClientType;
    }

  /* If the client is an "AI" client, then send it the 
   * dynamic ID of its species, which we allocate on-the-fly. 
   */

  if (iClientType == CLIENT_AI)
    {
      MsgSpeciesIdent msg;

      iClientSpecies = msg.iSpeciesID = SRV_AllocSpecies();
      (void)RISK_SendMessage(iCommLink, MSG_SPECIESIDENT, &msg);
    }
  
  /* Get the handle to a new client */
  iNewClient = CLIENTS_Alloc();
  
  /* Did the allocation fail? */
  if (iNewClient == -1)
  {
      printf("%s: %s\n",SERVERNAME,NO_ROOM);
      (void)RISK_SendMessage(iCommLink, MSG_EXIT, NULL);
      _SRV_CleanupRegistration();
      return;
    }

  /* Send its ID */
  msgClientIdent.iClientID = iNewClient;
  (void)RISK_SendMessage(iCommLink, MSG_CLIENTIDENT, &msgClientIdent);
  printf("%s: %s \"%s\" %s\n",SERVERNAME,NEW_CLIENT,strClientAddress, HAS_REGISTERED);

  /* Set up its fields */
  CLIENTS_SetCommLink(iNewClient, iCommLink);


  CLIENTS_SetAddress(iNewClient, strClientAddress);
  
  CLIENTS_SetType(iNewClient, iClientType);
  CLIENTS_SetSpecies(iNewClient, iClientSpecies);
  
  /* Remember to look for messages from it with select(), 
   * and update the maximum file descriptor.
   */
  
  FD_SET(iCommLink, &fdBackup);
  iMaxFileDescUsed = MAX(iMaxFileDescUsed, iCommLink);
  

  /* Let the new client know about old clients, if needed */
  /* should be passed client...*/
  CLIENTS_TellAboutOthers(iCommLink,iClientType);

  /* Let all other clients know about it */
  snprintf(buf, sizeof(buf),"%s %s",iClientType == CLIENT_AI ? NEW_AI:NEW_CLIENT,HAS_REGISTERED);
  msgMess.strMessage = buf;
  msgMess.iFrom      = FROM_SERVER;
  msgMess.iTo        = DST_OTHER;
  SRV_CompleteBroadcast(iNewClient, MSG_MESSAGEPACKET, &msgMess);

  /* And tell the client that just joined about itself */
  snprintf(buf, sizeof(buf),"%s \"%s\".",HELLO_NEW,CLIENTS_GetAddress(iNewClient));
  RISK_SendMessage(CLIENTS_GetCommLinkOfClient(iNewClient),
		   MSG_MESSAGEPACKET, &msgMess);

  /* Tell the new client to pop up its registration box, if we
   * are not in game playing mode.
   */
  
  if (iServerMode == SERVER_REGISTERING && iClientType != CLIENT_AI)
    (void)RISK_SendMessage(iCommLink, MSG_POPUPREGISTERBOX, NULL);


  /* If there are any players, let the new client 
   * know about them, so that it can set up the 
   * colors and other things.
   */
  
  if (RISK_GetNumPlayers() || (RISK_GetNumSpecies() > 1))
    {
      /* Let the clients know the situation */
        snprintf(buf, sizeof(buf),"%s %s",CLIENT,BEING_UPDATED);
      SRV_BroadcastTextMessage(buf);
      
      if (iServerMode == SERVER_REGISTERING)
	SRV_ReplicateRegistrationData(iCommLink);
      else if (iServerMode == SERVER_PLAYING ||
	       iServerMode == SERVER_FORTIFYING)
	{
	  MsgTurnNotify  msgTurnNotify;
	  
	  SRV_ReplicateAllData(iCommLink);
	  
	  /* Let the client know who's turn it is currently */
	  msgTurnNotify.iPlayer = iTurn;
	  msgTurnNotify.iClient = 
	    RISK_GetClientOfPlayer(msgTurnNotify.iPlayer);
	  (void)RISK_SendMessage(iCommLink, MSG_TURNNOTIFY, 
				 &msgTurnNotify);
	}
    }      
  
  /* Don't need these anymore */
  if (pvDeleteMe)
    NET_DeleteMessage(MSG_OLDREGISTERCLIENT, pvDeleteMe);
  else
    NET_DeleteMessage(MSG_REGISTERCLIENT, pvMess);
}


/**
 * Handles client deregistration.
 *
 * \b History:
 * \arg 06.10.94  ESF  Created.
 * \arg 08.31.94  ESF  Revamped, made more sophisticated.
 * \arg 09.30.94  ESF  Fixed bug, freeing player before MSG_DELETEMSGDST.
 * \arg 09.31.94  ESF  Fixed so that FreeClient is called last.
 * \arg 10.01.94  ESF  Fixed so that it doesn't return too soon.
 * \arg 01.15.95  ESF  Removed MSG_DELETEMSGDST stuff.
 * \arg 01.24.95  ESF  Fixed bug, reset iServerMode if SRV_NumClients()==1.
 */
void SRV_HandleDEREGISTERCLIENT(Int32 iClient)
{
  /*
   * This isn't used and the operation has no side effects...
   * const Int32 iNumPlayersOfClient = RISK_GetNumLivePlayersOfClient(iClient);
   */

  D_Assert(iClient>=0 && iClient<MAX_CLIENTS, "Bogus client!");
  /* We need to free the client */
  SRV_FreeClient(iClient);
}


/**
 * Terminates the program.
 *
 * \b History:
 *     06.16.94  ESF  Created.
 */
void UTIL_ExitProgram(Int32 iExitValue)
{
  MEM_TheEnd();

  /* Don't let any more data be received on this socket */
  shutdown(iServerCommLink, 0);

  exit(iExitValue);
}


/**
 * Server signal handler.
 *
 * \b History:
 * \arg 07.31.94  ESF  Created.
 * \arg 24.08.95  JC   Don't quit if a game is started, but remember.
 */
void SRV_HandleSignals(Int32 iParam)
{
  Int32          i;

#ifdef ENGLISH
  /* printf("Ouch, I've been killed...\n"); */
  printf("O! I am slain!\n");
#endif
#ifdef FRENCH
  printf("Ouch, Je suis tué...\n");
#endif

  if (    (    (iServerMode != SERVER_FORTIFYING)
            && (iServerMode != SERVER_PLAYING   ))
       || (    (iParam != SIGHUP ) && (iParam != SIGINT)
            && (iParam != SIGQUIT) && (iParam != SIGKILL))
       || (CLIENTS_GetNumClients() <= 0))
      SRV_HandleEXIT(-1);

  for (i=0; i!=MAX_CLIENTS; i++)
    if (    (CLIENTS_GetAllocationState(i) == ALLOC_COMPLETE)
         && (CLIENTS_GetStartState(i) == TRUE))
      {
        RISK_SendMessage(CLIENTS_GetCommLinkOfClient(i),
                         MSG_DEREGISTERCLIENT, NULL);
        close(CLIENTS_GetCommLinkOfClient(i));
      }
  fRememberKilled = TRUE;
  select(iMaxFileDescUsed+1, (FDSET *)&fdSet, (FDSET *)0, (FDSET *)0, NULL);
  signal(iParam, SRV_HandleSignals);
}


/**
 * Replicate species and player registration data to new client.
 * Called by SRV_ReplicateAllData(), do not use directly.
 *
 * \b History:
 * \arg 08.28.94  ESF  Created.
 * \arg 02.27.95  ESF  Added Species.
 * \arg 25.08.95  JC   Moved Species before.
 * \arg 29.08.95  JC   Don't send always the first allocated species.
 */
void SRV_ReplicateRegistrationData(Int32 iCommLinkDest)
{
  Int32 i, iPlayer, iSpecies;

  /* Ignore the "human" entry... */
  for (i=iSpecies=1; i<RISK_GetNumSpecies(); i++, iSpecies++)
    {
      while (RISK_GetAllocationStateOfSpecies(iSpecies) == ALLOC_NONE)
	iSpecies++;

      RISK_SelectiveReplicate(iCommLinkDest, SPE_NAME, iSpecies, 0);
      RISK_SelectiveReplicate(iCommLinkDest, SPE_VERSION, iSpecies, 0);
      RISK_SelectiveReplicate(iCommLinkDest, SPE_DESCRIPTION, iSpecies, 0);
      RISK_SelectiveReplicate(iCommLinkDest, SPE_AUTHOR, iSpecies, 0);
      RISK_SelectiveReplicate(iCommLinkDest, SPE_CLIENT, iSpecies, 0);
      RISK_SelectiveReplicate(iCommLinkDest, SPE_ALLOCATION, iSpecies, 0);
    }

  for (i=0; i<RISK_GetNumPlayers(); i++)
    {
      iPlayer = RISK_GetNthPlayer(i);
      
      RISK_SelectiveReplicate(iCommLinkDest, PLR_STATE, iPlayer, 0);
      RISK_SelectiveReplicate(iCommLinkDest, PLR_COLORSTRING, iPlayer, 0); 
      RISK_SelectiveReplicate(iCommLinkDest, PLR_NAME, iPlayer, 0);
      RISK_SelectiveReplicate(iCommLinkDest, PLR_SPECIES, iPlayer, 0);
      RISK_SelectiveReplicate(iCommLinkDest, PLR_CLIENT, iPlayer, 0);
      RISK_SelectiveReplicate(iCommLinkDest, PLR_ALLOCATION, iPlayer, 0);
    }
}


/**
 * Replicates all game state data to new client.
 *
 * \b History:
 * \arg  08.28.94  ESF  Created.
 * \arg  02.27.95  ESF  Added Species.
 */
void SRV_ReplicateAllData(Int32 iCommLinkDest)
{
  Int32 i, j, iPlayer;

  /* First replicate the registration data */
  SRV_ReplicateRegistrationData(iCommLinkDest);

  /* Now everything else */
  for (i=0; i<RISK_GetNumPlayers(); i++)
    {
      iPlayer = RISK_GetNthPlayer(i);
      
      RISK_SelectiveReplicate(iCommLinkDest, PLR_NUMCOUNTRIES, iPlayer, 0);
      RISK_SelectiveReplicate(iCommLinkDest, PLR_NUMARMIES, iPlayer, 0);
      RISK_SelectiveReplicate(iCommLinkDest, PLR_NUMCARDS, iPlayer, 0);
      
      for (j=0; j!=RISK_GetNumCardsOfPlayer(iPlayer); j++)
	RISK_SelectiveReplicate(iCommLinkDest, PLR_CARD, iPlayer, j);
    }

  for (i=0; i!=NUM_COUNTRIES; i++)
    {
      RISK_SelectiveReplicate(iCommLinkDest, CNT_OWNER, i, 0);
      RISK_SelectiveReplicate(iCommLinkDest, CNT_NUMARMIES, i, 0);
    }
}

/**
 * Totally removes the given client from the game.
 *
 * \b History:
 * \arg 05.12.94  ESF  Created.
 * \arg 08.08.94  ESF  Moved here from server.c.
 * \arg 08.08.94  ESF  Fixed to delete players at client.
 * \arg 09.09.94  ESF  Fixed to take into consideration side effects.
 * \arg 09.13.94  ESF  Fixed loop, was doing i<=0, changed to i>=0 :)
 * \arg 09.14.94  ESF  Fixed to deal with iNumClientsStarted.
 * \arg 03.25.95  ESF  Added species considerations.
 * \arg 24.08.95  JC   Quit if no client and fRememberKilled == TRUE.
 *
 * \b Notes:
 * \arg tony: to move to clients.c, fdbackup is problem
 */
void SRV_FreeClient(Int32 iClient)
{
  Int32            i, iNumPlayers;
  MsgFreePlayer    msgFreePlayer;

  /* Get rid of the client.  We do this first so that broadcasts
   * won't include this client.  This would normally result in
   * a SIGPIPE, if the exiting client had already shut down its
   * sockets, but since we ignore SIGPIPE, it would result in
   * a failed broadcast, which is technically wrong.
   */

  CLIENTS_SetAllocationState(iClient, ALLOC_NONE);
  CLIENTS_SetNumClients(CLIENTS_GetNumClients()-1);

  /* Note that the client won't be participating in the game */
  CLIENTS_SetStartState(iClient, FALSE);

  /* Reset the fields, close the sockets */
  close(CLIENTS_GetCommLinkOfClient(iClient));

  /* Remove the socket from the fd_set */
  FD_CLR(CLIENTS_GetCommLinkOfClient(iClient), &fdBackup);

  /* Free all of the players at the client that is being nixed.
   * The client would call CLNT_FreePlayer to do this, which
   * is an RPC type call that sends MSG_FREEPLAYER to the
   * server (that's me!)  So what we do here is manufacture a
   * MSG_FREEPLAYER and call the handler for it.  A bit roundabout,
   * perhaps, but nevertheless a consistant way of dealing with
   * freeing players.  Could probably be improved.  Notice that
   * we get the Nth player, and _not_ the Nth live player.
   * Note also, that freeing a player has the side effect of
   * changing the value of RISK_GetNumPlayers().
   */

  iNumPlayers = RISK_GetNumPlayers();

  for (i=iNumPlayers-1; i>=0; i--)
    if (RISK_GetClientOfPlayer(RISK_GetNthPlayer(i)) == iClient)
      {
	msgFreePlayer.iPlayer = RISK_GetNthPlayer(i);
	SRV_HandleFREEPLAYER((void *)&msgFreePlayer);
      }

  /* Additionally, if the client is an AI client, then we have to
   * remove the species from the list of species.
   */

  if (CLIENTS_GetType(iClient) == CLIENT_AI)
    SRV_FreeSpecies(CLIENTS_GetSpecies(iClient));

  if (fRememberKilled && (CLIENTS_GetNumClientsStarted() <= 0))
      SRV_HandleEXIT(-1);

  if (iServerMode == SERVER_REGISTERING)
    {
      /* Reset the game, and possible start it */
      SRV_ResetGame();
      SRV_AttemptNewGame();
    }
}


/**
 * Log a client failure.
 *
 * \b History:
 * \arg 10.01.94  ESF  Created.
 */
void SRV_LogFailure(CString strReason, Int32 iCommLink)
{
  const Int32 iClient = CLIENTS_GetCommLinkToClient(iCommLink);

  D_Assert(iClient >= -1 && iClient < MAX_CLIENTS, "Bogus client...");

  /* This stops any more bogus messages from coming in from the 
   * dead client, which probably has nothing interesting to say
   * anyway...
   */
  
  FD_CLR(iCommLink, &fdBackup);

  /* If this is -1, then it is not a known client */
  if (iClient == -1)
    {
      /* This shouldn't happen much */
      printf(ERR_COMMFAILED);
      close(iCommLink);
    }
  else
    {
        /* Log the failure, to be dealt with later. */
        CLIENTS_SetFailure(iClient,strReason);
    }
}


/**
 * Tries to recover all client failures.
 *
 * \b History:
 * \arg 08.28.94  ESF  Created.
 * \arg 08.31.94  ESF  Revised to do something.
 * \arg 09.29.94  ESF  Fixed to deregister client in case of failure.
 * \arg 09.30.94  ESF  Fixed to call the right deregistration function.
 * \arg 09.31.94  ESF  Added MSG_NETMESSAGE to be sent also.
 * \arg 10.01.94  ESF  Changed to reflect new failure handling.
 */
void SRV_RecoverFailures(void)
{
    Int32 iClient;
    Int32 count;
  char buf[256];

  /* An optimization */
  if (CLIENTS_GetNumFailures() == 0)
    return;

  /* Go through all of the failures and handle them */
  for (iClient=0; iClient!=MAX_CLIENTS; iClient++)
    if (CLIENTS_GetFailureCount(iClient) > 0)
      {
	MsgNetPopup       msgNetPopup;
	MsgMessagePacket  msgMessagePacket;
	
	/* It is in the list of clients, so we need to destroy it.
	 * Make as if we received a MSG_DEREGISTERCLIENT.
	 */
	
	SRV_HandleDEREGISTERCLIENT(iClient);
	
	/* Tell the clients what's happening (N.B. this broadcast call
	 * has to be made AFTER the CLIENTS_Free call, otherwise, the
	 * broadcast will try to send a message to the client that
	 * just failed, and we will enter a recursive situation.)
	 */

        count = CLIENTS_GetFailureCount(iClient);
	/* Information */
#ifdef ENGLISH
	snprintf(buf, sizeof(buf), "\"%s\" has failed (%d failure%s).",
#endif
#ifdef FRENCH
	snprintf(buf, sizeof(buf), "%s a échoué (%d échec%s)",
#endif
		CLIENTS_GetFailureReason(iClient),
		count, count >1 ?
		"s" : "");

	printf("%s: %s\n",SERVERNAME, CLIENTS_GetFailureReason(iClient));
	msgNetPopup.strMessage = buf;
	SRV_BroadcastMessage(MSG_NETPOPUP, &msgNetPopup);
	
	msgMessagePacket.strMessage = buf;
	msgMessagePacket.iFrom      = FROM_SERVER;
	msgMessagePacket.iTo        = DST_ALLPLAYERS;
	SRV_BroadcastMessage(MSG_MESSAGEPACKET, &msgMessagePacket);
	
        /* Pheww... Finished. */
        printf(TXT_HANDLED_FAILURE);
	CLIENTS_ResetFailure(iClient);
      }

  /* Reset this */
  CLIENTS_SetNumFailures(0);
}


/**
 * Determines the next player to receive the turn.
 *
 * \b History:
 * \arg 10.29.94  ESF  Created.
 * \arg 11.27.94  ESF  Fixed a really stupid and serious bug.
 */
Int32 SRV_IterateTurn(void)
{
  /* It may be that the player who's turn it just was killed a few
   * players, and the number of live players has gone down.  The way
   * players are organized is as if they were stacked on one top of
   * the next, with the 0th live player at the bottom.  In other words,
   * if the 2nd live player gets killed, then the 3rd live player becomes
   * the 2nd live player, etc.  This is done so that one can loop from the
   * [0th -- NumLivePlayer()th] live player.  However, when calculating 
   * turns, this may screw us up.  If is the 2nd live player's turn, and
   * during this turn the 1st live player gets killed, then the next turn
   * belongs to the 2nd live player (who used to be the 3rd live player).
   * So to calculate the next turn, we iterate through the players, NOT
   * the live players.  We simply search for the next live player.  In
   * case this sounds obvious, it probably is, but the code use to read:
   * "iTurn = (iTurn+1) % RISK_GetNumLivePlayers()" which is obviously wrong.
   */

  do 
    {
      /* Go to the next player, wrap-around if we're at the end */
      iTurn = (iTurn+1) % MAX_PLAYERS;
    } 
  while (RISK_GetStateOfPlayer(iTurn) == FALSE ||
	 RISK_GetAllocationStateOfPlayer(iTurn) != ALLOC_COMPLETE);

  /* Return the player who's turn it is */
  return iTurn;
}


/**
 * Starts the allocation of a new species.
 *
 * \b History:
 * \arg 02.23.95  ESF  Created.
 */
Int32 SRV_AllocSpecies(void)
{
  /* Simply loop through the species looking for an empty slot.
   * Since they are created on demand, if the list is full, when
   * we get to the end the Dist. Obj. will build us a new one.
   */

  Int32 i;

  for (i=0; 
       RISK_GetAllocationStateOfSpecies(i) != ALLOC_NONE;
       i++)
    ; /* TwiddleThumbs */

  /* Mark the species as allocated.  Otherwise, if the requesting ai Client
   * hasn't done anything with the new species, and another ai Client 
   * is asking for one, the first ai Client doesn't set the name to be 
   * something non-NULL, then the server will return the SAME species 
   * number -- race condition!
   */

  RISK_SetAllocationStateOfSpecies(i, ALLOC_INPROGRESS);

  return i;
}


/**
 * Frees a species.
 *
 * \b History:
 * \arg 02.32.95  ESF  Created.
 */
void SRV_FreeSpecies(Int32 i)
{
  /* Reset the allocation state of the species */
  RISK_SetAllocationStateOfSpecies(i, ALLOC_NONE);
}


/**
 * Restarts the game.
 *
 * \b History:
 * \arg 04.04.95  ESF  Created.
 */
void SRV_ResetGame(void)
{
  /* Things we have to do when a game is restarted:
   *   1. Set the server state to be SERVER_REGISTERING
   *   2. Actually reset the dist. obj.
   *   3. Create a new deck of cards.
   */

  iServerMode = SERVER_REGISTERING;
  
  /* Reset the game if it has not been already reset */
  if (!fGameReset)
    {
      RISK_ResetGame();
      fGameReset = TRUE;
			  
      /* Get a new deck, destroy the old one. */
      DECK_Destroy(pCardDeck);
      pCardDeck = DECK_Create(NUM_COUNTRIES + 2);
    }
}


/**
 * Sanity checks and attempting to start a new game.
 *
 * \b History:
 * \arg 04.11.95  ESF  Created.
 * \arg 30.08.95  JC   if (iServerMode == SERVER_REGISTERING && ... ?
 */
void SRV_AttemptNewGame(void)
{
  Int32 n, iIndex;

  /* See if we can start a game */
  if (    (iServerMode != SERVER_REGISTERING)
       || (CLIENTS_GetNumClientsStarted() != CLIENTS_GetNumClients())
       || (RISK_GetNumPlayers() < 2))
    return;

  D_Assert(RISK_GetNumLivePlayers()>=2, "Bogus number of players!");

  printf("%s: %s\n",SERVERNAME,BEGINNING);
  /* We're going to need to reset it before playing again, so mark this. */
  fGameReset = FALSE;

  iServerMode = SERVER_FORTIFYING;

  SRV_SetInitialArmiesOfPlayers();
  SRV_SetInitialMissionOfPlayers();

  /* pacman sez:
   * Must choose a player to be "first" before choosing initial territories,
   * otherwise things won't go right. Example: with 5 players, the order of
   * things must be as follows:
   * 1. Every player gets 25 armies
   * 2. We go around choosing territories. When that's all done, the first
   *    two players have 9 each and the other three players have 8 each.
   * 3. Enter "fortify" mode - AND IT IS NOW THE 3RD PLAYER'S TURN. To pick a
   *    random player at this point is Wrong(TM).
   * 4. When the fortify mode is done, we'll be back at the first player
   *    again. He gets to attack first.
   * This order might get disrupted during the fortify stage, if the clients
   * are using bpk's multiple-place patch. We must remember who was first to
   * undo this disruption and enter 4 with iTurn being what it should be,
   * Which is what it would have been if the clients had fortified
   * one-at-a-time, which is iFirstPlayer. */
  iIndex = rand() % RISK_GetNumLivePlayers();
  iTurn = iFirstPlayer = RISK_GetNthLivePlayer(iIndex);

  /* Dole out the countries */
  SRV_DistributeCountries();

  SRV_NotifyClientsOfTurn(iTurn);

  /* Reset this for the next time */
  for (n=0; n!=MAX_CLIENTS; n++)
    if (CLIENTS_GetAllocationState(n) == ALLOC_COMPLETE)
      CLIENTS_SetStartState(n, FALSE);
}
