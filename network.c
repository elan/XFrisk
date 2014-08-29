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
 *   $Id: network.c,v 1.6 1999/12/26 15:06:09 morphy Exp $
 *
 *   $Log: network.c,v $
 *   Revision 1.6  1999/12/26 15:06:09  morphy
 *   Doxygen comments
 *   Added static qualifier to local functions
 *   Miscellaneous editorial changes
 *
 */

/**
 * Messaging interface to/from the network.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "riskgame.h"
#include "types.h"
#include "network.h"
#include "debug.h"

/* Useful macro */
#define ReturnIfError(foo) if ((foo) <= 0) return (-1);
  
/* Local prototypes */
static Int32 _NET_SendString(Int32 iSocket, CString strCString);
static Int32 _NET_SendLong(Int32 iSocket, Int32 iLong);
static Int32 _NET_RecvString(Int32 iSocket, CString *pstrCString);
static Int32 _NET_SocketRead(Int32 iSocket, void *ptr, Int32 iNumBytes);
static Int32 _NET_SocketWrite(Int32 iSocket, void *ptr, Int32 iNumBytes);
static Int32 _NET_RecvLong(Int32 iSocket, Int32 *piLong);

/* Private to this routine */
static char strLogging[1024];


/**
 * Send a message to the given socket, field by field.
 *
 * \bug void pointer
 * \bug massive switch - case construct
 *
 * \b History:
 * \arg 01.24.94  ESF  Created 
 * \arg 01.28.94  ESF  Added strFrom in MSG_MESSAGEPACKET.
 * \arg 03.04.94  ESF  Changed MSG_UPDATE mesage.
 * \arg 03.05.94  ESF  Added MSG_ENTERSTATE for fault tolerance.
 * \arg 03.28.94  ESF  Added MSG_DEADPLAYER & MSG_ENDOFGAME.
 * \arg 03.28.94  ESF  Changed MSG_REQUESTCARDS to ...CARD.
 * \arg 03.29.94  ESF  Changed MSG_UPDATECARDS to MSG_EXCHANGECARDS.
 * \arg 03.29.94  ESF  Changed uiReply to be an Int32.
 * \arg 04.11.94  ESF  Added a player parameter to MSG_CARDPACKET.
 * \arg 04.11.94  ESF  Added a killer paramter to MSG_DEADPLAYER.
 * \arg 05.03.94  ESF  Added MSG_OBJ*UPDATE msgs.
 * \arg 05.03.94  ESF  Removed MSG_REGISTERPLAYER and MSG_UPDATEARMIES.
 * \arg 05.05.94  ESF  Added MSG_OBJ* msgs.
 * \arg 05.12.94  ESF  Removed MSG_OBJ* msgs and added GAME messages.
 * \arg 05.12.94  ESF  Added MSG_DEREGISTERCLIENT.
 * \arg 05.12.94  ESF  Added MSG_DELETEMSGDST.
 * \arg 05.13.94  ESF  Added MSG_STARTREGISTRATION.
 * \arg 05.15.94  ESF  Added MSG_[FREE|ALLOC]PLAYER
 * \arg 05.15.94  ESF  Removed MSG_DEADPLAYER.
 * \arg 05.17.94  ESF  Added MSG_NETMESSAGE.
 * \arg 07.27.94  ESF  Removed MSG_STARTREGISTRATION.
 * \arg 07.31.94  ESF  Added MSG_NETPOPUP.
 * \arg 08.03.94  ESF  Fixed to return error message. 
 * \arg 08.28.94  ESF  Added MSG_POPUPREGISTERBOX.
 * \arg 09.31.94  ESF  Changed MSG_ENDOFGAME to take a string parameter.
 * \arg 10.29.94  ESF  Added MSG_DICEROLL.
 * \arg 10.30.94  ESF  Added MSG_PLACENOTIFY.
 * \arg 10.30.94  ESF  Added MSG_ATTACKNOTIFY.
 * \arg 10.30.94  ESF  Added MSG_MOVENOTIFY.
 * \arg 01.17.95  ESF  Removed MSG_DELETEMSGDST.
 * \arg 02.21.95  ESF  Added MSG_HELLO, MSG_VERSION.
 * \arg 02.21.95  ESF  Modified MSG_REGISTERCLIENT to include type of client.
 * \arg 02.23.95  ESF  Added MSG_OLDREGISTERCLIENT for backwards compatibility.
 * \arg 02.23.95  ESF  Added MSG_SPECIESIDENT.
 * \arg 24.08.95  JC   Added MSG_MISSION.
 * \arg 28.08.95  JC   Added MSG_ENDOFMISSION and MSG_VICTORY.
 * \arg 30.08.95  JC   Added MSG_FORCEEXCHANGECARDS.
 */
