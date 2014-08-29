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
 *   $Id: riskgame.h,v 1.7 1999/12/26 19:09:04 morphy Exp $
 *
 *   $Log: riskgame.h,v $
 *   Revision 1.7  1999/12/26 19:09:04  morphy
 *   Doxygen comments
 *
 */

/** \file
 * Interface definitions for the RISK game distributed object.
 *
 * \bug Contains a LOT of hardcoded information
 */

#ifndef _RISK
#define _RISK

#include <stdio.h>

#include "network.h"
#include "types.h"

/* Fields of the RiskGame object.  There MUST NOT be any gaps in the
 * numeric sequence (because of how the replication works.
 */

#define PLR_ATTACKMODE       0   /* Remembers the attack mode of the player */
#define PLR_MSGDSTMODE       1   /* Remembers the last message destination */
#define PLR_DICEMODE         2   /* Remembers the last die mode */

#define PLR_STATE            3   /* Either TRUE = Alive, FALSE = Dead */
#define PLR_CLIENT           4   /* The client of the player */
#define PLR_NUMCOUNTRIES     5   /* Number of territories the player owns */
#define PLR_NUMARMIES        6   /* Number of armies the player holds */
#define PLR_NUMCARDS         7   /* Number of cards the player holds */
#define PLR_NAME             8   /* Name of the player */
#define PLR_COLORSTRING      9   /* Name of the color of the player */
#define PLR_CARD            10   /* A card of the player */
#define PLR_SPECIES         11   /* What is the player */
#define PLR_ALLOCATION      12   /* State of allocation of player */
#define PLR_MISSION         13   /* Mission's type of player */
#define PLR_MISSION1        14   /* Mission's number1 of player */
#define PLR_MISSION2        15   /* Mission's number2 of player */

#define CNT_NAME            16
#define CNT_CONTINENT       17
#define CNT_NUMARMIES       18
#define CNT_OWNER           19
#define CNT_ADJCOUNTRY      20
#define CNT_TEXTX           21
#define CNT_TEXTY           22

#define CON_NAME            23
#define CON_VALUE           24
#define CON_NUMCOUNTRIES    25

#define SPE_NAME            26
#define SPE_VERSION         27
#define SPE_DESCRIPTION     28
#define SPE_AUTHOR          29
#define SPE_CLIENT          30
#define SPE_ALLOCATION      31

/* Port at which the clients can find the server */
#define RISK_PORT 5324

/* Amount of time (in ms) for graphical notification */
#define NOTIFY_TIME 750

/* Limits */
#define MAX_CLIENTS     32
#define MAX_CARDS       10
#define MAX_PLAYERS     12
#define MAX_COLORS     256

/* This may be a static value (i.e. OPEN_MAX), or a dynamic value,
 * as in OSF/1, accessed through sysconf().  I'm not sure that all 
 * systems have these, so we'll just guess.  I don't think this is
 * exceptionally evil, since if we run out of descriptors, the socket
 * or accept calls will fail.
 */


#define MAX_DESCRIPTORS 128

/* Numbers */
#define NUM_CONTINENTS     6
#define NUM_COUNTRIES     42
#define NUM_OTHERCOLORS    4 /* Dice*2, Player Turn, ColorEdit */
#define NUM_CARDS         (NUM_COUNTRIES+2)
#define NUM_FORTIFY_ARMIES 1

/* Continents */
#define CNT_NORTHAMERICA  0 
#define CNT_SOUTHAMERICA  1
#define CNT_AFRICA        2
#define CNT_AUSTRALIA     3
#define CNT_ASIA          4
#define CNT_EUROPE        5

/* Missions */
#define NO_MISSION              0
#define CONQUIER_WORLD          1
#define CONQUIER_Nb_COUNTRY     2
#define CONQUIER_TWO_CONTINENTS 3
#define KILL_A_PLAYER           4

/* Macros */
#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) ((a)>(b)?(a):(b))

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) ((a)>(b)?(b):(a))

#ifndef TRUE
#define TRUE   1
#endif
#ifndef FALSE
#define FALSE  0
#endif

/* States in risk state machine */
#define STATE_REGISTER  0
#define STATE_FORTIFY   1
#define STATE_PLACE     2
#define STATE_ATTACK    3
#define STATE_MOVE      4

/* Defines for widgets */
#define ATTACK_ONE      0
#define ATTACK_TWO      1
#define ATTACK_THREE    2
#define ATTACK_AUTO     3

