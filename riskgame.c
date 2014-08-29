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
 *   $Id: riskgame.c,v 1.16 2000/01/23 20:10:09 tony Exp $
 */

/** \file
 *   "World" used by server as well as client
 */
/*
 *
 *   $Log: riskgame.c,v $
 *   Revision 1.16  2000/01/23 20:10:09  tony
 *   oops, needed another commit for doxygen :-)
 *
 *   Revision 1.14  2000/01/09 16:06:23  tony
 *   oops, wrong doxygen tags
 *
 *   Revision 1.13  2000/01/04 21:41:53  tony
 *   removed redundant stuff for jokers
 *
 *   Revision 1.12  1999/12/14 19:10:50  tony
 *   got rid of this annoying message:
 *   RISK_GetNthPlayerAtClient got 0, numplayers returned 2. to fix!!
 *   lets hope the assertion is just wrong
 *
 *   Revision 1.11  1999/11/28 14:28:47  tony
 *   nothing special
 *
 *   Revision 1.10  1999/11/27 19:19:07  tony
 *   oops :-) only 40 in MessageNames
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "callbacks.h"
#include "network.h"
#include "utils.h"
#include "riskgame.h"
#include "debug.h"
#include "language.h"

  
/* Private variables */
static FILE *hLogFile = (FILE *)NULL;

/* Private functions */
static Int32    _RISK_GetMsgType(Int32 iField);
static Int32    _RISK_GetIntValue(Int32 iField, Int32 iIndex1, Int32 iIndex2); 
static CString  _RISK_GetStrValue(Int32 iField, Int32 iIndex1, Int32 iIndex2); 
static void    *_RISK_BuildMsg(Int32 iMessType, Int32 iField, 
			       Int32 iIndex1, Int32 iIndex2, 
			       CString strNewValue, Int32 iNewValue);
static void     _RISK_Replicate(Int32 iField, Int32 iIndex1, Int32 iIndex2, 
				CString strNewValue, Int32 iNewValue);

/****************************************/
typedef struct _ContinentObject
{
  CString  strName;
  Int32    iValue;
  Int32    iNumCountries;
} ContinentObject;

/****************************************/ 
typedef struct _CountryObject
{
  CString   strName;            /* Name of the country */
  Int32     iContinent;         /* The continent it forms part of */
  Int32     iNumArmies;         /* Number of armies on the country */
  Int32     iTextX, iTextY;     /* Where to write the text */
  Int32     piOwner;            /* The player it belongs to */
  Int32     piAdjCountries[6];  /* All attackable countries */
} CountryObject;

/****************************************/
typedef struct _PlayerObject
{	
  Int32      iAllocationState;
  Int32      iAttackMode, iDiceMode, iMsgDstMode;
  Int32      iSpecies;
  Flag       iState;
  Int32      iClient;
  CString    strName, strColor;
  Int32      iCountriesOwned, iNumArmies, iNumCards;
  Int32      piCards[MAX_CARDS];
  Int16      typOfMission;
  Int32      mission1, mission2;
} PlayerObject;

/****************************************/
typedef struct _SpeciesObject
{
  Int32    iAllocationState;
  Int32    iClient;
  CString  strName;
  CString  strAuthor;
  CString  strVersion;
  CString  strDescription;
  struct _SpeciesObject *next;
} SpeciesObject;


#ifdef LOGGING
static char * MessageNames[] = {
    "MSG_NOMESSAGE",
    "MSG_OLDREGISTERCLIENT",
    "unused 0x02",
    "MSG_EXCHANGECARDS",
    "MSG_REQUESTCARD",
    "MSG_CARDPACKET",
    "MSG_CARDPACKET",
    "MSG_SENDMESSAGE",
    "MSG_SENDMESSAGE",
    "MSG_EXIT",
    "MSG_STARTGAME",
    "MSG_TURNNOTIFY",
    "MSG_ENDTURN",
    "MSG_CLIENTIDENT",
    "MSG_ENTERSTATE",
    "unused 0x0F"
     "MSG_ENDOFGAME",
     "MSG_OBJSTRUPDATE",
    "MSG_OBJINTUPDATE",
    "PLR_MISSION",
    "unused 0x14",
    "unused 0x15",
    "MSG_DEREGISTERCLIENT",
    "unused 0x17",
    "unused 0x18",
    "MSG_ALLOCPLAYER"
    "unused 0x1a",
    "unused 0x1b",
    "unused 0x1c",
    "unused 0x1d",
    "unused 0x1e",
    "unused 0x1d",
    "MSG_FREEPLAYER"
    "MSG_NETMESSAGE"
    "MSG_NETPOPUP"
    "MSG_POPUPREGISTERBOX"
    "MSG_DICEROLL"
    "MSG_PLACENOTIFY"
    "MSG_ATTACKNOTIFY",
    "MSG_MOVENOTIFY",
    "MSG_POLLCLIENTS",
    "unused 0x29",
    "MSG_HELLO",
    "MSG_VERSION",
    "MSG_ENDOFMISSION",
     "MSG_VICTORY",
     "MSG_VICTORY"
};

#endif

/****************************************/
typedef struct _Game
{
  void               (*ReplicateCallback)(Int32, void *, Int32, Int32);
  void               (*FailureCallback)(CString, Int32);
  void               (*PreViewCallback)(Int32, void *);
  void               (*PostViewCallback)(Int32, void *);
  
  /* The databases */
  SpeciesObject      pSpecies;
  ContinentObject    pContinents[NUM_CONTINENTS];
  PlayerObject       pPlayers[MAX_PLAYERS];
  CountryObject      pCountries[NUM_COUNTRIES];
} Game;





