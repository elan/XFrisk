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
 *   $Id: network.h,v 1.4 1999/11/27 18:19:33 tony Exp $
 */

#ifndef _NETWORK
#define _NETWORK

#include <sys/types.h>

#include "types.h"

/****************/
/* The Messages */
/****************/

#define MSG_NOMESSAGE          0xDead

#define MSG_OLDREGISTERCLIENT  0x01
typedef struct _msgOldRegisterClient
{
  CString  strClientAddress;
} MsgOldRegisterClient;

#define MSG_REGISTERCLIENT     0x99
typedef struct _msgRegisterClient
{
  CString  strClientAddress;
  Int32    iClientType;  /* Flags */
} MsgRegisterClient;

/* Types of clients to connect */
#define CLIENT_NORMAL         0
#define CLIENT_AI             1
#define CLIENT_STRICTOBSERVER 2 /* TBD, for regression testing */

#define MSG_EXCHANGECARDS      0x03
typedef struct _msgExchangeCards
{
  Int32 piCards[3];
} MsgExchangeCards;

#define MSG_REQUESTCARD        0x04
typedef struct _msgRequestCard
{
  Int32 iPlayer;
} MsgRequestCard;

#define MSG_CARDPACKET         0x05
typedef struct _msgCardPacket
{
  Int32  iPlayer;
  Int32  cdCard;
} MsgCardPacket;

#define MSG_REPLYPACKET        0x06
typedef struct _msgReplyPacket
{
  Int32 iReply;
} MsgReplyPacket;

#define MSG_SENDMESSAGE        0x07
typedef struct _msgSendMessage
{
  CString strMessage;
  CString strDestination;
} MsgSendMessage;


#define MSG_MESSAGEPACKET      0x08
typedef struct _msgMessagePacket
{
  CString  strMessage;
  Int32    iFrom;
  Int32    iTo;
} MsgMessagePacket;

#define FROM_SERVER    -1
#define FROM_UNKNOW    -2
#define DST_ALLPLAYERS -1
#define DST_ALLBUTME   -2
#define DST_OTHER      -3

#define MSG_EXIT               0x09
#define MSG_STARTGAME          0x0A

#define MSG_TURNNOTIFY         0x0B
typedef struct _msgTurnNotify
{
  Int32 iPlayer;
  Int32 iClient;
} MsgTurnNotify;

#define MSG_ENDTURN            0x0C

#define MSG_CLIENTIDENT        0x0D
typedef struct _msgClientIdent
{
  Int32 iClientID;
} MsgClientIdent;

#define MSG_ENTERSTATE         0x0E
typedef struct _msgEnterState
{
  Int32 iState;
} MsgEnterState;

#define MSG_ENDOFGAME          0x10

#define MSG_OBJSTRUPDATE       0x11
typedef struct _msgObjStrUpdate
{
  Int32     iField;
  Int32     iIndex1, iIndex2;
  CString   strNewValue;
} MsgObjStrUpdate;

#define MSG_OBJINTUPDATE       0x12
typedef struct _msgObjIntUpdate
{
  Int32     iField;
  Int32     iIndex1, iIndex2;
  Int32     iNewValue;
} MsgObjIntUpdate;

#define MSG_DEREGISTERCLIENT   0x16
#define MSG_ALLOCPLAYER        0x19

#define MSG_FREEPLAYER         0x20
typedef struct _msgFreePlayer
{
  Int32 iPlayer;
} MsgFreePlayer;

#define MSG_NETMESSAGE         0x21
typedef struct _msgNetMessage
{
  CString strMessage;
} MsgNetMessage;

#define MSG_NETPOPUP           0x22
typedef struct _msgNetPopup
{
  CString strMessage;
} MsgNetPopup;

#define MSG_POPUPREGISTERBOX 0x23

#define MSG_DICEROLL           0x24
typedef struct _msgDiceRoll
{
  Int32 iDefendingPlayer;
  Int32 pAttackDice[3];
  Int32 pDefendDice[3];
} MsgDiceRoll;

#define MSG_PLACENOTIFY        0x25
typedef struct _msgPlaceNotify
{
  Int32 iCountry;
} MsgPlaceNotify;

#define MSG_ATTACKNOTIFY       0x26
typedef struct _msgAttackNotify
{
  Int32 iSrcCountry;
  Int32 iDstCountry;
} MsgAttackNotify;

#define MSG_MOVENOTIFY         0x27
typedef struct _msgMoveNotify
{
  Int32 iSrcCountry;
  Int32 iDstCountry;
} MsgMoveNotify;

#define MSG_POLLCLIENTS        0x28
typedef struct _msgPollClients
{
  CString strPollQuestion;
} MsgPollClients;

#define MSG_HELLO              0x2a

#define MSG_VERSION            0x2b
typedef struct _msgVersion
{
  CString strVersion;
} MsgVersion;

#define MSG_SPECIESIDENT       0x2c
typedef struct _msgSpeciesIdent
{
  Int32 iSpeciesID;
} MsgSpeciesIdent;

#define MSG_MISSION            0x2d

#define MSG_ENDOFMISSION       0x2e
typedef struct _msgEndOfMission
{
  Int32 iWinner;
  Int32 iTyp;
  Int32 iNum1;
  Int32 iNum2;
} MsgEndOfMission;

#define MSG_VICTORY            0x2f
typedef struct _msgVictory
{
  Int32 iWinner;
} MsgVictory;

#define MSG_FORCEEXCHANGECARDS 0x30
typedef struct _msgForceExchangeCards
{
  Int32 iPlayer;
} MsgForceExchangeCards;


Int32    NET_SendMessage(Int32 iSocket, Int32 iMessType, void *pvMessage);
Int32    NET_RecvMessage(Int32 iSocket, Int32 *piMessType, void **pvMessage);
void     NET_DeleteMessage(Int32 iMessType, void *pvMessage);
void     NET_SetCommLinkOptions(Int32 iSocket);
CString  NET_MessageToString(Int32 iMessType, void *pvMessage);
#endif