#define ACTION_PLACE    0
#define ACTION_ATTACK   1
#define ACTION_DOORDIE  2
#define ACTION_MOVE     3

/* The only known species at the beginning */
#define SPECIES_HUMAN    0

/* A PERSISTANT object will maintain a copy of itself on secondary storage.  
 * Every time the object updates a field, it also updates the field on disk.
 */

#define MODE_PERSISTANT 8

/* States in allocation for players and species */
#define ALLOC_NONE       0
#define ALLOC_INPROGRESS 1
#define ALLOC_COMPLETE   2

/* States for players */
#define PLAYER_DEAD      0
#define PLAYER_ALIVE     1

/* These functions support all of the functionality of the RiskGame structure,
 * in order for it to work as a fully distributed object.  The functions
 * are split into two catagories, those that receive input from a remote 
 * source (through a message), those that receive input from the local
 * client, which is sent back to the server to broadcast to all the other
 * clients, and those that return fields of the object to the local client.
 * This file serves the functionality of a C++ object -- I would use GCC but
 * its C++ side is not stable enough.  Hold your breath, though :)
 */

/* Constructor */

void    RISK_InitObject(void (*ReplicateCallback)(Int32, void *, Int32, Int32),
			void (*PreViewCallback)(Int32, void *),
			void (*PostViewCallback)(Int32, void *),
			void (*FailureCallback)(CString, Int32),
			FILE *hDebug);

#define MESS_INCOMING 0
#define MESS_OUTGOING 1

/* For persistance */

void     RISK_SaveObject(CString strFileName);
void     RISK_LoadObject(CString strFileName);


/* Utilities */

Int32    RISK_GetNthLivePlayer(Int32 iIndex);
Int32    RISK_GetNthPlayer(Int32 iIndex);
Int32    RISK_GetNthPlayerAtClient(Int32 iClient, Int32 iIndex);
Int32    RISK_GetNumPlayersOfClient(Int32 iClient);
Int32    RISK_GetNumLivePlayersOfClient(Int32 iClient);
void     RISK_ObjectFailure(CString strReason, Int32 iCommLink);

/* For the failure routine */
#define SERVER -1


/* Reliable communication */

Int32    RISK_SendMessage(Int32 iDest, Int32 iMessType, void *pvMessage);
Int32    RISK_ReceiveMessage(Int32 iSource, Int32 *piMessType, 
			     void **ppvMessage);
Int32    RISK_SendSyncMessage(Int32 iCommLink, Int32 iMessType, 
			      void *pvMessage, Int32 iReturnMessType,
			      void (*CBK_MessageReceived)(Int32, void *)); 

/* Methods for the distributed object. */

void     RISK_SelectiveReplicate(Int32 iClientToServerSocket, Int32 iField, 
				 Int32 iIndex1, Int32 iIndex2);
void     RISK_ResetObj(void);
void     RISK_ResetGame(void);
void     RISK_ProcessMessage(Int32 iMessType, void *pMessage);

void     RISK_SetSpeciesOfPlayer(Int32 iPlayer, Int32 iSpecies);
void     RISK_SetAttackModeOfPlayer(Int32 iPlayer, Int32 iMode);
void     RISK_SetDiceModeOfPlayer(Int32 iPlayer, Int32 iMode);
void     RISK_SetMsgDstModeOfPlayer(Int32 iPlayer, Int32 iMode);
void     RISK_SetStateOfPlayer(Int32 iPlayer, Int32 iState);
void     RISK_SetClientOfPlayer(Int32 iPlayer, Int32 iClient);
void     RISK_SetAllocationStateOfPlayer(Int32 iPlayer, Int32 iState);
void     RISK_SetNumCountriesOfPlayer(Int32 iCountry, Int32 iNumCountries);
void     RISK_SetNumArmiesOfPlayer(Int32 iCountry, Int32 iNumArmies);
void     RISK_SetNumCardsOfPlayer(Int32 iPlayer, Int32 iNumCards);
void     RISK_SetNameOfPlayer(Int32 iPlayer, CString strName);
void     RISK_SetColorCStringOfPlayer(Int32 iPlayer, CString strColor);
void     RISK_SetCardOfPlayer(Int32 iPlayer, Int32 iCard, Int32 iValue);
void     RISK_SetMissionTypeOfPlayer(Int32 iPlayer, Int16 typ);
void     RISK_SetMissionNumberOfPlayer(Int32 iPlayer, Int32 n);
void     RISK_SetMissionContinent1OfPlayer(Int32 iPlayer, Int32 n);
void     RISK_SetMissionContinent2OfPlayer(Int32 iPlayer, Int32 n);
void     RISK_SetMissionMissionPlayerToKillOfPlayer(Int32 iPlayer, Int32 n);
void     RISK_SetMissionPlayerIsKilledOfPlayer(Int32 iPlayer, Flag boool);
void     RISK_SetNameOfCountry(Int32 iCountry, CString strName);
void     RISK_SetContinentOfCountry(Int32 iCountry, Int32 iContinent);
void     RISK_SetOwnerOfCountry(Int32 iCountry, Int32 iOwner);
void     RISK_SetNumArmiesOfCountry(Int32 iCountry, Int32 iNumArmies);
void     RISK_SetAdjCountryOfCountry(Int32 iCountry, Int32 iIndex, 
				     Int32 OtherC);