/* The object (eventually do this in a loop?) */
static Game RiskGame =
{
    NULL, NULL, NULL, NULL,
    {
        ALLOC_COMPLETE,
        -1,HUMAN,UNKNOWN,
        "0.001",
        PINK_CREATURE,
        (SpeciesObject *)NULL
    },
    {
        { NORTH_AMERICA, 5, 9 },
        { SOUTH_AMERICA,  2, 4 },
        { AFRICA,         3, 6 },
        { AUSTRALIA,      2, 4 },
        { ASIA,           7, 12 },
        { EUROPE,         5, 7 },

    },
    {
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
	{0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  }, 
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
	{0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  }, 
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1  },
        { ALLOC_NONE, 1, 3, 0, 0, PLAYER_ALIVE, -1, NULL, NULL, 0, 0, 0,
        {0,0,0,0,0,0,0,0,0,0}, NO_MISSION, -1, -1 },
    },
    {

        { GREENLAND,  CNT_NORTHAMERICA,  0, 264,  32, -1, /* 0 */
        {1, 5, 10, 11, -1, -1} },
        { ICELAND,  CNT_EUROPE,        0, 342,  51, -1,
        {0, 7, 14, -1, -1, -1} },
        { SIBERIA,  CNT_ASIA,          0, 622,  91, -1,
        {3, 6, 13, 21, 22, -1} },
        { URAL,  CNT_ASIA,           0, 557, 106, -1,
        {2, 8, 15, 22, -1, -1} },
        { ALASKA,  CNT_NORTHAMERICA, 0,  13,  61, -1,  
        {5, 9, 12, -1, -1, -1} },
        { NORTH_WEST,  CNT_NORTHAMERICA,  0, 61, 62, -1, /* 5 */
        {0, 4, 11, 12, -1, -1} },
        { IRKUTSK,  CNT_ASIA,          0, 700,  71, -1,
        {2, 9, 13, -1, -1, -1} },
        { SCANDINAVIA, CNT_EUROPE,         0, 387,  85, -1,
        {1, 8, 14, 16, -1, -1} },
        { UKRAINE, CNT_EUROPE,        0, 450, 103, -1,
        {3, 7, 15, 16, 20, 24}},
        { KAMCHATKA, CNT_ASIA,          0, 731, 121, -1, 
        {4, 6, 13, 21, 23, -1} },
        { QUEBEC, CNT_NORTHAMERICA,  0, 181,  95, -1,     /* 10 */
        {0, 11, 18, -1, -1, -1} },
        { ONTARIO, CNT_NORTHAMERICA,  0, 116,  97, -1,
        {0, 5, 10, 12, 17, 18} },
        { ALBERTA, CNT_NORTHAMERICA,  0,  55,  90, -1,
        {4, 5, 11, 17, -1, -1} },
        { YAKUTSK, CNT_ASIA,          0, 690, 111, -1,
        {2, 6, 9, 21, -1, -1} },
        { GREAT_BRITAIN, CNT_EUROPE,        0, 338, 110, -1,  
        {1, 7, 16, 19, -1, -1} },
        { AFGHANISTAN, CNT_ASIA,          0, 534, 141, -1,  /* 15 */
        {3, 8, 22, 24, 28, -1} },
        { NORTHERN_EUROPE, CNT_EUROPE,    0, 386, 113, -1,
        {7, 8, 14, 19, 20, -1} },
        { WEST_STATES, CNT_NORTHAMERICA, 0, 62, 126, -1,
        {11, 12, 18, 25, -1, -1} },
        { EAST_STATES, CNT_NORTHAMERICA, 0, 126, 145, -1,
        {10, 11, 17, 25, -1, -1} },
        { WEST_EUROPE, CNT_EUROPE,    0, 338, 148, -1,  
        {14, 16, 20, 26, -1, -1} },
        { SOUTH_EUROPE, CNT_EUROPE,    0, 416, 134, -1, /* 20 */
        {8, 16, 19, 24, 26, 27} },
        { MONGOLIA, CNT_ASIA,          0, 685, 139, -1,
        {2, 9, 13, 22, 23, -1} },
        { CHINA,  CNT_ASIA,          0, 632, 163, -1,
        {2, 3, 15, 21, 28, 29} },
        { JAPAN, CNT_ASIA,          0, 735, 166, -1,
        {9, 21, -1, -1, -1, -1} },
        { MIDDLE_EAST, CNT_ASIA,          0, 477, 193, -1,  
        {8, 15, 20, 27, 28, 30} },
        { CENTRAL_AMERICA, CNT_NORTHAMERICA,  0, 87, 165, -1, /* 25 */
        {17, 18, 32, -1, -1, -1}},
        { NORTH_AFRICA, CNT_AFRICA,        0, 339, 214, -1,
        {19, 20, 27, 30, 33, 35} },
        { EGYPT, CNT_AFRICA,        0, 407, 194, -1,
        {20, 24, 26, 30, -1, -1} },
        { INDIA, CNT_ASIA,          0, 572, 194, -1,
        {15, 22, 24, 29, -1, -1} },
        { SIAM,  CNT_ASIA,          0, 637, 207, -1,  
        {22, 28, 31, -1, -1, -1} },
        { EAST_AFRICA, CNT_AFRICA,        0, 437, 231, -1, /* 30 */
        {24, 26, 27, 35, 37, 40} },
        { INDONESIA, CNT_AUSTRALIA,     0, 672, 262, -1,
        {29, 36, 38, -1, -1, -1} },
        { VENEZUELA, CNT_SOUTHAMERICA,  0, 188, 230, -1,
        {25, 33, 34, -1, -1, -1} },
        { BRASIL, CNT_SOUTHAMERICA,  0, 233, 277, -1,
        {26, 32, 34, 41, -1, -1} },
        { PERU, CNT_SOUTHAMERICA,  0, 194, 294, -1,  
        {32, 33, 41, -1, -1, -1}},
        { CONGO, CNT_AFRICA,        0, 410, 271, -1, /* 35 */
        {26, 30, 37, -1, -1, -1} },
        { NEW_GUINEA, CNT_AUSTRALIA,     0, 751, 276, -1,
        {31, 38, 39, -1, -1, -1} },
        { SOUTH_AFRICA, CNT_AFRICA,        0, 416, 326, -1,
        {30, 35, 40, -1, -1, -1} },
        { WESTERN_AUSTRALIA, CNT_AUSTRALIA,     0, 700, 335, -1,
        {31, 36, 39, -1, -1, -1} },
        { EASTERN_AUSTRALIA, CNT_AUSTRALIA,     0, 756, 343, -1,  
        {36, 38, -1, -1, -1, -1} },
        { MADAGASCAR, CNT_AFRICA,        0, 479, 321, -1, /* 40 */
        {30, 37, -1, -1, -1, -1} },
        { ARGENTINA,  CNT_SOUTHAMERICA,  0, 189, 352, -1,
        {33, 34, -1, -1, -1, -1} },
    }
};

/* Species related private function */
SpeciesObject *_RISK_GetSpecies(Int32 iHandle);


/**
 * Init callbacks
 *
 * \b  History:
 * \tag     07.17.94  ESF  Created.
 * \tag     08.13.94  ESF  Rewrote.
 * \b Notes:
 * \parThis function is called by both client and server.
 */
void RISK_InitObject(void (*ReplicateCallback)(Int32, void *, Int32, Int32),
		     void (*PreViewCallback)(Int32, void *),
		     void (*PostViewCallback)(Int32, void *),
		     void (*FailureCallback)(CString, Int32),
		     FILE *hDebug)
{
  RiskGame.ReplicateCallback = ReplicateCallback;
  RiskGame.FailureCallback   = FailureCallback;
  RiskGame.PreViewCallback   = PreViewCallback;
  RiskGame.PostViewCallback  = PostViewCallback;

  /* The message callback has to be set -- it handles replication. */
  D_Assert(RiskGame.ReplicateCallback, "ReplicateCallback needs to be set!");

  /* Save this */
  hLogFile = hDebug;
}