Int32 NET_SendMessage(Int32 iSocket, Int32 iMessType, void *pvMessage)
{
  Int32 i;

  /* Send the message ID */
  ReturnIfError(_NET_SendLong(iSocket, (Int32)iMessType));

  switch(iMessType)
    {
    case MSG_OLDREGISTERCLIENT:  
      {
	MsgOldRegisterClient *pmsgMess = (MsgOldRegisterClient *)pvMessage;
	
	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strClientAddress));
      }
      break;

    case MSG_REGISTERCLIENT:  
      {
	MsgRegisterClient *pmsgMess = (MsgRegisterClient *)pvMessage;
	
	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strClientAddress));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iClientType));
      }
      break;

    case MSG_EXCHANGECARDS:
      {
	MsgExchangeCards *pmsgMess = (MsgExchangeCards *)pvMessage;
	
	for(i=0; i!=3; i++)
	  ReturnIfError(_NET_SendLong(iSocket, (Int32)pmsgMess->piCards[i]));
      }
      break;

    case MSG_CARDPACKET:
      {
	MsgCardPacket *pmsgMess = (MsgCardPacket *)pvMessage;

	ReturnIfError(_NET_SendLong(iSocket, (Int32)pmsgMess->iPlayer));
	ReturnIfError(_NET_SendLong(iSocket, (Int32)pmsgMess->cdCard));
      }
      break;
      
    case MSG_REPLYPACKET:
      {
	MsgReplyPacket *pmsgMess = (MsgReplyPacket *)pvMessage;

	ReturnIfError(_NET_SendLong(iSocket, (Int32)pmsgMess->iReply));
      }
      break;

    case MSG_SENDMESSAGE:
      {
	MsgSendMessage *pmsgMess = (MsgSendMessage *)pvMessage;

	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strMessage));
	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strDestination));
      } 
      break;

    case MSG_MESSAGEPACKET:
      {
	MsgMessagePacket *pmsgMess = (MsgMessagePacket *)pvMessage;

	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strMessage));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iFrom));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iTo));
      }
      break;

    case MSG_TURNNOTIFY:
      {
	MsgTurnNotify *pmsgMess = (MsgTurnNotify *)pvMessage;
	
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iPlayer));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iClient));
      }
      break;
      
    case MSG_CLIENTIDENT:
      {
	MsgClientIdent *pmsgMess = (MsgClientIdent *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iClientID));
      }
      break;

    case MSG_REQUESTCARD:
      {
	MsgRequestCard *pmsgMess = (MsgRequestCard *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iPlayer));
      }
      break;

    case MSG_ENTERSTATE:
      {
	MsgEnterState *pmsgMess = (MsgEnterState *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iState));
      }
      break;

    case MSG_OBJSTRUPDATE:
      {
	MsgObjStrUpdate *pmsgMess = (MsgObjStrUpdate *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iField));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iIndex1));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iIndex2));
	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strNewValue));
      }
      break;

    case MSG_OBJINTUPDATE:
      {
	MsgObjIntUpdate *pmsgMess = (MsgObjIntUpdate *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iField));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iIndex1));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iIndex2));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iNewValue));
      }
      break;

    case MSG_FREEPLAYER:
      {
	MsgFreePlayer *pmsgMess = (MsgFreePlayer *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iPlayer));
      }
      break;

    /* These happen to be identical, so we can lump them. */  
    case MSG_NETMESSAGE:
    case MSG_NETPOPUP:
      {
	MsgNetMessage *pmsgMess = (MsgNetMessage *)pvMessage;
	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strMessage));
      }
      break;

    case MSG_ENDOFMISSION:
      {
	MsgEndOfMission *pmsgMess = (MsgEndOfMission *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iWinner));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iTyp));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iNum1));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iNum2));
      }
      break;

    case MSG_VICTORY:
      {
	MsgVictory *pmsgMess = (MsgVictory *)pvMessage;
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iWinner));
      }
      break;

    case MSG_DICEROLL:
      {
	MsgDiceRoll *pmsgMess = (MsgDiceRoll *)pvMessage;

	for (i=0; i!=3; i++)
	  ReturnIfError(_NET_SendLong(iSocket, pmsgMess->pAttackDice[i]));

	for (i=0; i!=3; i++)
	  ReturnIfError(_NET_SendLong(iSocket, pmsgMess->pDefendDice[i]));

	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iDefendingPlayer));
      }
      break;

    case MSG_PLACENOTIFY:
      {
	MsgPlaceNotify *pmsgMess = (MsgPlaceNotify *)pvMessage;

	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iCountry));
      }
      break;

    case MSG_ATTACKNOTIFY:
      {
	MsgAttackNotify *pmsgMess = (MsgAttackNotify *)pvMessage;

	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iSrcCountry));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iDstCountry));
      }
      break;

    case MSG_MOVENOTIFY:
      {
	MsgMoveNotify *pmsgMess = (MsgMoveNotify *)pvMessage;
	
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iSrcCountry));
	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iDstCountry));
      }
      break;

    case MSG_VERSION:
      {
	MsgVersion *pmsgMess = (MsgVersion *)pvMessage;
	
	ReturnIfError(_NET_SendString(iSocket, pmsgMess->strVersion));
      }
      break;

    case MSG_SPECIESIDENT:
      {
	MsgSpeciesIdent *pmsgMess = (MsgSpeciesIdent *)pvMessage;

	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iSpeciesID));
      }
      break;

    case MSG_FORCEEXCHANGECARDS:
      {
	MsgForceExchangeCards *pmsgMess = (MsgForceExchangeCards *)pvMessage;

	ReturnIfError(_NET_SendLong(iSocket, pmsgMess->iPlayer));
      }

    case MSG_EXIT:
    case MSG_STARTGAME:
    case MSG_ENDTURN:
    case MSG_DEREGISTERCLIENT:
    case MSG_ALLOCPLAYER:
    case MSG_POPUPREGISTERBOX:
    case MSG_HELLO:
    case MSG_MISSION:
    case MSG_ENDOFGAME:
      /* No parameters */
      break;

    default:
      printf("NETWORK: Illegal message!\n");
    }

  return (1);
}