Int32    RISK_GetSpeciesOfPlayer(Int32 iPlayer);
Int32    RISK_GetDiceModeOfPlayer(Int32 iPlayer);
Int32    RISK_GetMsgDstModeOfPlayer(Int32 iPlayer);
Int32    RISK_GetAttackModeOfPlayer(Int32 iPlayer);
Int32    RISK_GetStateOfPlayer(Int32 iPlayer);
Int32    RISK_GetClientOfPlayer(Int32 iPlayer);
Int32    RISK_GetAllocationStateOfPlayer(Int32 iPlayer);
Int32    RISK_GetNumCountriesOfPlayer(Int32 iPlayer);
Int32    RISK_GetNumArmiesOfPlayer(Int32 iPlayer);
Int32    RISK_GetNumCardsOfPlayer(Int32 iPlayer);
CString  RISK_GetNameOfPlayer(Int32 iPlayer);
CString  RISK_GetColorCStringOfPlayer(Int32 iPlayer);
Int32    RISK_GetCardOfPlayer(Int32 iPlayer, Int32 iCard);
Int16    RISK_GetMissionTypeOfPlayer(Int32 iPlayer);
Int32    RISK_GetMissionNumberOfPlayer(Int32 iPlayer);
Int32    RISK_GetMissionContinent1OfPlayer(Int32 iPlayer);
Int32    RISK_GetMissionContinent2OfPlayer(Int32 iPlayer);
Int32    RISK_GetMissionPlayerToKillOfPlayer(Int32 iPlayer);
Flag     RISK_GetMissionIsPlayerKilledOfPlayer(Int32 iPlayer);
CString  RISK_GetNameOfCountry(Int32 iCountry);
Int32    RISK_GetContinentOfCountry(Int32 iCountry);
Int32    RISK_GetNumArmiesOfCountry(Int32 iCountry);
Int32    RISK_GetOwnerOfCountry(Int32 iCountry);
Int32    RISK_GetAdjCountryOfCountry(Int32 iCountry, Int32 iIndex);
Int32    RISK_GetTextXOfCountry(Int32 iCountry);
Int32    RISK_GetTextYOfCountry(Int32 iCountry);
Int32    RISK_GetValueOfContinent(Int32 iContinent);
CString  RISK_GetNameOfContinent(Int32 iContinent);
Int32    RISK_GetNumCountriesOfContinent(Int32 iContinent);
Int32    RISK_GetNumLivePlayers(void);
Int32    RISK_GetNumPlayers(void);

/* The first (should-be-a) composite object in Frisk */
CString  RISK_GetNameOfSpecies(Int32 iHandle);
Int32    RISK_GetClientOfSpecies(Int32 iHandle);
CString  RISK_GetAuthorOfSpecies(Int32 iHandle);
CString  RISK_GetDescriptionOfSpecies(Int32 iHandle);
CString  RISK_GetVersionOfSpecies(Int32 iHandle);
Int32    RISK_GetAllocationStateOfSpecies(Int32 iHandle);
Int32    RISK_GetNumSpecies(void); 

void     RISK_SetNameOfSpecies(Int32 iHandle, CString strName);
void     RISK_SetClientOfSpecies(Int32 iHandle, Int32 iClient);
void     RISK_SetAuthorOfSpecies(Int32 iHandle, CString strAuthor);
void     RISK_SetVersionOfSpecies(Int32 iHandle, CString strVersion);
void     RISK_SetDescriptionOfSpecies(Int32 iHandle, CString strDescription);
void     RISK_SetAllocationStateOfSpecies(Int32 iHandle, Int32 iState);
void     RISK_SetNumSpecies(Int32 iNumSpecies);

#endif