/************************************************************************ 
 *  FUNCTION: _RISK_Replicate
 *  HISTORY: 
 *     05.03.94  ESF  Created.
 *     06.24.94  ESF  Fixed memory leak.
 *     07.16.94  ESF  Added assert.
 *     08.16.94  ESF  Changed so that both can get callback (Server/Client).
 *     08.18.94  ESF  Changed so that callback handles actual replication.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void _RISK_Replicate(Int32 iField, Int32 iIndex1, Int32 iIndex2, 
		     CString strNewValue, Int32 iNewValue)
{
  void   *pvMess;
  Int32   iMessType;

  /* Get the message and the message type */
  iMessType = _RISK_GetMsgType(iField);
  pvMess    = _RISK_BuildMsg(iMessType, iField, 
			     iIndex1, iIndex2, 
			     strNewValue, iNewValue);
  D_Assert(pvMess, "Got back NULL message!");
  
  /* Call the callback for messages (before changing dist. obj.) */
  if (RiskGame.PreViewCallback)
    RiskGame.PreViewCallback(iMessType, pvMess);
  
  /* Actually set the value (sort of roundabout :) Everything
   * to set the message follows the same code path now, so
   * it may be a bit roundabout but at least it's consistant.
   */

  RISK_ProcessMessage(iMessType, pvMess);

  /* Call the callback for messages (after changing dist. obj.) */
  if (RiskGame.PostViewCallback)
    RiskGame.PostViewCallback(iMessType, pvMess);

  /* Call the message handler.  If there wasn't one, then
   * we would have a problem, since it handles the actual replication.
   * This involves sending a message to the server, or broadcasting
   * a message out, depending on whether a client or server has
   * registered this callback.  The last parameter doesn't matter in
   * this case, because it's an outgoing message (the last parameter
   * is the message source).
   */
  
  RiskGame.ReplicateCallback(iMessType, pvMess, MESS_OUTGOING, -1);
  
  /* Delete the memory the message was taking */
  MEM_Free(pvMess);
}


/************************************************************************ 
 *  FUNCTION: RISK_SelectiveReplicate
 *  HISTORY: 
 *     05.06.94  ESF  Created.
 *     05.10.94  ESF  Added needed check for existence of callback.
 *     07.16.94  ESF  Fixed loop bug, added assertions.
 *     08.28.94  ESF  Changed to take indices, instead of ranges.
 *     01.01.95  ESF  Fixed to remove a memory leak.
 *  PURPOSE: 
 *     Sends data to a client, for purposes of data synchronicity.
 *  NOTES: 
 ************************************************************************/
void RISK_SelectiveReplicate(Int32 iSocket, Int32 iField, 
			     Int32 iIndex1, Int32 iIndex2)
{
  void     *pvMess;
  Int32     iValue = 0, iMessType;
  CString   strValue = NULL;

  /* Type of the message */
  iMessType = _RISK_GetMsgType(iField);

  /* Get the value... */
  if (iMessType == MSG_OBJINTUPDATE)
    iValue = _RISK_GetIntValue(iField, iIndex1, iIndex2);
  else
    strValue = _RISK_GetStrValue(iField, iIndex1, iIndex2);

  /* And build the message... */
  pvMess = _RISK_BuildMsg(iMessType, iField, 
			  iIndex1, iIndex2, 
			  strValue, iValue);

  (void)RISK_SendMessage(iSocket, iMessType, pvMess);
  
  /* Delete the memory the message took up */
  MEM_Free(pvMess);
}


/************************************************************************ 
 *  FUNCTION: RISK_ResetObj
 *  HISTORY: 
 *     05.02.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void RISK_ResetObj(void)
{
  Int32 i, iPlayer;

  /* Init the needed fields of the players */
  for (i=0; i<RISK_GetNumPlayers(); i++)
    {
      iPlayer = RISK_GetNthPlayer(i);

      RISK_SetAttackModeOfPlayer(iPlayer, 1);
      RISK_SetStateOfPlayer(iPlayer, PLAYER_ALIVE);
      RISK_SetClientOfPlayer(iPlayer, -1);
      RISK_SetNumCountriesOfPlayer(iPlayer, 0);
      RISK_SetNumArmiesOfPlayer(iPlayer, 0);
      RISK_SetNumCardsOfPlayer(iPlayer, 0);
      RISK_SetAllocationStateOfPlayer(iPlayer, ALLOC_NONE);
      RISK_SetMissionTypeOfPlayer(iPlayer, NO_MISSION);
    }

  /* Init the needed fields of the countries */
  for (i=0; i!=NUM_COUNTRIES; i++)
    {
      RISK_SetOwnerOfCountry(i, -1);
      RISK_SetNumArmiesOfCountry(i, 0);
    }
}


/************************************************************************ 
 *  FUNCTION: RISK_ResetGame
 *  HISTORY: 
 *     05.12.94  ESF  Created.
 *  PURPOSE:
 *     New game, keep players.
 *  NOTES: 
 ************************************************************************/
void RISK_ResetGame(void)
{
  Int32 i, iPlayer;

  /* Init the needed fields of the players */
  for (i=0; i<RISK_GetNumPlayers(); i++)
    {
      iPlayer = RISK_GetNthPlayer(i);

      RISK_SetStateOfPlayer(iPlayer, PLAYER_ALIVE);
      RISK_SetNumCountriesOfPlayer(iPlayer, 0);
      RISK_SetNumCardsOfPlayer(iPlayer, 0);
      RISK_SetNumArmiesOfPlayer(iPlayer, 0);
      RISK_SetMissionTypeOfPlayer(iPlayer, NO_MISSION);
    }

  /* Init the needed fields of the countries */
  for (i=0; i!=NUM_COUNTRIES; i++)
    {
      RISK_SetOwnerOfCountry(i, -1);
      RISK_SetNumArmiesOfCountry(i, 0);
    }
}