/**
 * Receive one message from the network, field by field.
 *
 * \bug pointer to void pointer
 * \bug massive switch - case construct
 *
 * \b History:
 * \arg 01.24.94  ESF  Created.
 * \arg 01.27.94  ESF  Fixed bug in MSG_MESSAGEPACKET case.
 * \arg 01.28.94  ESF  Added strFrom in MSG_MESSAGEPACKET.
 * \arg 03.04.94  ESF  Changed _UPDATE mesage.
 * \arg 03.05.94  ESF  Added _ENTERSTATE for fault tolerance.
 * \arg 03.28.94  ESF  Added MSG_DEADPLAYER & MSG_ENDOFGAME.
 * \arg 03.28.94  ESF  Changed MSG_REQUESTCARDS to ...CARD.
 * \arg 03.29.94  ESF  Changed MSG_UPDATECARDS to MSG_EXCHANGECARDS.
 * \arg 03.29.94  ESF  Changed uiReply to be an Int32.
 * \arg 04.11.94  ESF  Added a player parameter to MSG_CARDPACKET.
 * \arg 04.11.94  ESF  Added a killer paramter to MSG_DEADPLAYER.
 * \arg 05.03.94  ESF  Added MSG_OBJ*UPDATE msgs.
 * \arg 05.03.94  ESF  Removed MSG_REGISTERPLAYER and MSG_UPDATEARMIES.
 * \arg 05.05.94  ESF  Added MSG_OBJ* msgs.
 * \arg 05.12.94  ESF  Removed MSG_OBJ* msgs and added GAME messages.
 * \arg 05.12.94  ESF  Added MSG_DEREGISTERCLIENT.
 * \arg 05.12.94  ESF  Added MSG_DELETEMSGDST.
 * \arg 05.13.94  ESF  Added MSG_STARTREGISTRATION.
 * \arg 05.15.94  ESF  Added MSG_[FREE|ALLOC]PLAYER
 * \arg 05.15.94  ESF  Removed MSG_DEADPLAYER.
 * \arg 05.17.94  ESF  Added MSG_NETMESSAGE.
 * \arg 07.27.94  ESF  Removed MSG_STARTREGISTRATION.
 * \arg 07.31.94  ESF  Added MSG_NETPOPUP.
 * \arg 08.03.94  ESF  Fixed to return error message. 
 * \arg 08.28.94  ESF  Added MSG_POPUPREGISTERBOX.
 * \arg 09.31.94  ESF  Changed MSG_ENDOFGAME to take a string parameter.
 * \arg 10.29.94  ESF  Added MSG_DICEROLL.
 * \arg 10.30.94  ESF  Added MSG_PLACENOTIFY.
 * \arg 10.30.94  ESF  Added MSG_ATTACKNOTIFY.
 * \arg 10.30.94  ESF  Added MSG_MOVENOTIFY.
 * \arg 01.17.95  ESF  Removed MSG_DELETEMSGDST.
 * \arg 02.21.95  ESF  Added MSG_HELLO, MSG_VERSION.
 * \arg 02.21.95  ESF  Modified MSG_REGISTERCLIENT to include type of client.
 * \arg 02.23.95  ESF  Added MSG_OLDREGISTERCLIENT.
 * \arg 02.23.95  ESF  Added MSG_SPECIESIDENT.
 * \arg 24.08.95  JC   Added MSG_MISSION.
 * \arg 28.08.95  JC   Added MSG_ENDOFMISSION and MSG_VICTORY.
 * \arg 30.08.95  JC   Added MSG_FORCEEXCHANGECARDS.
 */