/************************************************************************ 
 *  FUNCTION: RISK_ProcessMessage
 *  HISTORY: 
 *     05.02.94  ESF  Created.
 *     05.10.94  ESF  Added needed check for existence of callback.
 *     02.15.95  ESF  Moved the callback code to _after_ the update.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void RISK_ProcessMessage(Int32 iMessType, void *pvMess)
{
  Int32               iIndex1, iIndex2, iValue, iField;
  CString             strValue;
  MsgObjIntUpdate    *pIntMess = (MsgObjIntUpdate *)pvMess;
  MsgObjStrUpdate    *pStrMess = (MsgObjStrUpdate *)pvMess;

  D_Assert(pvMess, "pvMess is NULL!");
  
  if (iMessType == MSG_OBJINTUPDATE)
    {
      iField  = pIntMess->iField;
      iIndex1 = pIntMess->iIndex1;
      iIndex2 = pIntMess->iIndex2;
      iValue  = pIntMess->iNewValue;
      
      switch (iField)
	{
	case PLR_ATTACKMODE:
	  RiskGame.pPlayers[iIndex1].iAttackMode = iValue;
	  break;
	case PLR_DICEMODE:
	  RiskGame.pPlayers[iIndex1].iDiceMode = iValue;
	  break;
	case PLR_MSGDSTMODE:
	  RiskGame.pPlayers[iIndex1].iMsgDstMode = iValue;
	  break;
	case PLR_STATE:
	  RiskGame.pPlayers[iIndex1].iState = iValue;
	  break;
	case PLR_CLIENT:
	  RiskGame.pPlayers[iIndex1].iClient = iValue;
	  break;
	case PLR_NUMCOUNTRIES:
	  RiskGame.pPlayers[iIndex1].iCountriesOwned = iValue;
	  break;
	case PLR_NUMARMIES:
	  RiskGame.pPlayers[iIndex1].iNumArmies = iValue;
	  break;
	case PLR_NUMCARDS:
	  RiskGame.pPlayers[iIndex1].iNumCards = iValue;
	  break;
	case PLR_SPECIES:
	  RiskGame.pPlayers[iIndex1].iSpecies = iValue;
	  break;
	case PLR_ALLOCATION:
	  RiskGame.pPlayers[iIndex1].iAllocationState = iValue;
	  break;
        case PLR_CARD: /* TdH: one of the places where greenlandbug might be caught */
	  RiskGame.pPlayers[iIndex1].piCards[iIndex2] = iValue;
	  break;
	case PLR_MISSION:
	  RiskGame.pPlayers[iIndex1].typOfMission = iValue;
	  break;
	case PLR_MISSION1:
	  RiskGame.pPlayers[iIndex1].mission1 = iValue;
	  break;
	case PLR_MISSION2:
	  RiskGame.pPlayers[iIndex1].mission2 = iValue;
	  break;
	case CNT_NUMARMIES:
	  RiskGame.pCountries[iIndex1].iNumArmies = iValue;
	  break;
	case CNT_OWNER:
	  RiskGame.pCountries[iIndex1].piOwner = iValue;
	  break;
	case SPE_CLIENT:
	  {
	    SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	    s->iClient = iValue;
	  }
	  break;
	case SPE_ALLOCATION:
	  {
	    SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	    s->iAllocationState = iValue;
	  }
	  break;
	
	default:
	  D_Assert(FALSE, "Badly build MSG_OBJINTUPDATE!");
	}
    }
  else
    {
      iField  = pStrMess->iField;
      iIndex1 = pStrMess->iIndex1;
      iIndex2 = pStrMess->iIndex2;
      strValue  = pStrMess->strNewValue;

      switch (iField)
	{
	case PLR_NAME:
	  if (RiskGame.pPlayers[iIndex1].strName != NULL)
	    MEM_Free(RiskGame.pPlayers[iIndex1].strName);
	  if (strValue)
	    {
	      RiskGame.pPlayers[iIndex1].strName = 
		(CString)MEM_Alloc(strlen(strValue)+1);
	      strcpy(RiskGame.pPlayers[iIndex1].strName, strValue);
	    }
	  else
	    RiskGame.pPlayers[iIndex1].strName = NULL;
	  break;
	case PLR_COLORSTRING:
	  if (RiskGame.pPlayers[iIndex1].strColor != NULL)
	    MEM_Free(RiskGame.pPlayers[iIndex1].strColor);
	  if (strValue)
	    {
	      RiskGame.pPlayers[iIndex1].strColor = 
		(CString)MEM_Alloc(strlen(strValue)+1);
	      strcpy(RiskGame.pPlayers[iIndex1].strColor, strValue);
	    }
	  else
	    RiskGame.pPlayers[iIndex1].strColor = NULL;
	  break;
	case SPE_NAME:
	  {
	    SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	    
	    if (s->strName != NULL)
	      MEM_Free(s->strName);
	    
	    if (strValue)
	      {
		s->strName = (CString)MEM_Alloc(strlen(strValue)+1);
		strcpy(s->strName, strValue); 
	      }
	    else
	      s->strName = NULL;
	  }
	  break;
	case SPE_VERSION:
	  {
	    SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	    
	    if (s->strVersion != NULL)
	      MEM_Free(s->strVersion);
	    
	    if (strValue)
	      {
		s->strVersion = (CString)MEM_Alloc(strlen(strValue)+1);
		strcpy(s->strVersion, strValue); 
	      }
	    else
	      s->strVersion = NULL;
	  }
	  break;
	case SPE_DESCRIPTION:
	  {
	    SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	    
	    if (s->strDescription != NULL)
	      MEM_Free(s->strDescription);
	    
	    if (strValue)
	      {
		s->strDescription = (CString)MEM_Alloc(strlen(strValue)+1);
		strcpy(s->strDescription, strValue); 
	      }
	    else
	      s->strDescription = NULL;
	  }
	  break;
	case SPE_AUTHOR:
	  {
	    SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	    
	    if (s->strAuthor != NULL)
	      MEM_Free(s->strAuthor);
	    
	    if (strValue)
	      {
		s->strAuthor = (CString)MEM_Alloc(strlen(strValue)+1);
		strcpy(s->strAuthor, strValue); 
	      }
	    else
	      s->strAuthor = strValue;
	  }
	  break;

	default:
	  D_Assert(FALSE, "Badly build MSG_OBJSTRUPDATE!");
	}
    }
}

/***************************************/
void RISK_SetAttackModeOfPlayer(Int32 iPlayer, Int32 iMode)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_ATTACKMODE, iPlayer, 0, NULL, iMode);
}

/***************************************/
void RISK_SetDiceModeOfPlayer(Int32 iPlayer, Int32 iMode)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_DICEMODE, iPlayer, 0, NULL, iMode);
}

/***************************************/
void RISK_SetMsgDstModeOfPlayer(Int32 iPlayer, Int32 iMode)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_MSGDSTMODE, iPlayer, 0, NULL, iMode);
}

/***************************************/
void RISK_SetStateOfPlayer(Int32 iPlayer, Int32 iState)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_STATE, iPlayer, 0, NULL, iState);
}

/***************************************/
void RISK_SetClientOfPlayer(Int32 iPlayer, Int32 iClient)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_CLIENT, iPlayer, 0, NULL, iClient);
}

/***************************************/
void RISK_SetNumCountriesOfPlayer(Int32 iPlayer, Int32 iNumCountries)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_NUMCOUNTRIES, iPlayer, 0, NULL, iNumCountries);
}

/***************************************/
void RISK_SetNumArmiesOfPlayer(Int32 iPlayer, Int32 iNumArmies)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_NUMARMIES, iPlayer, 0, NULL, iNumArmies);
}

/***************************************/
void RISK_SetNumCardsOfPlayer(Int32 iPlayer, Int32 iNumCards)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_NUMCARDS, iPlayer, 0, NULL, iNumCards);
}

/***************************************/
void RISK_SetNameOfPlayer(Int32 iPlayer, CString strName)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_NAME, iPlayer, 0, strName, 0);
}

/***************************************/
void RISK_SetColorCStringOfPlayer(Int32 iPlayer, CString strColor)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_COLORSTRING, iPlayer, 0, strColor, 0);
}

/***************************************/
void RISK_SetCardOfPlayer(Int32 iPlayer, Int32 iCard, Int32 iValue)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_CARD, iPlayer, iCard, NULL, iValue);
}

/***************************************/
void RISK_SetMissionTypeOfPlayer(Int32 iPlayer, Int16 typ)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_MISSION , iPlayer, 0, NULL, typ);
}

/***************************************/
void RISK_SetMissionNumberOfPlayer(Int32 iPlayer, Int32 n)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_MISSION1 , iPlayer, 0, NULL, n);
}

/***************************************/
void RISK_SetMissionContinent1OfPlayer(Int32 iPlayer, Int32 n)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_MISSION1 , iPlayer, 0, NULL, n);
}

/***************************************/
void RISK_SetMissionContinent2OfPlayer(Int32 iPlayer, Int32 n)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_MISSION2 , iPlayer, 0, NULL, n);
}

/***************************************/
void RISK_SetMissionMissionPlayerToKillOfPlayer(Int32 iPlayer, Int32 n)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_MISSION1 , iPlayer, 0, NULL, n);
}