Int32 NET_RecvMessage(Int32 iSocket, Int32 *piMessType, void **ppvMessage)
{
  Int32 i;

  /* Get the message ID */
  ReturnIfError(_NET_RecvLong(iSocket, (Int32 *)piMessType));

  switch(*piMessType)
    {
    case MSG_OLDREGISTERCLIENT:  
      {
	MsgOldRegisterClient *pmsgMess = 
	  (MsgOldRegisterClient *)MEM_Alloc(sizeof(MsgOldRegisterClient));
	
	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strClientAddress));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_REGISTERCLIENT:  
      {
	MsgRegisterClient *pmsgMess = 
	  (MsgRegisterClient *)MEM_Alloc(sizeof(MsgRegisterClient));
	
	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strClientAddress));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iClientType));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_EXCHANGECARDS:
      {
	MsgExchangeCards *pmsgMess = 
	  (MsgExchangeCards *)MEM_Alloc(sizeof(MsgExchangeCards));
	
	for(i=0; i!=3; i++)
	  ReturnIfError(_NET_RecvLong(iSocket, 
				      (Int32 *)&pmsgMess->piCards[i]));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_CARDPACKET:
      {
	MsgCardPacket *pmsgMess = 
	  (MsgCardPacket *)MEM_Alloc(sizeof(MsgCardPacket));

	ReturnIfError(_NET_RecvLong(iSocket, (Int32 *)&pmsgMess->iPlayer));
	ReturnIfError(_NET_RecvLong(iSocket, (Int32 *)&pmsgMess->cdCard));

	*ppvMessage = (void *)pmsgMess;
      }
      break;
      
    case MSG_REPLYPACKET:
      {
	MsgReplyPacket *pmsgMess = 
	  (MsgReplyPacket *)MEM_Alloc(sizeof(MsgReplyPacket));

	ReturnIfError(_NET_RecvLong(iSocket, (Int32 *)&pmsgMess->iReply));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_SENDMESSAGE:
      {
	MsgSendMessage *pmsgMess = 
	  (MsgSendMessage *)MEM_Alloc(sizeof(MsgSendMessage));

	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strMessage));
	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strDestination));

	*ppvMessage = (void *)pmsgMess;
      } 
      break;

    case MSG_MESSAGEPACKET:
      {
	MsgMessagePacket *pmsgMess = 
	  (MsgMessagePacket *)MEM_Alloc(sizeof(MsgMessagePacket));

	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strMessage));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iFrom));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iTo));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_TURNNOTIFY:
      {
	MsgTurnNotify *pmsgMess = 
	  (MsgTurnNotify *)MEM_Alloc(sizeof(MsgTurnNotify));
	
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iPlayer));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iClient));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_CLIENTIDENT:
      {
	MsgClientIdent *pmsgMess = 
	  (MsgClientIdent *)MEM_Alloc(sizeof(MsgClientIdent));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iClientID));
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_REQUESTCARD:
      {
	MsgRequestCard *pmsgMess = 
	  (MsgRequestCard *)MEM_Alloc(sizeof(MsgRequestCard));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iPlayer));
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_ENTERSTATE:
      {
	MsgEnterState *pmsgMess = 
	  (MsgEnterState *)MEM_Alloc(sizeof(MsgEnterState));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iState));
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_OBJSTRUPDATE:
      {
	MsgObjStrUpdate *pmsgMess = 
	  (MsgObjStrUpdate *)MEM_Alloc(sizeof(MsgObjStrUpdate));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iField));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iIndex1));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iIndex2));
	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strNewValue));
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_OBJINTUPDATE:
      {
	MsgObjIntUpdate *pmsgMess = 
	  (MsgObjIntUpdate *)MEM_Alloc(sizeof(MsgObjIntUpdate));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iField));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iIndex1));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iIndex2));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iNewValue));
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_FREEPLAYER:
      {
	MsgFreePlayer *pmsgMess = 
	  (MsgFreePlayer *)MEM_Alloc(sizeof(MsgFreePlayer));
	
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iPlayer));
	*ppvMessage = (void *)pmsgMess;
      }
      break;      

    /* These happen to be identical, so we can lump them. */  
    case MSG_NETMESSAGE:
    case MSG_NETPOPUP:
      {
	MsgNetMessage *pmsgMess = 
	  (MsgNetMessage *)MEM_Alloc(sizeof(MsgNetMessage));
	
	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strMessage));
	*ppvMessage = (void *)pmsgMess;
      }
      break;      

    case MSG_ENDOFMISSION:
      {
	MsgEndOfMission *pmsgMess = 
	  (MsgEndOfMission *)MEM_Alloc(sizeof(MsgEndOfMission));
	
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iWinner));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iTyp));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iNum1));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iNum2));
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_VICTORY:
      {
	MsgVictory *pmsgMess = 
	  (MsgVictory *)MEM_Alloc(sizeof(MsgVictory));
	
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iWinner));
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_DICEROLL:
      {
	MsgDiceRoll *pmsgMess = 
	  (MsgDiceRoll *)MEM_Alloc(sizeof(MsgDiceRoll));

    	for (i=0; i!=3; i++)
	  ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->pAttackDice[i]));

    	for (i=0; i!=3; i++)
	  ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->pDefendDice[i]));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iDefendingPlayer));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_PLACENOTIFY:
      {
	MsgPlaceNotify *pmsgMess = 
	  (MsgPlaceNotify *)MEM_Alloc(sizeof(MsgPlaceNotify));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iCountry));
	
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_ATTACKNOTIFY:
      {
	MsgAttackNotify *pmsgMess = 
	  (MsgAttackNotify *)MEM_Alloc(sizeof(MsgAttackNotify));

	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iSrcCountry));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iDstCountry));
	
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_MOVENOTIFY:
      {
	MsgMoveNotify *pmsgMess = 
	  (MsgMoveNotify *)MEM_Alloc(sizeof(MsgMoveNotify));
		
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iSrcCountry));
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iDstCountry));

	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_VERSION:
      {
	MsgVersion *pmsgMess = 
	  (MsgVersion *)MEM_Alloc(sizeof(MsgVersion));

	ReturnIfError(_NET_RecvString(iSocket, &pmsgMess->strVersion));
	
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_SPECIESIDENT:
      {
	MsgSpeciesIdent *pmsgMess = 
	  (MsgSpeciesIdent *)MEM_Alloc(sizeof(MsgSpeciesIdent));
	
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iSpeciesID));
	
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_FORCEEXCHANGECARDS:
      {
	MsgForceExchangeCards *pmsgMess = 
	  (MsgForceExchangeCards *)MEM_Alloc(sizeof(MsgForceExchangeCards));
	
	ReturnIfError(_NET_RecvLong(iSocket, &pmsgMess->iPlayer));
	
	*ppvMessage = (void *)pmsgMess;
      }
      break;

    case MSG_EXIT:
    case MSG_STARTGAME:
    case MSG_ENDTURN:
    case MSG_DEREGISTERCLIENT:
    case MSG_ALLOCPLAYER:
    case MSG_POPUPREGISTERBOX:
    case MSG_HELLO:
    case MSG_MISSION:
    case MSG_ENDOFGAME:
      *ppvMessage = NULL;
      break;

    default:
      printf("NETWORK: Illegal message!\n");
    }

  return (1);
}