/***************************************/
void RISK_SetMissionPlayerIsKilledOfPlayer(Int32 iPlayer, Flag boool)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_MISSION2 , iPlayer, 0, NULL, boool);
}

/***************************************/
void RISK_SetSpeciesOfPlayer(Int32 iPlayer, Int32 iSpecies)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_SPECIES, iPlayer, 0, NULL, iSpecies);
}

/***************************************/
void RISK_SetAllocationStateOfPlayer(Int32 iPlayer, Int32 iState)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  _RISK_Replicate(PLR_ALLOCATION, iPlayer, 0, NULL, iState);
}

/***************************************/
void RISK_SetNumArmiesOfCountry(Int32 iCountry, Int32 iNumArmies)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  _RISK_Replicate(CNT_NUMARMIES, iCountry, 0, NULL, iNumArmies);
}

/***************************************/
void RISK_SetOwnerOfCountry(Int32 iCountry, Int32 iOwner)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  _RISK_Replicate(CNT_OWNER, iCountry, 0, NULL, iOwner);
}

/***************************************/
void RISK_SetClientOfSpecies(Int32 iHandle, Int32 iClient)
{
 _RISK_Replicate(SPE_CLIENT, iHandle, 0, NULL, iClient);
}

/***************************************/
void RISK_SetNameOfSpecies(Int32 iHandle, CString strName)
{
  _RISK_Replicate(SPE_NAME, iHandle, 0, strName, 0); 
}

/***************************************/
void RISK_SetAllocationStateOfSpecies(Int32 iHandle, Int32 iState)
{
  _RISK_Replicate(SPE_ALLOCATION, iHandle, 0, NULL, iState);
}

/***************************************/
void RISK_SetAuthorOfSpecies(Int32 iHandle, CString strAuthor)
{
  _RISK_Replicate(SPE_AUTHOR, iHandle, 0, strAuthor, 0); 
}

/***************************************/
void RISK_SetVersionOfSpecies(Int32 iHandle, CString strVersion)
{
  _RISK_Replicate(SPE_VERSION, iHandle, 0, strVersion, 0); 
}

/***************************************/
void RISK_SetDescriptionOfSpecies(Int32 iHandle, CString strDescription)
{
  _RISK_Replicate(SPE_DESCRIPTION, iHandle, 0, strDescription, 0); 
}

/**********************************************************************
 *  PURPOSE: determine number of initialized players
 *  NOTES: 
 *********************************************************************/
Int32 RISK_GetNumPlayers(void) 
{
  Int32 i, iCount;

  for (i=iCount=0; i < MAX_PLAYERS; i++)
    if (RISK_GetAllocationStateOfPlayer(i) == ALLOC_COMPLETE)
      iCount++;

  return iCount;
}

/***************************************/
Int32 RISK_GetAttackModeOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iAttackMode;
}

/***************************************/
Int32 RISK_GetMsgDstModeOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iMsgDstMode;
}

/***************************************/
Int32 RISK_GetDiceModeOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iDiceMode;
}

/***************************************/
Int32 RISK_GetStateOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iState;
}

/***************************************/
Int32 RISK_GetClientOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iClient;
}

/***************************************/
Int32 RISK_GetAllocationStateOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iAllocationState;
}

/***************************************/
Int32 RISK_GetNumCountriesOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iCountriesOwned;
}

/***************************************/
Int32 RISK_GetNumArmiesOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iNumArmies;
}

/***************************************/
Int32 RISK_GetNumCardsOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iNumCards;
}

/***************************************/
Int32 RISK_GetSpeciesOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].iSpecies;
}

/***************************************/
Int32 RISK_GetValueOfContinent(Int32 iContinent)
{
  D_Assert(iContinent>=0 && iContinent<NUM_CONTINENTS, "Bogus continent!");
  return RiskGame.pContinents[iContinent].iValue;
}

/***************************************/
CString RISK_GetNameOfContinent(Int32 iContinent)
{
  D_Assert(iContinent>=0 && iContinent<NUM_CONTINENTS, "Bogus continent!");
  return RiskGame.pContinents[iContinent].strName;
}

/***************************************/
Int32 RISK_GetNumCountriesOfContinent(Int32 iContinent)
{
  D_Assert(iContinent>=0 && iContinent<NUM_CONTINENTS, "Bogus continent!");
  return RiskGame.pContinents[iContinent].iNumCountries;
}

/***************************************/
CString RISK_GetNameOfPlayer(Int32 iPlayer) 
{ 
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  /* 
   * This patches the symptom of a problem whose cause I haven't found yet.
   * --Pac. 
   *
   *  return RiskGame.pPlayers[iPlayer].strName;
   */
  return RiskGame.pPlayers[iPlayer].strName ?
         RiskGame.pPlayers[iPlayer].strName :
	 "UnknownPlayerBug";
}

/***************************************/
CString RISK_GetColorCStringOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].strColor;
}

/***************************************/
Int32 RISK_GetCardOfPlayer(Int32 iPlayer, Int32 iCard)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].piCards[iCard];
}

/***************************************/
Int16 RISK_GetMissionTypeOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].typOfMission;
}

/***************************************/
Int32 RISK_GetMissionContinent1OfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].mission1;
}

/***************************************/
Int32 RISK_GetMissionNumberOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].mission1;
}

/***************************************/
Int32 RISK_GetMissionContinent2OfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].mission2;
}

/***************************************/
Int32 RISK_GetMissionPlayerToKillOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].mission1;
}

/***************************************/
Flag RISK_GetMissionIsPlayerKilledOfPlayer(Int32 iPlayer)
{
  D_Assert(iPlayer>=0 && iPlayer<MAX_PLAYERS, "Bogus player!");
  return RiskGame.pPlayers[iPlayer].mission2;
}

/***************************************/
CString RISK_GetNameOfCountry(Int32 iCountry)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  return RiskGame.pCountries[iCountry].strName;
}

/***************************************/
Int32 RISK_GetContinentOfCountry(Int32 iCountry)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  return RiskGame.pCountries[iCountry].iContinent;
}

/***************************************/
Int32 RISK_GetNumArmiesOfCountry(Int32 iCountry)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  return RiskGame.pCountries[iCountry].iNumArmies;
}

/***************************************/
Int32 RISK_GetOwnerOfCountry(Int32 iCountry)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  return RiskGame.pCountries[iCountry].piOwner;
}

/***************************************/
Int32 RISK_GetAdjCountryOfCountry(Int32 iCountry, Int32 iIndex)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  return RiskGame.pCountries[iCountry].piAdjCountries[iIndex];
}

/***************************************/
Int32 RISK_GetNumLivePlayers(void)
{
  Int32 i, iCount;

  for (i=iCount=0; i < MAX_PLAYERS; i++)
    if (RISK_GetAllocationStateOfPlayer(i) == ALLOC_COMPLETE &&
	RISK_GetStateOfPlayer(i) == PLAYER_ALIVE)
      iCount++;
  
  return iCount;
}

/***************************************/
Int32 RISK_GetNumSpecies(void)
{
  Int32           iCount;
  SpeciesObject  *pSpecies;

  for (pSpecies = &RiskGame.pSpecies, iCount = 0;
       pSpecies != NULL;
       pSpecies = pSpecies->next)
    if (pSpecies->iAllocationState == ALLOC_COMPLETE)
      iCount++;

  return iCount;
}

/***************************************/
Int32 RISK_GetTextXOfCountry(Int32 iCountry)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  return RiskGame.pCountries[iCountry].iTextX;
}

/***************************************/
Int32 RISK_GetTextYOfCountry(Int32 iCountry)
{
  D_Assert(iCountry>=0 && iCountry<NUM_COUNTRIES, "Bogus country!");
  return RiskGame.pCountries[iCountry].iTextY;
}

/***************************************/
Int32 RISK_GetClientOfSpecies(Int32 iHandle)
{
  return _RISK_GetSpecies(iHandle)->iClient; 
}

/***************************************/
CString RISK_GetNameOfSpecies(Int32 iHandle)
{
  return _RISK_GetSpecies(iHandle)->strName;
}

/***************************************/
Int32 RISK_GetAllocationStateOfSpecies(Int32 iHandle)
{
  return _RISK_GetSpecies(iHandle)->iAllocationState;
}

/***************************************/
CString RISK_GetAuthorOfSpecies(Int32 iHandle)
{
  return _RISK_GetSpecies(iHandle)->strAuthor;
}

/***************************************/
CString RISK_GetDescriptionOfSpecies(Int32 iHandle)
{
  return _RISK_GetSpecies(iHandle)->strDescription;
}

/***************************************/
CString RISK_GetVersionOfSpecies(Int32 iHandle)
{
  return _RISK_GetSpecies(iHandle)->strVersion;
}


/***************************************/
Int32 _RISK_GetMsgType(Int32 iField)
{
  switch (iField)
    {
    case PLR_ATTACKMODE:
    case PLR_DICEMODE:
    case PLR_MSGDSTMODE:
    case PLR_STATE:     
    case PLR_SPECIES:
    case PLR_ALLOCATION:
    case PLR_CLIENT:    
    case PLR_NUMCOUNTRIES:
    case PLR_NUMARMIES:   
    case PLR_NUMCARDS:    
    case PLR_CARD:        
    case PLR_MISSION:
    case PLR_MISSION1:
    case PLR_MISSION2:
    case CNT_CONTINENT:   
    case CNT_NUMARMIES:   
    case CNT_OWNER:     
    case CNT_ADJCOUNTRY:  
    case CNT_TEXTX:       
    case CNT_TEXTY:       
    case CON_VALUE:         
    case CON_NUMCOUNTRIES:  
    case SPE_CLIENT:
    case SPE_ALLOCATION:
      return MSG_OBJINTUPDATE;
      break;
      
    case PLR_NAME:        
    case PLR_COLORSTRING: 
    case CNT_NAME:        
    case CON_NAME:
    case SPE_NAME:
    case SPE_VERSION:
    case SPE_DESCRIPTION:
    case SPE_AUTHOR:
        return MSG_OBJSTRUPDATE;
    default:


 D_Assert(FALSE, "Shouldn't be here!");

    }
  
  /* For the compiler */
  return (0);
}

/***************************************/
void *_RISK_BuildMsg(Int32 iMessType, Int32 iField, 
		     Int32 iIndex1, Int32 iIndex2,
		     CString strValue, Int32 iValue)
{
  if (iMessType == MSG_OBJINTUPDATE)
    {
      MsgObjIntUpdate *pIntMess = 
	(MsgObjIntUpdate *)MEM_Alloc(sizeof(MsgObjIntUpdate));
      
      /* Build the new message */
      pIntMess->iField    = iField;
      pIntMess->iIndex1   = iIndex1;
      pIntMess->iIndex2   = iIndex2;
      pIntMess->iNewValue = iValue;
      
      /* Return it */
      return pIntMess;
    }
  else if (iMessType == MSG_OBJSTRUPDATE)
    {
      MsgObjStrUpdate *pStrMess = 
	(MsgObjStrUpdate *)MEM_Alloc(sizeof(MsgObjStrUpdate));

      /* Build the new message */
      pStrMess->iField      = iField;
      pStrMess->iIndex1     = iIndex1;
      pStrMess->iIndex2     = iIndex2;
      pStrMess->strNewValue = strValue;

      /* Return it */
      return pStrMess;
    }

  D_Assert(FALSE, "Shouldn't be here!!");

  /* Make the stupid compiler happy */
  return NULL;
}


/************************************************************************ 
 *  FUNCTION: RISK_ObjectFailure
 *  HISTORY: 
 *     08.03.94  ESF  Created.
 *     08.12.94  ESF  Made nicer, integrated with sister routine.
 *     08.17.94  ESF  Added string parameter.
 *     08.26.94  ESF  Deleted integer parameter :)
 *     08.28.94  ESF  Added callback for eventual recovery.
 *     10.01.94  ESF  Added assertion.
 *  PURPOSE: 
 *     Eventually this will handle fault tolerance.
 *  NOTES: 
 ************************************************************************/
void RISK_ObjectFailure(CString strReason, Int32 iCommLink)
{
  D_Assert(RiskGame.FailureCallback, "Need to register a failure callback!");
  RiskGame.FailureCallback(strReason, iCommLink);
}


/************************************************************************ 
 *  FUNCTION: RISK_GetNthLivePlayer
 *  HISTORY: 
 *     05.15.94  ESF  Created.
 *     07.25.94  ESF  Fixed a bug with the for loop, off-by-one.
 *     08.08.94  ESF  Moved here from server.c
 *     08.27.94  ESF  Renamed.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 RISK_GetNthLivePlayer(Int32 iIndex)
{
  Int32 i, iCount;

  D_Assert(iIndex>=0 && iIndex<RISK_GetNumLivePlayers(), 
	   "GetNthLivePlayer passed a bogus index!");

  /* Increment, because the index passed in will start at zero */
  iIndex++;

  /* Loop through, looking for the iIndex'th allocated player. */
  for (i=0, iCount=0; i!=MAX_PLAYERS && iIndex!=iCount; i++)
    if (RISK_GetAllocationStateOfPlayer(i) == ALLOC_COMPLETE &&
	RISK_GetStateOfPlayer(i) == PLAYER_ALIVE)
      iCount++;
  
  if (iIndex == iCount)
    return (i-1);
  
  /* Something wierd... */
  D_Assert(FALSE, "Something wierd happened!");
  
  /* Make the compiler happy */
  return (-1);
}


/************************************************************************ 
 *  FUNCTION: RISK_GetNthPlayerAtClient
 *  HISTORY: 
 *     10.02.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: bug!! check out, fires when ai joins and assertion is on
 *         now also crashed on the assertion itself
 ************************************************************************/
Int32 RISK_GetNthPlayerAtClient(Int32 iClient, Int32 iIndex)
{
  Int32 i, iCount;

  /* ok, seems to be harmless, guess this assertion is not needed, and even wrong
   printf("RISK_GetNthPlayerAtClient got %d, numplayers returned %d. to fix!!\n",iIndex,RISK_GetNumPlayers());
   */
  /* tony: attention! ai's cause this one to fire */
/*  printf("before assertion\n");
  D_Assert(iIndex>=0 && iIndex<RISK_GetNumPlayers(), "RISK_GetNthPlayerAtClient was passed a bogus index!");
  printf("passed assertion\n");*/
  /* Increment, because the index passed in will start at zero */
  iIndex++;

  /* Loop through, looking for the iIndex'th player */
  for (i=0, iCount=0; i!=MAX_PLAYERS && iIndex!=iCount; i++)
    if (RISK_GetClientOfPlayer(i) == iClient)
      iCount++;
  
  if (iIndex == iCount)
    return (i-1);

  /* Something wierd...
   D_Assert(FALSE, "Something weird happened!");
   */

  /* Make the compiler happy */
  return (-1);
}