/************************************************************************ 
 *  FUNCTION: _NET_SendString
 *  HISTORY: 
 *     01.24.94  ESF  Created. 
 *     08.03.94  ESF  Fixed to return error message. 
 *     03.22.95  ESF  Added handling for NULL strings.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
static Int32 _NET_SendString(Int32 iSocket, CString strCString)
{
  Int32 iLength;

  /* Send the length and then the string itself */
  if (strCString == NULL)
    iLength = 0;
  else
    iLength = strlen(strCString)+1;

  ReturnIfError(_NET_SendLong(iSocket, (Int32)iLength));
  return (_NET_SocketWrite(iSocket, (Char *)strCString, (Int32)iLength));
}


/**
 * Send a long integer to the given socket.
 *
 * \b History:
 *     01.24.94  ESF  Created 
 *     08.03.94  ESF  Fixed to return error message. 
 */
static Int32 _NET_SendLong(Int32 iSocket, Int32 iLong)
{
  Int32 iData = htonl(iLong);
  
  return (_NET_SocketWrite(iSocket, &iData, sizeof(Int32)));
}


/**
 * Receive a string from the given socket. First reads the length
 * (32bit value), then the string data.
 *
 * \bug Assumes that a nul terminating byte is in the data stream
 *
 * \b History:
 * \arg 01.24.94  ESF  Created 
 * \arg 08.03.94  ESF  Fixed to return error message. 
 * \arg 01.01.94  ESF  Added check for correct number of bytes read.
 * \arg 03.22.95  ESF  Added handling for NULL strings.
 */
static Int32 _NET_RecvString(Int32 iSocket, CString *pstrCString)
{
  Int32    iLength;
  CString  strTemp;
  Int32    iRet;

  /* Receive the length and then the byte stream */
  ReturnIfError(_NET_RecvLong(iSocket, &iLength));

  if (iLength)
    {
      strTemp = (CString)MEM_Alloc(iLength);
      iRet = _NET_SocketRead(iSocket, strTemp, iLength);

      /* Return an error if less than the string was read! */
      if (iRet<0 || iRet!=iLength)
	iRet = -1;
    }
  else
    { 
      iRet = -1;
      strTemp = NULL;
    }
  
  *pstrCString = strTemp;
  return iRet;
}


/**
 * Reads a 32bit value from the given socket.
 *
 * \b History:
 * \arg 01.24.94  ESF  Created 
 * \arg 08.03.94  ESF  Fixed to return error message. 
 * \arg 01.01.94  ESF  Added check for correct number of bytes read.
 */
static Int32 _NET_RecvLong(Int32 iSocket, Int32 *piLong)
{
  Int32 iRet = _NET_SocketRead(iSocket, piLong, sizeof(Int32));
  
  /* Adjust for the network munging */
  *piLong = ntohl(*piLong);
  
  /* Return an error if read returns an error or if the wrong number
   * of bytes come across -- there should be sizeof(Int32) bytes!
   */
  
  if (iRet<0 || iRet!=sizeof(Int32))
    iRet = -1;

  return iRet;
}


/**
 * Reads the given number of bytes from the given socket.
 *
 * \b History:
 * \arg 01.01.95  ESF  Created.
 * \arg 01.21.95  ESF  Fixed non-ANSIness.
 */
static Int32 _NET_SocketRead(Int32 iSocket, void *ptr, Int32 iNumBytes)
{
  Int32 iBytesLeft, iRet;

  /* We may have to do multiple read() calls here */
  iBytesLeft = iNumBytes;
  
  while (iBytesLeft > 0)
    {
      iRet = read(iSocket, ptr, iBytesLeft);

      /* If an error occured, return */
      if (iRet < 0)
	return iRet;
      else if (iRet == 0)
	break; /* EOF */
      
      iBytesLeft  -= iRet;
      ptr = (Byte *)ptr + iRet;
    }
  
  /* Return the number of bytes read */
  return (iNumBytes - iBytesLeft);
}


/**
 * Write the given number of bytes to the given socket.
 *
 * \b History:
 * \arg 01.24.94  ESF  Created 
 * \arg 08.03.94  ESF  Fixed to return error message. 
 * \arg 01.01.94  ESF  Added check for correct number of bytes read.
 * \arg 01.21.95  ESF  Fixed non-ANSIness.
 */
static Int32 _NET_SocketWrite(Int32 iSocket, void *ptr, Int32 iNumBytes)
{
  Int32 iBytesLeft, iRet;

  /* We may have to do multiple read() calls here */
  iBytesLeft = iNumBytes;
  
  while (iBytesLeft > 0)
    {
      iRet = write(iSocket, ptr, iBytesLeft);

      /* If an error occured, return */
      if (iRet < 0)
	return iRet;

      iBytesLeft  -= iRet;
      ptr = (Byte *)ptr + iRet;
    }
  
  /* Return the number of bytes read */
  return (iNumBytes - iBytesLeft);
}


/**
 * Free the memory allocated for the given message.
 *
 * \b History:
 * \arg 06.24.94  ESF  Created 
 * \arg 07.31.94  ESF  Added MSG_NETPOPUP.
 * \arg 09.31.94  ESF  Changed MSG_ENDOFGAME to take a string parameter.
 * \arg 10.29.94  ESF  Added MSG_DICEROLL.
 * \arg 10.30.94  ESF  Added MSG_PLACENOTIFY.
 * \arg 10.30.94  ESF  Added MSG_ATTACKNOTIFY.
 * \arg 10.30.94  ESF  Added MSG_MOVENOTIFY.
 * \arg 01.17.95  ESF  Removed MSG_DELETEMSGDST.
 * \arg 02.21.95  ESF  Added MSG_HELLO, MSG_VERSION.
 * \arg 02.21.95  ESF  Modified MSG_REGISTERCLIENT to include type of client.
 * \arg 02.23.95  ESF  Added MSG_OLDREGISTERCLIENT.
 * \arg 02.23.95  ESF  Added MSG_SPECIESIDENT.
 * \arg 24.08.95  JC   Added MSG_MISSION.
 * \arg 30.08.95  JC   Added MSG_FORCEEXCHANGECARDS.
 */
void NET_DeleteMessage(Int32 iMessType, void *pvMessage)
{
  switch(iMessType)
    {
    case MSG_NOMESSAGE:
      break;

    case MSG_OLDREGISTERCLIENT:  
      {
	MsgOldRegisterClient *pmsgMess = (MsgOldRegisterClient *)pvMessage;
	MEM_Free(pmsgMess->strClientAddress);
	MEM_Free(pmsgMess);
      }
      break;
      
    case MSG_REGISTERCLIENT:  
      {
	MsgRegisterClient *pmsgMess = (MsgRegisterClient *)pvMessage;
	MEM_Free(pmsgMess->strClientAddress);
	MEM_Free(pmsgMess);
      }
      break;

    case MSG_EXCHANGECARDS:
      MEM_Free(pvMessage);
      break;

    case MSG_CARDPACKET:
      MEM_Free(pvMessage);
      break;
      
    case MSG_REPLYPACKET:
      MEM_Free(pvMessage);
      break;

    case MSG_SENDMESSAGE:
      {
	MsgSendMessage *pmsgMess = (MsgSendMessage *)pvMessage;

	MEM_Free(pmsgMess->strMessage);
	MEM_Free(pmsgMess->strDestination);
	MEM_Free(pvMessage);
      } 
      break;

    case MSG_MESSAGEPACKET:
      {
	MsgMessagePacket *pmsgMess = (MsgMessagePacket *)pvMessage;

	MEM_Free(pmsgMess->strMessage);
	MEM_Free(pvMessage);
      }
      break;

    case MSG_TURNNOTIFY:
      MEM_Free(pvMessage);
      break;
      
    case MSG_CLIENTIDENT:
      MEM_Free(pvMessage);
      break;

    case MSG_REQUESTCARD:
      MEM_Free(pvMessage);
      break;

    case MSG_ENTERSTATE:
      MEM_Free(pvMessage);
      break;

    case MSG_OBJSTRUPDATE:
      {
	MsgObjStrUpdate *pmsgMess = (MsgObjStrUpdate*)pvMessage;

	if (pmsgMess->strNewValue)
	  MEM_Free(pmsgMess->strNewValue);
	MEM_Free(pvMessage);
      }
      break;
      
    case MSG_OBJINTUPDATE:
      MEM_Free(pvMessage);
      break;

    case MSG_FREEPLAYER:
      MEM_Free(pvMessage);
      break;

    /* These happen to be identical, so we can lump them. */  
    case MSG_NETMESSAGE:
    case MSG_NETPOPUP:
      {
	MsgNetMessage *pmsgMess = (MsgNetMessage *)pvMessage;
	MEM_Free(pmsgMess->strMessage);
	MEM_Free(pvMessage);
      }
      break;

    case MSG_ENDOFMISSION:
      MEM_Free(pvMessage);
      break;

    case MSG_VICTORY:
      MEM_Free(pvMessage);
      break;

    case MSG_DICEROLL:
      MEM_Free(pvMessage);
      break;

    case MSG_PLACENOTIFY:
      MEM_Free(pvMessage);
      break;

    case MSG_ATTACKNOTIFY:
      MEM_Free(pvMessage);
      break;

    case MSG_MOVENOTIFY:
      MEM_Free(pvMessage);
      break;
      
    case MSG_VERSION:
      {
	MsgVersion *pmsgMess = (MsgVersion *)pvMessage;
	MEM_Free(pmsgMess->strVersion);
	MEM_Free(pvMessage);
      }
      break;

    case MSG_SPECIESIDENT:
      MEM_Free(pvMessage);
      break;

    case MSG_FORCEEXCHANGECARDS:
      MEM_Free(pvMessage);
      break;

    case MSG_EXIT:
    case MSG_STARTGAME:
    case MSG_ENDTURN:
    case MSG_DEREGISTERCLIENT:
    case MSG_ALLOCPLAYER:
    case MSG_POPUPREGISTERBOX:
    case MSG_HELLO:
    case MSG_MISSION:
    case MSG_ENDOFGAME:
      /* No parameters */
      D_Assert(pvMessage == NULL, "Hum...this shouldn't happen.");
      break;

    default:
      D_Assert(FALSE, "Add case for a new message in NET_DeleteMessage!");
    }
}


/**
 * Set socket options for better performance.
 *
 * \b History:
 * \arg 01.22.95  ESF  Created.
 * \arg 02.27.95  ESF  Added SO_REUSEADDR.
 * \arg 17.08.95  JC   iOption -> &iOption.
 */