/************************************************************************ 
 *  FUNCTION: RISK_GetNthPlayer
 *  HISTORY: 
 *     08.27.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 RISK_GetNthPlayer(Int32 iIndex)
{
  Int32 i, iCount;

  D_Assert(iIndex>=0 && iIndex<RISK_GetNumPlayers(), 
	   "GetNthPlayer passed a bogus index!");

  /* Increment, because the index passed in will start at zero */
  iIndex++;

  /* Loop through, looking for the iIndex'th player */
  for (i=0, iCount=0; i!=MAX_PLAYERS && iIndex!=iCount; i++)
    if (RISK_GetAllocationStateOfPlayer(i) == ALLOC_COMPLETE)
      iCount++;
  
  if (iIndex == iCount)
    return (i-1);

  /* Something wierd... */
  D_Assert(FALSE, "Something wierd happened!");

  /* Make the compiler happy */
  return (-1);
}


/************************************************************************ 
 *  FUNCTION: RISK_SendMessage
 *  HISTORY: 
 *     08.12.94  ESF  Created.
 *     08.17.94  ESF  Fixed the failure handling.
 *     10.01.94  ESF  Added return parameter.
 *     03.30.95  ESF  Added callback calling.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 RISK_SendMessage(Int32 iDest, Int32 iMessType, void *pvMessage)
{
  /* Call the callback for messages if it's not a dist. obj. related msg. */
  if (iMessType != MSG_OBJINTUPDATE && iMessType != MSG_OBJSTRUPDATE)
    {
      if (RiskGame.PreViewCallback)
	RiskGame.PreViewCallback(iMessType, pvMessage);
      
      if (RiskGame.PostViewCallback)
	RiskGame.PostViewCallback(iMessType, pvMessage);
    }

  /* Try to send it, if it fails, call the appropriate recovery routines */
  if (NET_SendMessage(iDest, iMessType, pvMessage) < 0)
    {
      RISK_ObjectFailure(TXT_SEND_FAILED, iDest);
      return 0;
    }


#ifdef LOGGING
  /* Log it if there's a file */
  if (hLogFile)
    {
        fprintf(hLogFile, "** Sent Message to (%d): %s %s\n", iDest,
                               iMessType < 40 ? MessageNames[iMessType] : "higher",
      	      NET_MessageToString(iMessType, pvMessage));
      fflush(hLogFile);
    }
#endif

  return 1;
}


/************************************************************************ 
 *  FUNCTION: RISK_ReceiveMessage
 *  HISTORY: 
 *     08.12.94  ESF  Created.
 *     08.17.94  ESF  Fixed the failure handling.
 *     10.01.94  ESF  Added return parameter.
 *     02.25.95  ESF  Changed to internalize handling of OBJ[INT|STR]UPDATE.
 *  PURPOSE: Handle incoming message
 *  NOTES: 
 ************************************************************************/
Int32 RISK_ReceiveMessage(Int32 iSource, Int32 *piMessType, void **ppvMessage)
{
  /* Init these */
  *ppvMessage = NULL;
  *piMessType = MSG_NOMESSAGE;

  /* Try to receive it, if it fails, call the appropriate recovery routines */
  if (NET_RecvMessage(iSource, piMessType, ppvMessage) < 0)
    {
      RISK_ObjectFailure(TXT_RECEIVE_FAILED, iSource);
      return 0;
    }


#ifdef LOGGING
  /* Log it if there's a file */
  if (hLogFile)
    {
      fprintf(hLogFile, "** Rec. Message from (%d): %s\n", iSource,
	      NET_MessageToString(*piMessType, *ppvMessage));
      fflush(hLogFile);
    }
#endif
  
  /* Call the callback for messages (before changing dist. obj.) */
  if (RiskGame.PreViewCallback)
    RiskGame.PreViewCallback(*piMessType, *ppvMessage);
  
  /* If the message coming in is a dist. obj. msg., then don't 
   * let it get through to the upper layers, as they shouldn't
   * need to get up there for any reason.
   */

  if (*piMessType == MSG_OBJINTUPDATE || *piMessType == MSG_OBJSTRUPDATE)
    {
      RISK_ProcessMessage(*piMessType, *ppvMessage);

      /* Call the callback for messages (after changing dist. obj.) */
      if (RiskGame.PostViewCallback)
	RiskGame.PostViewCallback(*piMessType, *ppvMessage);
      
      /* Call the replicate message */
      RiskGame.ReplicateCallback(*piMessType, *ppvMessage, 
				 MESS_INCOMING, iSource);

      /* Done with the message */
      NET_DeleteMessage(*piMessType, *ppvMessage);

      /* Nobody above here should get wind of this! */
      *piMessType = MSG_NOMESSAGE;
      *ppvMessage = NULL;
    }
  else
    /* Call the callback for messages */
    if (RiskGame.PostViewCallback)
      RiskGame.PostViewCallback(*piMessType, *ppvMessage);
  
  D_Assert(*piMessType != MSG_OBJINTUPDATE && 
	   *piMessType != MSG_OBJSTRUPDATE,
	   "I shouldn't be returning this message!");
  return 1;
}


/************************************************************************ 
 *  FUNCTION: RISK_SendSyncMessage
 *  HISTORY: 
 *     03.03.94  ESF  Created.
 *     08.03.94  ESF  Fixed to return error message. 
 *     02.25.95  ESF  Moved to riskgame.c
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 RISK_SendSyncMessage(Int32 iSocket, Int32 iSendMessType, 
			   void *pvMessage, Int32 iReturnMessType,
			   void (*CBK_MessageReceived)(Int32, void *))
{
  Int32     iMessType;
  void     *pvMess;

  /* Send the message */
  (void)RISK_SendMessage(iSocket, iSendMessType, pvMessage);

  /* Loop, until we receive the desired message, dispatching 
   * all others to the specified callback.
   */
  
  for (;;)
    {
      /* This will block when there is no input */
      (void)RISK_ReceiveMessage(iSocket, &iMessType, &pvMess);
      
      /* If we received the message we were looking for, 
       * then dispatch it and return finally.
       */
      
      CBK_MessageReceived(iMessType, pvMess);
      NET_DeleteMessage(iMessType, pvMess);

      if (iMessType == iReturnMessType)
	  return (1);
    }

  /* For the compiler */
  /*return (1);*/
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     08.31.94  ESF  Created.
 *     11.13.00  TdH  added inum
 *  PURPOSE: The number of players connected from a client
 *  NOTES: 
 *     This is a worst case O^2 algorithm.  It could be sped up to
 *   constant time if the number of players was kept track of, but
 *   I doubt this will be much of an overhead.
 ************************************************************************/