void NET_SetCommLinkOptions(Int32 iSocket)
{
#ifdef TCP_NODELAY
  {
    Int32 iOption = 1;
    if (setsockopt(iSocket, IPPROTO_TCP, TCP_NODELAY, 
		   (char *)&iOption, sizeof(iOption)) != 0)
      printf("NETWORK: Warning -- couldn't set TCP_NODELAY on CommLink!\n");
  }
#endif
#ifdef SO_REUSEADDR
  {
    Int32 iOption = 1;
    if (setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, 
		   (char *)&iOption, sizeof(iOption)) != 0)
      printf("NETWORK: Warning -- couldn't set SO_REUSEADDR on CommLink!\n");
  }
#endif
}


/**
 * Convert the given message to a human readable string for debugging.
 *
 * \b History:
 * \arg 03.19.95  ESF  Created.
 * \arg 24.08.95  JC   Added MSG_MISSION.
 * \arg 30.08.95  JC   Added MSG_FORCEEXCHANGECARDS.
 */
CString NET_MessageToString(Int32 iMessType, void *pvMessage)
{
  switch (iMessType)
    {
    case MSG_NOMESSAGE:
      snprintf(strLogging, sizeof(strLogging), "MsgNoMessage()");
      break;

    case MSG_OLDREGISTERCLIENT:
      snprintf(strLogging, sizeof(strLogging), "MsgOldRegisterClient(strClientAddress=\"%s\")", 
	      ((MsgOldRegisterClient *)pvMessage)->strClientAddress);  
      break;
      
    case MSG_REGISTERCLIENT:  
      snprintf(strLogging, sizeof(strLogging), "MsgRegisterClient(strClientAddress=\"%s\","
	      " iClientType=%s)",
	      ((MsgRegisterClient *)pvMessage)->strClientAddress, 
	      ((MsgRegisterClient *)pvMessage)->iClientType == 
	      CLIENT_NORMAL ? "CLIENT_NORMAL" :
	      ((MsgRegisterClient *)pvMessage)->iClientType == 
	      CLIENT_AI ? "CLIENT_AI" :
	      ((MsgRegisterClient *)pvMessage)->iClientType == 
	      CLIENT_STRICTOBSERVER ? "CLIENT_STRICTOBSERVER" :
	      "*Unknown*");
      break;
      
    case MSG_EXCHANGECARDS:
      snprintf(strLogging, sizeof(strLogging), "MsgExchangeCards(piCards={%d, %d, %d})",
	      ((MsgExchangeCards *)pvMessage)->piCards[0],
	      ((MsgExchangeCards *)pvMessage)->piCards[1],
	      ((MsgExchangeCards *)pvMessage)->piCards[2]);
      break;
      
    case MSG_CARDPACKET:
      snprintf(strLogging, sizeof(strLogging), "MsgCardPacket(iPlayer=%d, iCard=%d)",
	      ((MsgCardPacket *)pvMessage)->iPlayer, 
	      ((MsgCardPacket *)pvMessage)->cdCard); 
      break;
      
    case MSG_REPLYPACKET:
      snprintf(strLogging, sizeof(strLogging), "MsgReplyPacket(iReply=%d)",
	      ((MsgReplyPacket *)pvMessage)->iReply);
      break;

    case MSG_SENDMESSAGE:
      snprintf(strLogging, sizeof(strLogging), "MsgSendMessage(strMessage=\"%s\", "
	      "strDestination=\"%s\")",
	      ((MsgSendMessage *)pvMessage)->strMessage,
	      ((MsgSendMessage *)pvMessage)->strDestination);
      break;
      
    case MSG_MESSAGEPACKET:
      snprintf(strLogging, sizeof(strLogging), "MsgMessagePacket(strMessage=\"%s\", "
	      "iFrom=\"%d\", iTo=%d)",
	      ((MsgMessagePacket *)pvMessage)->strMessage,
	      ((MsgMessagePacket *)pvMessage)->iFrom,
	      ((MsgMessagePacket *)pvMessage)->iTo);
      break;

    case MSG_TURNNOTIFY:
      snprintf(strLogging, sizeof(strLogging), "MsgTurnNotify(iPlayer=%d, iClient=%d)",
	      ((MsgTurnNotify *)pvMessage)->iPlayer,
	      ((MsgTurnNotify *)pvMessage)->iClient);
      break;
      
    case MSG_CLIENTIDENT:
      snprintf(strLogging, sizeof(strLogging), "MsgClientIdent(iClientID=%d)",
	      ((MsgClientIdent *)pvMessage)->iClientID);
      break;

    case MSG_REQUESTCARD:
      snprintf(strLogging, sizeof(strLogging), "MsgRequestCard(iPlayer=%d)",
	      ((MsgRequestCard *)pvMessage)->iPlayer);
      break;

    case MSG_ENTERSTATE:
      snprintf(strLogging, sizeof(strLogging), "MsgEnterState(iState=%d)",
	      ((MsgEnterState *)pvMessage)->iState);
      break;

    case MSG_OBJSTRUPDATE:
      snprintf(strLogging, sizeof(strLogging), "ObjStrUpdate(iField=%d, iIndex=[%d, %d], "
	      "strNewValue=\"%s\")",
	      ((MsgObjStrUpdate *)pvMessage)->iField,
	      ((MsgObjStrUpdate *)pvMessage)->iIndex1,
	      ((MsgObjStrUpdate *)pvMessage)->iIndex2,
	      ((MsgObjStrUpdate *)pvMessage)->strNewValue);
      break;
      
    case MSG_OBJINTUPDATE:
      snprintf(strLogging, sizeof(strLogging), "ObjIntUpdate(iField=%d, iIndex=[%d, %d], "
	      "iNewValue=%d)",
	      ((MsgObjIntUpdate *)pvMessage)->iField,
	      ((MsgObjIntUpdate *)pvMessage)->iIndex1,
	      ((MsgObjIntUpdate *)pvMessage)->iIndex2,
	      ((MsgObjIntUpdate *)pvMessage)->iNewValue);
      break;

    case MSG_FREEPLAYER:
      snprintf(strLogging, sizeof(strLogging), "MsgFreePlayer(iPlayer=%d)",
	      ((MsgFreePlayer *)pvMessage)->iPlayer);
      break;

    case MSG_NETMESSAGE:
      snprintf(strLogging, sizeof(strLogging), "MsgNetMessage(strMessage=\"%s\")",
	      ((MsgNetMessage *)pvMessage)->strMessage);
      break;

    case MSG_NETPOPUP:
      snprintf(strLogging, sizeof(strLogging), "MsgNetPopup(strMessage=\"%s\")",
	      ((MsgNetPopup *)pvMessage)->strMessage);
      break;

    case MSG_ENDOFMISSION:
      snprintf(strLogging, sizeof(strLogging), "MsgEndOfMission(iWinner=%d, iTyp=%d, iNum1=%d, iNum2=%d)",
	      ((MsgEndOfMission *)pvMessage)->iWinner,
	      ((MsgEndOfMission *)pvMessage)->iTyp,
	      ((MsgEndOfMission *)pvMessage)->iNum1,
	      ((MsgEndOfMission *)pvMessage)->iNum2);
      break;

    case MSG_VICTORY:
      snprintf(strLogging, sizeof(strLogging), "MsgVictory(iWinner=%d)",
	      ((MsgVictory *)pvMessage)->iWinner);
      break;

    case MSG_ENDOFGAME:
      snprintf(strLogging, sizeof(strLogging), "MsgEndOfGame()");
      break;

    case MSG_DICEROLL:
      snprintf(strLogging, sizeof(strLogging), "MsgDiceRoll(iDefendingPlayer=%d, "
	      "pAttackDice={%d, %d, %d}, pDefendDice={%d, %d, %d})",
	      ((MsgDiceRoll *)pvMessage)->iDefendingPlayer,
	      ((MsgDiceRoll *)pvMessage)->pAttackDice[0],
	      ((MsgDiceRoll *)pvMessage)->pAttackDice[1],
	      ((MsgDiceRoll *)pvMessage)->pAttackDice[2],
	      ((MsgDiceRoll *)pvMessage)->pDefendDice[0],
	      ((MsgDiceRoll *)pvMessage)->pDefendDice[1],
	      ((MsgDiceRoll *)pvMessage)->pDefendDice[2]);
      break;

    case MSG_PLACENOTIFY:
      snprintf(strLogging, sizeof(strLogging), "MsgPlaceNotify(iCountry=%d)",
	      ((MsgPlaceNotify *)pvMessage)->iCountry);
      break;

    case MSG_ATTACKNOTIFY:
      snprintf(strLogging, sizeof(strLogging), "MsgAttackNotify(iSrcCountry=%d, iDstCountry=%d)",
	      ((MsgAttackNotify *)pvMessage)->iSrcCountry,
	      ((MsgAttackNotify *)pvMessage)->iDstCountry);
      break;

    case MSG_MOVENOTIFY:
      snprintf(strLogging, sizeof(strLogging), "MsgMoveNotify(iSrcCountry=%d, iDstCountry=%d)",
	      ((MsgMoveNotify *)pvMessage)->iSrcCountry,
	      ((MsgMoveNotify *)pvMessage)->iDstCountry);
      break;
      
    case MSG_VERSION:
      snprintf(strLogging, sizeof(strLogging), "MsgVersion(strVersion=\"%s\")",
	      ((MsgVersion *)pvMessage)->strVersion);
      break;

    case MSG_SPECIESIDENT:
      snprintf(strLogging, sizeof(strLogging), "MsgSpeciesIdent(iSpeciesID=%d)",
	      ((MsgSpeciesIdent *)pvMessage)->iSpeciesID);
      break;

    case MSG_FORCEEXCHANGECARDS:
      snprintf(strLogging, sizeof(strLogging), "MsgForceExchangeCards(iPlayer=%d)",
	      ((MsgForceExchangeCards *)pvMessage)->iPlayer);
      break;

    case MSG_EXIT:
      snprintf(strLogging, sizeof(strLogging), "MsgExit()");
      break;

    case MSG_STARTGAME:
      snprintf(strLogging, sizeof(strLogging), "MsgStartGame()");
      break;

    case MSG_ENDTURN:
      snprintf(strLogging, sizeof(strLogging), "MsgEndTurn()");
      break;

    case MSG_DEREGISTERCLIENT:
      snprintf(strLogging, sizeof(strLogging), "MsgDeregisterClient()");
      break;

    case MSG_ALLOCPLAYER:
      snprintf(strLogging, sizeof(strLogging), "MsgAllocPlayer()");
      break;

    case MSG_POPUPREGISTERBOX:
      snprintf(strLogging, sizeof(strLogging), "MsgPopupRegisterBox()");
      break;

    case MSG_HELLO:
      snprintf(strLogging, sizeof(strLogging), "MsgHello()");
      break;

    case MSG_MISSION:
      snprintf(strLogging, sizeof(strLogging), "MsgMission()");
      break;

    default:
      D_Assert(FALSE, "Add case to NET_StringToMessage!");
    }
  
  return strLogging;
}

/* EOF */