Int32 RISK_GetNumPlayersOfClient(Int32 iClient)
{
  Int32 i, iCount, inum;

  D_Assert(iClient>=0 && iClient<MAX_CLIENTS, "Bogus client!");

  /* Count up all of the players at this client */
  inum = RISK_GetNumPlayers();
  for (i=iCount=0; i<inum; i++)
    if (RISK_GetClientOfPlayer(RISK_GetNthPlayer(i)) == iClient)
      iCount++;

  return iCount;
}


/************************************************************************ 
 *  FUNCTION: RISK_GetNumLivePlayersOfClient
 *  HISTORY: 
 *     08.31.94  ESF  Created.
 *     10.01.94  ESF  Fixed a bug, changed NumPlayers to NumLivePlayers.
 *  PURPOSE: 
 *  NOTES: 
 *     This is a worst case O^2 algorithm.  It could be sped up to
 *   constant time if the number of live players was kept track of, but
 *   I doubt this will be much of an overhead.
 ************************************************************************/
Int32 RISK_GetNumLivePlayersOfClient(Int32 iClient)
{
  Int32 i, iCount;

  D_Assert(iClient>=0 && iClient<MAX_CLIENTS, "Bogus client!");

  /* Count up all of the players at this client */
  for (i=iCount=0; i!=RISK_GetNumLivePlayers(); i++)
    if (RISK_GetClientOfPlayer(RISK_GetNthLivePlayer(i)) == iClient)
      iCount++;

  return iCount;
}


/************************************************************************ 
 *  FUNCTION: RISK_SaveObject
 *  HISTORY: 
 *     09.14.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void RISK_SaveObject(CString strFileName)
{
  UNUSED(strFileName);
  /* I need to write _RISK_[Read|Write][CString|Int32eger], just like
   * for network.  Use the htonl() so that files saved on one
   * endianness can be read on another.  I don't need to save the
   * read-only fields, like the ContinentDatabase stuff.  Since I
   * will save the ID of the field, old game files should work for
   * future version of the game, as long as I don't fiddle with the IDs.
   */

  D_Assert(FALSE, "Not yet implemented.");
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     09.14.94  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void RISK_LoadObject(CString strFileName)
{
  UNUSED(strFileName);
  D_Assert(FALSE, "Not yet implemented.");
}


/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     02.12.95  ESF  Created.
 *     02.23.95  ESF  Fixed bug.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
SpeciesObject *_RISK_GetSpecies(Int32 iSpecies)
{
  SpeciesObject *ptr=NULL, *ptrLast;
  Int32          i;

  /* Thread down the linked list as if it were an array, 
   * looking for the iSpecies'th object.
   */

  for (ptrLast=NULL,ptr=&RiskGame.pSpecies, i=0; 
       ptr && i!=iSpecies; 
       ptr=ptr->next, i++)
    ptrLast = ptr; 
  
  D_Assert(i==iSpecies || i<iSpecies, "Passed over iSpecies?");

  /* Create a new species object if needed, with 
   * blank objects inbetween if needed.
   */
  
  if (ptr)
    return ptr;

  /* The current species is NULL, ptrLast points to the last valid one */
  i--;

  for (; 
       i!=iSpecies; 
       i++, ptrLast = ptr)
    {
      ptr = (SpeciesObject *)MEM_Alloc(sizeof(SpeciesObject));
      
      /* Init the object */
      ptr->strName = NULL;
      ptr->strVersion = NULL;
      ptr->strAuthor = NULL;
      ptr->strDescription = NULL;
      ptr->iClient = -1;
      ptr->iAllocationState = ALLOC_NONE;
      ptr->next = NULL;

      /* Append it to the list */
      D_Assert(ptrLast, "Something is messed up!");
      ptrLast->next = ptr;
    }

  D_Assert(ptr, "Something went very wrong, I don't have a pointer??");
  
  return ptr;
}


/************************************************************************ 
 *  FUNCTION: _RISK_GetIntValue
 *  HISTORY: 
 *     03.20.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 _RISK_GetIntValue(Int32 iField, Int32 iIndex1, Int32 iIndex2)
{
  switch(iField)
    {
    case PLR_ATTACKMODE:
      return RiskGame.pPlayers[iIndex1].iAttackMode;
      break;
    case PLR_DICEMODE:
      return RiskGame.pPlayers[iIndex1].iDiceMode;
      break;
    case PLR_MSGDSTMODE:
      return RiskGame.pPlayers[iIndex1].iMsgDstMode;
      break;
    case PLR_STATE:
      return RiskGame.pPlayers[iIndex1].iState;
      break;
    case PLR_CLIENT:
      return RiskGame.pPlayers[iIndex1].iClient;
      break;
    case PLR_SPECIES:
      return RiskGame.pPlayers[iIndex1].iSpecies;
      break;
    case PLR_ALLOCATION:
      return RiskGame.pPlayers[iIndex1].iAllocationState;
      break;
    case PLR_NUMCOUNTRIES:
      return RiskGame.pPlayers[iIndex1].iCountriesOwned;
      break;
    case PLR_NUMARMIES:
      return RiskGame.pPlayers[iIndex1].iNumArmies;
      break;
    case PLR_NUMCARDS:
      return RiskGame.pPlayers[iIndex1].iNumCards;
      break;
    case PLR_CARD:
      return RiskGame.pPlayers[iIndex1].piCards[iIndex2];
      break;
    case PLR_MISSION:
      return RiskGame.pPlayers[iIndex1].typOfMission;
      break;
    case PLR_MISSION1:
      return RiskGame.pPlayers[iIndex1].mission1;
      break;
    case PLR_MISSION2:
      return RiskGame.pPlayers[iIndex1].mission2;
      break;
    case CNT_NUMARMIES:
      return RiskGame.pCountries[iIndex1].iNumArmies;
      break;
    case CNT_OWNER:
      return RiskGame.pCountries[iIndex1].piOwner;
      break;
    case SPE_CLIENT:
      {
	SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	return s->iClient;
      }
      break;
    case SPE_ALLOCATION:
      {
	SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	return s->iAllocationState;
      }
      break;

    default:
      D_Assert(FALSE, "Add case to _RISK_GetIntValue!");
    }
  
  /* Make compiler happy */
  return 0;
}


/************************************************************************ 
 *  FUNCTION: _RISK_GetStrValue
 *  HISTORY: 
 *     03.20.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
CString _RISK_GetStrValue(Int32 iField, Int32 iIndex1, Int32 iIndex2)
{
  UNUSED(iIndex2);
  switch(iField)
    {
    case PLR_NAME:
      return RiskGame.pPlayers[iIndex1].strName;
      break;
    case PLR_COLORSTRING:
      return RiskGame.pPlayers[iIndex1].strColor;
      break;
    case SPE_NAME:
      {
	SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	return s->strName;
      }
      break;
    case SPE_VERSION:
      {
	SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	return s->strVersion;
      }
      break;
    case SPE_DESCRIPTION:
      {
	SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	return s->strDescription;
      }
      break;
    case SPE_AUTHOR:
      {
	SpeciesObject *s = _RISK_GetSpecies(iIndex1);
	return s->strAuthor;
      }
      break;

    default:
      D_Assert(FALSE, "Add case to _RISK_GetStrValue");
    }

  /* Make the compiler happy */
  return NULL;
} 
