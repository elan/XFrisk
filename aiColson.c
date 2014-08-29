/***********************************************************************
 *
 *   18-8-95  Created by Jean-Claude Colson.
 *
 *   RISK game player.
 *   $Id: aiColson.c,v 1.5 1999/11/07 15:57:29 tony Exp $
 *
 ***********************************************************************/

#include <stdlib.h>
#include <string.h>

#include "aiClient.h"
#include "client.h"
#include "debug.h"
#include "types.h"
#include "riskgame.h"
#include "game.h"
#include "utils.h"

void *COLSON_Play(void *pData, Int32 iCommand, void *pArgs);

#define AUTHOR "Jean-Claude COLSON"
/* The species */
DefineSpecies(
	      COLSON_Play,
              "Ordinateur",
	      AUTHOR,
	      "0.01",
              "Machine violente."
	      )

/************************************************************************/

/* External data */
extern Int32  iCurrentPlayer;


/************************************************************************/
static Int32 numTurn[MAX_PLAYERS];
static Int32 piContinent[MAX_PLAYERS];
static Int16 isEnemyPlayer[MAX_PLAYERS];
static Int16 levelEnemy=0;

/************************************************************************ 
 *  FUNCTION: RISK_GetTotalArmiesOfPlayer
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 RISK_GetTotalArmiesOfPlayer(Int32 iPlayer)
{
  Int32 i, nb;

  nb = 0;
  for (i=0; i<NUM_COUNTRIES; i++)
    if (RISK_GetOwnerOfCountry(i) == iPlayer)
      nb += RISK_GetNumArmiesOfCountry(i);
  return nb;
}


/************************************************************************ 
 *  FUNCTION: AddPlayer
 *  HISTORY: 
 *     01.09.95  JC  Created from PLAYER_Ok.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AddPlayer(Char *Name, Char *Color, Int32 iSpecies)
{
  extern Int32 iCommLink, iReply;
  extern void CBK_IncomingMessage(Int32 iMessType, void *pvMessage);

  /* See if there are too many players */
  (void)RISK_SendSyncMessage(iCommLink, 
			     MSG_ALLOCPLAYER, NULL,
			     MSG_REPLYPACKET, CBK_IncomingMessage);
  if (iReply == -1)
    return;

  /* Init. the player */
  RISK_SetNameOfPlayer(iReply, Name);
  RISK_SetColorCStringOfPlayer(iReply, Color);
  RISK_SetSpeciesOfPlayer(iReply, iSpecies);
  RISK_SetClientOfPlayer(iReply, CLNT_GetThisClientID());
  RISK_SetAllocationStateOfPlayer(iReply, ALLOC_COMPLETE);
}


/************************************************************************ 
 *  FUNCTION: InitClient
 *  HISTORY: 
 *     04.09.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void InitClient(Int32 iSpecies)
{
  Char  Name[20];

  snprintf(Name, sizeof(Name), "Titi-%d", iSpecies);
  AddPlayer(Name, "#FFFF00", iSpecies);
  snprintf(Name, sizeof(Name), "Toto-%d", iSpecies);
  AddPlayer(Name, "#FF0000", iSpecies);
  snprintf(Name, sizeof(Name), "Truc-%d", iSpecies);
  AddPlayer(Name, "#00FF00", iSpecies);
}


/************************************************************************ 
 *  FUNCTION: IsContinentOfMission
 *  HISTORY: 
 *     08.09.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag IsContinentOfMission(Int32 iPlayer, Int32 iContinent)
{
  if (RISK_GetMissionTypeOfPlayer(iPlayer) != CONQUIER_TWO_CONTINENTS)
      return FALSE;
  if (RISK_GetMissionContinent1OfPlayer(iPlayer) == iContinent)
      return TRUE;
  if (RISK_GetMissionContinent2OfPlayer(iPlayer) == iContinent)
      return TRUE;
  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: IsEnemyPlayer
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag IsEnemyPlayer(Int32 iPlayer)
{
  return isEnemyPlayer[iPlayer] >= levelEnemy;
}


/************************************************************************ 
 *  FUNCTION: IsFriendPlayer
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag IsFriendPlayer(Int32 iPlayer)
{
  return isEnemyPlayer[iPlayer] < levelEnemy;
}


/************************************************************************ 
 *  FUNCTION: GetNumEnemy
 *  HISTORY: 
 *     01.09.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 GetNumEnemy(Int32 iPlayer)
{
  Int32 i, nb, nbPlayers;
  UNUSED(iPlayer);

  nbPlayers = RISK_GetNumLivePlayers();
  nb = 0;
  for (i=0; i<nbPlayers; i++)
      if (isEnemyPlayer[RISK_GetNthLivePlayer(i)]>= levelEnemy)
          nb++;
  if (levelEnemy == 1)
      nb--;
  return(nb);
}


/************************************************************************ 
 *  FUNCTION: IsStrongerPlayer
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag IsStrongerPlayer(Int32 iPlayer)
{
  Int32 i, nb, nbPlayers;

  nbPlayers = RISK_GetNumLivePlayers();
  nb = RISK_GetTotalArmiesOfPlayer(iPlayer);
  nb = nb + nb/5;
  for (i=0; i<nbPlayers; i++)
    if (RISK_GetTotalArmiesOfPlayer(RISK_GetNthLivePlayer(i)) > nb)
      return FALSE;
  return TRUE;
}


/************************************************************************ 
 *  FUNCTION: IsSmallerPlayer
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag IsSmallerPlayer(Int32 iPlayer)
{
  Int32 i, nb, nbPlayers;

  nbPlayers = RISK_GetNumLivePlayers();
  nb = 3 * RISK_GetTotalArmiesOfPlayer(iPlayer);
  for (i=0; i<nbPlayers; i++)
    if (RISK_GetTotalArmiesOfPlayer(RISK_GetNthLivePlayer(i)) > nb)
      return TRUE;
  return FALSE;
}


/************************************************************************ 
 *  FUNCTION: IsContinentOfPlayer
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag IsContinentOfPlayer(Int32 iContinent, Int32 iPlayer)
{
  Int32 i, nb;

  nb = 0;
  for (i=0; i<NUM_COUNTRIES; i++)
      if (    (RISK_GetContinentOfCountry(i) == iContinent)
           && (RISK_GetOwnerOfCountry(i) == iPlayer))
          nb++;
  return (RISK_GetNumCountriesOfContinent(iContinent) == nb);
}


/************************************************************************ 
 *  FUNCTION: ComputeChoiceOfContinent
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
static void ComputeChoiceOfContinent(void)
{
  Int32 piCount     [MAX_PLAYERS][NUM_CONTINENTS];
  Flag  maxContinent[MAX_PLAYERS][NUM_CONTINENTS];
  Flag  posContinent[MAX_PLAYERS][NUM_CONTINENTS];
  Int32 continent;
  Int32 i, j, n, cont, min, max, bonus;

  for (i=0; i<MAX_PLAYERS; i++)
    {
      piContinent[i] = -1;
      for (cont=0; cont<NUM_CONTINENTS; cont++)
        {
          piCount[i][cont] = 0;
          maxContinent[i][cont] = FALSE;
          posContinent[i][cont] = FALSE;
        }
    }

  for (i=0; i<NUM_COUNTRIES; i++)
      piCount[RISK_GetOwnerOfCountry(i)][RISK_GetContinentOfCountry(i)]++;

  for (i=0; i<MAX_PLAYERS; i++)
    {
      min = 10000;
      max = 0;
      bonus = 0;
      for (cont=0; cont<NUM_CONTINENTS; cont++)
          if (piCount[i][cont] > 0)
            {
              if (min == 0)
                {
                  if (    (piCount[i][cont] == RISK_GetNumCountriesOfContinent(cont))
                       && (RISK_GetValueOfContinent(cont)>bonus))
                    {
                      for (j=0; j<NUM_CONTINENTS; j++)
                        {
                          maxContinent[i][j] = FALSE;
                          posContinent[i][j] = FALSE;
                        }
                      max = piCount[i][cont];
                      bonus = RISK_GetValueOfContinent(cont);
                      maxContinent[i][cont] = TRUE;
                      piContinent[i] = cont;
                    }
                }
              else if (piCount[i][cont] == RISK_GetNumCountriesOfContinent(cont))
                {
                  for (j=0; j<NUM_CONTINENTS; j++)
                    {
                      maxContinent[i][j] = FALSE;
                      posContinent[i][j] = FALSE;
                    }
                  min = 0;
                  max = piCount[i][cont];
                  maxContinent[i][cont] = TRUE;
                  piContinent[i] = cont;
                }
              else if (piCount[i][cont] > max)
                {
                  for (j=0; j<NUM_CONTINENTS; j++)
                    {
                      maxContinent[i][j] = FALSE;
                      if (piCount[i][j] < max-1)
                          posContinent[i][j] = (piCount[i][cont] < RISK_GetNumCountriesOfContinent(cont)/2);
                      else
                          posContinent[i][j] = TRUE;
                    }
                  min = RISK_GetNumCountriesOfContinent(cont) - piCount[i][cont];
                  max = piCount[i][cont];
                  bonus = RISK_GetValueOfContinent(cont);
                  maxContinent[i][cont] = TRUE;
                }
              else if (   (piCount[i][cont] == max)
                       &&  IsContinentOfMission(i, cont))
                {
                  min = RISK_GetNumCountriesOfContinent(cont) - piCount[i][cont];
                  bonus = 2 * RISK_GetValueOfContinent(cont);
                  maxContinent[i][cont] = TRUE;
                }
              else if (   (piCount[i][cont] == max)
                       && (RISK_GetValueOfContinent(cont) > bonus))
                {
                  min = RISK_GetNumCountriesOfContinent(cont) - piCount[i][cont];
                  bonus = RISK_GetValueOfContinent(cont);
                  maxContinent[i][cont] = TRUE;
                }
              else if (piCount[i][cont] >= RISK_GetNumCountriesOfContinent(cont)/2)
                {
                  posContinent[i][j] = TRUE;
                }
            }
    }

  for (n=0; n<=MAX_PLAYERS; n++)
    {
      /* Search a continent with no conflict */
      for (j=0; j<MAX_PLAYERS; j++)
        {
          continent = piContinent[j];
          if (continent == -1)
            {
              cont = 0;
              while ((cont<NUM_CONTINENTS) && (continent==-1))
                {
                  if (maxContinent[j][cont])
                    {
                      continent = cont;
                      i = 0;
                      while (i<MAX_PLAYERS)
                        {
                            if (i != j) {
                              if ( piContinent[i] == cont)
                                  continent = -1;
                              else if (    (maxContinent[i][cont])
                                        && (piCount[i][cont] > piCount[j][cont]))
                                  continent = -1;
                            }
                          i++;
                        }
                    }
                  cont++;
                }

              if (IsFriendPlayer(j) && (continent!=-1))
                {
                  for (cont=0; cont!=NUM_CONTINENTS; cont++)
                      if (cont != continent)
                        {
                          maxContinent[j][cont] = FALSE;
                          posContinent[j][cont] = FALSE;
                        }
                  for (i=0; i!=MAX_PLAYERS; i++)
                      posContinent[i][continent] = FALSE;
                  piContinent[j] = continent;
                }
            }
        }

      /* Search a continent with no computer conflict */
      for (j=0; j<MAX_PLAYERS; j++)
        {
          continent = piContinent[j];
          if (IsFriendPlayer(j) && (continent == -1))
            {
              cont = 0;
              while ((cont<NUM_CONTINENTS) && (continent==-1))
                {
                  if (maxContinent[j][cont])
                    {
                      continent = cont;
                      i = 0;
                      while (i<MAX_PLAYERS)
                        {
                            if (IsFriendPlayer(i) && (i != j)) {
                              if ( piContinent[i] == cont)
                                  continent = -1;
                              else if (    (maxContinent[i][cont])
                                        && (piCount[i][cont] > piCount[j][cont]))
                                  continent = -1;
                            }
                          i++;
                        }
                    }
                  cont++;
                }

              if (continent!=-1)
                {
                  for (cont=0; cont!=NUM_CONTINENTS; cont++)
                      if (cont != continent)
                        {
                          maxContinent[j][cont] = FALSE;
                          posContinent[j][cont] = FALSE;
                        }
                  for (i=0; i!=MAX_PLAYERS; i++)
                      posContinent[i][continent] = FALSE;
                  piContinent[j] = continent;
                }
            }
        }

      /* Search a possible continent with no conflict */
      for (j=0; j<MAX_PLAYERS; j++)
        {
          continent = piContinent[j];
          if (IsFriendPlayer(j) && (continent == -1))
            {
              cont = 0;
              while ((cont<NUM_CONTINENTS) && (continent==-1))
                {
                  if (posContinent[j][cont])
                    {
                      continent = cont;
                      i = 0;
                      while (i<MAX_PLAYERS)
                        {
                            if (IsFriendPlayer(i) && (i != j)) {
                              if ( piContinent[i] == cont)
                                  continent = -1;
                              else if (    (maxContinent[i][cont])
                                        && (piCount[i][cont] > piCount[j][cont]))
                                  continent = -1;
                            }
                          i++;
                        }
                    }
                  cont++;
                }

              if (continent!=-1)
                {
                  for (cont=0; cont!=NUM_CONTINENTS; cont++)
                      if (cont != continent)
                        {
                          maxContinent[j][cont] = FALSE;
                          posContinent[j][cont] = FALSE;
                        }
                  for (i=0; i!=MAX_PLAYERS; i++)
                      posContinent[i][continent] = FALSE;
                  piContinent[j] = continent;
                }
            }
        }

      /* Search a possible continent with no computer conflict */
      for (j=0; j<MAX_PLAYERS; j++)
        {
          continent = piContinent[j];
          if (IsFriendPlayer(j) && (continent == -1))
            {
              cont = 0;
              while ((cont<NUM_CONTINENTS) && (continent==-1))
                {
                  if (posContinent[j][cont])
                    {
                      continent = cont;
                      i = 0;
                      while (i<MAX_PLAYERS)
                        {
                            if (IsFriendPlayer(i) && (i != j)) {
                              if ( piContinent[i] == cont)
                                  continent = -1;
                              else if (    (maxContinent[i][cont])
                                        && (piCount[i][cont] > piCount[j][cont]))
                                  continent = -1;
                            }
                          i++;
                        }
                    }
                  cont++;
                }

              if (continent!=-1)
                {
                  for (cont=0; cont!=NUM_CONTINENTS; cont++)
                      if (cont != continent)
                        {
                          maxContinent[j][cont] = FALSE;
                          posContinent[j][cont] = FALSE;
                        }
                  for (i=0; i!=MAX_PLAYERS; i++)
                      posContinent[i][continent] = FALSE;
                  piContinent[j] = continent;
                }
            }
        }
    }

  for (j=0; j<MAX_PLAYERS; j++)
    {
      continent = piContinent[j];

      if (IsFriendPlayer(j) && (continent == -1))
        {
          /* Search a continent */
          cont = 0;
          while ((cont<NUM_CONTINENTS) && (continent==-1))
            {
              if (piCount[j][cont]>0)
                {
                  continent = cont;
                  i = 0;
                  while (i<MAX_PLAYERS)
                    {
                      if (i != j)
                          if (IsFriendPlayer(i) && (i != j))
                              continent = -1;
                      i++;
                    }
                }
              cont++;
            }

          if (continent!=-1)
            {
              for (cont=0; cont!=NUM_CONTINENTS; cont++)
                  if (cont!=continent)
                    {
                      maxContinent[j][cont] = FALSE;
                      posContinent[j][cont] = FALSE;
                    }
              for (i=0; i!=MAX_PLAYERS; i++)
                  posContinent[i][continent] = FALSE;
              piContinent[j] = continent;
            }
        }
    }

  for (j=0; j<MAX_PLAYERS; j++)
    {
      continent = piContinent[j];

      /* Search a continent */
      cont = 0;
      while ((cont<NUM_CONTINENTS) && (continent==-1))
        {
          if (maxContinent[j][cont])
              continent = cont;
          cont++;
        }

      if (continent!=-1)
        {
          for (cont=0; cont!=NUM_CONTINENTS; cont++)
              if (cont!=continent)
                {
                  maxContinent[j][cont] = FALSE;
                  posContinent[j][cont] = FALSE;
                }
          for (i=0; i!=MAX_PLAYERS; i++)
              posContinent[i][continent] = FALSE;
          piContinent[j] = continent;
        }
    }
}


/************************************************************************ 
 *  FUNCTION: InitGame
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void InitGame(Int32 iSpecies)
{
  Int32 i, iPlayer, iSpe, nb;

  for (i=0; i<MAX_PLAYERS; i++)
      isEnemyPlayer[i] = 0;
  nb = RISK_GetNumLivePlayers();
  for (i=0; i<nb; i++)
    {
      iPlayer = RISK_GetNthLivePlayer(i);
      piContinent[iPlayer] = -1;
      numTurn[iPlayer] = 0;
      iSpe = RISK_GetSpeciesOfPlayer(iPlayer);
      if (iSpe == iSpecies)
          isEnemyPlayer[iPlayer] = 1;
      else if (strcmp(RISK_GetAuthorOfSpecies(iSpe), AUTHOR) == 0)
          isEnemyPlayer[iPlayer] = 2;
      else if (iSpe != SPECIES_HUMAN)
          isEnemyPlayer[iPlayer] = 3;
      else
          isEnemyPlayer[iPlayer] = 4;
    }
  levelEnemy = 3;
  ComputeChoiceOfContinent();
}


/************************************************************************ 
 *  FUNCTION: GetContinentToFortify
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 GetContinentToFortify(Int32 iPlayer, Int32 *attack)
{
  Int32 i, continent;

  continent = piContinent[iPlayer];
  *attack = 0;
  for (i=0; i!=NUM_COUNTRIES; i++)
      if (    (RISK_GetOwnerOfCountry(i) == iPlayer)
           && (RISK_GetContinentOfCountry(i) == continent)
           &&  GAME_IsEnemyAdjacent(i))
	      (*attack)++;
  return(continent);
}


/************************************************************************ 
 *  FUNCTION: GetNumOfEnemy
 *  HISTORY: 
 *     31.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 GetNumOfEnemy(Int32 iPlayer)
{
  Int32 i, nb, nbPlayers;

  nbPlayers = RISK_GetNumLivePlayers();
  nb = 0;
  for (i=0; i<nbPlayers; i++)
    {
      iPlayer = RISK_GetNthLivePlayer(i);
      if (isEnemyPlayer[iPlayer]>= levelEnemy)
          nb++;
    }
  return nb;
}


/************************************************************************ 
 *  FUNCTION: GetContinentToConquier
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 GetContinentToConquier(Int32 iPlayer, Int32 *attack)
{
  Int32  piCount[NUM_CONTINENTS];
  Int32  piOppo[NUM_CONTINENTS];
  Int32  piAttac[NUM_CONTINENTS];
  Int32  i, min, max, bonus, continent;

  /* Init. */
  for (i=0; i!=NUM_CONTINENTS; i++)
    {
       piCount[i] = 0;
       piOppo[i] = 0;
       piAttac[i] = 0;
    }

  /* Count up how many countries the player has in each of the continents */
  for (i=0; i!=NUM_COUNTRIES; i++)
    {
      if (RISK_GetOwnerOfCountry(i) == iPlayer)
        {
	  piCount[RISK_GetContinentOfCountry(i)]++;
          piOppo[RISK_GetContinentOfCountry(i)] -= RISK_GetNumArmiesOfCountry (i);
          if (GAME_IsEnemyAdjacent(i))
	      piAttac[RISK_GetContinentOfCountry(i)]++;
	}
      else
        {
          if (IsEnemyPlayer(RISK_GetOwnerOfCountry(i)))
              piOppo[RISK_GetContinentOfCountry(i)] += RISK_GetNumArmiesOfCountry (i);
          else
              piOppo[RISK_GetContinentOfCountry(i)] += 2 * RISK_GetNumArmiesOfCountry (i);
        }
    }

  continent = -1;
  min = 10000;
  max = 0;
  bonus = 0;
  for (i=0; i!=NUM_CONTINENTS; i++)
    {
      if (IsContinentOfMission(iPlayer, i))
          piOppo[i] -= piOppo[i]/3;
      if (piCount[i] < RISK_GetNumCountriesOfContinent(i))
          if (piOppo[i] < min)
            {
               continent = i;
               min = piOppo[i];
               max = piCount[i];
               bonus = RISK_GetValueOfContinent(i);
            }
    }

  *attack = piAttac[continent];
  return(continent);
}


/************************************************************************ 
 *  FUNCTION: NbEnenyAdjacent
 *  HISTORY: 
 *     28.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 NbEnenyAdjacent(Int32 iCountry)
{
  Int32 i, iPlayer, max, nb, destCountry, iEnemy;
  Int32 NumEnemy[MAX_PLAYERS];
  Flag  fIsEnemy[MAX_PLAYERS];
  Flag  fIAmStrong;

  iPlayer = RISK_GetOwnerOfCountry(iCountry);
  fIAmStrong = IsStrongerPlayer(iPlayer);
  for (i=0; i<MAX_PLAYERS; i++)
    fIsEnemy[i] = FALSE;
  i = 0;
  while ((i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
    {
      destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (iEnemy != iPlayer)
        {
          if (NumEnemy[iEnemy] < RISK_GetNumArmiesOfCountry(destCountry))
            NumEnemy[iEnemy] = RISK_GetNumArmiesOfCountry(destCountry);
          fIsEnemy[iEnemy] = TRUE;
        }
      i++;
    }
  max = 0;
  for (i=0; i<MAX_PLAYERS; i++)
    {
      if ((i != iPlayer) && fIsEnemy[i])
        {
          nb = NumEnemy[iEnemy];
          if (!fIAmStrong && IsStrongerPlayer(i))
              nb = nb + 10;
          if (IsEnemyPlayer(i))
              nb = nb + 5;
          if (nb > max)
              max = nb;
        }
    }
  return max;
}


/************************************************************************ 
 *  FUNCTION: NbToEqualEnenyAdjacent
 *  HISTORY: 
 *     07.09.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Int32 NbToEqualEnenyAdjacent(Int32 iCountry)
{
  Int32 i, iPlayer, nb, nbe, nbi, destCountry, iEnemy;

  iPlayer = RISK_GetOwnerOfCountry(iCountry);
  nbe = 0;
  nb = 0;
  i = 0;
  while ((i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
    {
      destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (iEnemy != iPlayer)
        {
          nbe -= RISK_GetNumArmiesOfCountry(destCountry);
          nb++;
        }
      i++;
    }
  nbi = RISK_GetNumArmiesOfCountry(iCountry);
  nbe = (nbe + nbi)/nb;
  if ((nbi - nbe)< 10)
      nbe = nbi - 10;
  return nbe;
}


/************************************************************************/

static Int32 Attack_SrcCountry = -1;
static Int32 Attack_DestCountry = -1;

/************************************************************************ 
 *  FUNCTION: ComputerAttack
 *  HISTORY: 
 *     09.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
Flag ComputerAttack(Int32 iPlayer, Int32 destCountry, Flag die, Int32 dif, Int32 iMove)
{
  Int32 srcCountry, iCountry, i, nb, max;

  srcCountry = -1;
  max = RISK_GetNumArmiesOfCountry(destCountry) + dif;
  nb = 0;
  i = 0;
  while ((i < 6) && (RISK_GetAdjCountryOfCountry(destCountry, i) != -1))
    {
      iCountry = RISK_GetAdjCountryOfCountry(destCountry, i);
      if (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
        {
          if (RISK_GetNumArmiesOfCountry(iCountry) > max)
            {
              max = RISK_GetNumArmiesOfCountry(iCountry);
              srcCountry = iCountry;
            }
          nb = nb + RISK_GetNumArmiesOfCountry(iCountry);
        }
      i++;
    }
  if (srcCountry == -1)
      return FALSE;

  nb = nb - RISK_GetNumArmiesOfCountry(srcCountry);
  do
    {
      Attack_SrcCountry = srcCountry;
      Attack_DestCountry = destCountry;
      AI_Attack (srcCountry, destCountry, ATTACK_DOORDIE, DICE_MAXIMUM, iMove);
    }
  while (   (RISK_GetOwnerOfCountry(destCountry) != iPlayer)
          && (RISK_GetNumArmiesOfCountry(srcCountry) > 1)
          && (die || (   RISK_GetNumArmiesOfCountry(srcCountry)
                       > RISK_GetNumArmiesOfCountry(destCountry))));
  if (RISK_GetOwnerOfCountry(destCountry) == iPlayer)
      return (TRUE);

  nb = nb + RISK_GetNumArmiesOfCountry(srcCountry);
  if (!die && (nb <= (RISK_GetNumArmiesOfCountry(destCountry) + 20)))
      return FALSE;

  i = 0;
  while ((i < 6) && (RISK_GetAdjCountryOfCountry(destCountry, i) != -1))
    {
      iCountry = RISK_GetAdjCountryOfCountry(destCountry, i);
      if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
           && (RISK_GetNumArmiesOfCountry(iCountry) > 1))
        {
          do
            {
              Attack_SrcCountry = iCountry;
              Attack_DestCountry = destCountry;
              AI_Attack (iCountry, destCountry, ATTACK_DOORDIE, DICE_MAXIMUM, iMove);
            }
          while (   (RISK_GetOwnerOfCountry(destCountry) != iPlayer)
                  && (RISK_GetNumArmiesOfCountry(iCountry) > 1)
                  && (die || (   RISK_GetNumArmiesOfCountry(iCountry)
                               > RISK_GetNumArmiesOfCountry(destCountry))));
          if (RISK_GetOwnerOfCountry(destCountry) == iPlayer)
              return (TRUE);
        }
      i++;
    }
  return (FALSE);
}


/************************************************************************ 
 *  FUNCTION: Fortify
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void Fortify(Int32 iPlayer)
{
  Int32 iContinent, iCountry, nb;
  Flag  boool;

  iContinent = GetContinentToFortify(iPlayer, &nb);
  boool = FALSE;
  iCountry = 0;
  if (nb > 0)
    {
      nb = rand() % nb;
      while (!boool && (iCountry < NUM_COUNTRIES))
        {
          if ((RISK_GetOwnerOfCountry(iCountry) == iPlayer) &&
              (RISK_GetContinentOfCountry(iCountry) == iContinent) &&
              GAME_IsEnemyAdjacent(iCountry))
            {
              if (nb <= 0)
                {
                  AI_Place (iCountry, 1);
                  boool = TRUE;
                }
              nb--;
            }
          if (!boool)
              iCountry++;
        }
    }
  iCountry = 0;
  while (!boool && (iCountry < NUM_COUNTRIES))
    {
      if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
           &&  GAME_IsEnemyAdjacent(iCountry))
        {
          AI_Place (iCountry, 1);
          boool = TRUE;
        }
      else
          iCountry++;
    }
  iCountry = 0;
  while (!boool && (iCountry < NUM_COUNTRIES))
    {
      if (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
        {
          AI_Place (iCountry, 1);
          boool = TRUE;
        }
      else
          iCountry++;
    }
}


/************************************************************************ 
 *  FUNCTION: Place
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void Place(Int32 iPlayer)
{
  Int32 iContinent, iCountry, iEnemy;
  Int32 destCountry, i, j, nb, min;
  Flag  boool;

  iContinent = GetContinentToFortify(iPlayer, &nb);
  boool = FALSE;

  /* Try to destroy a enemy player */
  iCountry = 0;
  while ( !boool && (iCountry < NUM_COUNTRIES))
    {
      if ((RISK_GetOwnerOfCountry(iCountry) == iPlayer) &&
           GAME_IsEnemyAdjacent(iCountry))
        {
          i = 0;
          while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
            {
              destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
              iEnemy = RISK_GetOwnerOfCountry(destCountry);
              if (    (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy)
                   && (RISK_GetTotalArmiesOfPlayer(iEnemy)
                       < (   RISK_GetNumArmiesOfPlayer(iPlayer)
                           + RISK_GetNumArmiesOfCountry(iCountry) - 5)))
                {
                  AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
                  boool = TRUE;
                }
              else
                  i++;
            }
        }
      if (!boool)
          iCountry++;
    }

  /* Try to conquier an entire continent, attack enemy */
  destCountry = 0;
  while ( !boool && (destCountry < NUM_COUNTRIES))
    {
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (    (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy)
           && (RISK_GetContinentOfCountry(destCountry) == iContinent))
        {
          i = 0;
          while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(destCountry, i) != -1))
            {
              j = RISK_GetAdjCountryOfCountry(destCountry, i);
              if (RISK_GetOwnerOfCountry(j) == iPlayer)
                {
                  iCountry = j;
                  boool = TRUE;
                }
              i++;
            }
          if (boool)
            AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
        }
      destCountry++;
    }

  /* Try to destroy a player */
  iCountry = 0;
  while ( !boool && (iCountry < NUM_COUNTRIES))
    {
      if ((RISK_GetOwnerOfCountry(iCountry) == iPlayer) &&
           GAME_IsEnemyAdjacent(iCountry))
        {
          i = 0;
          while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
            {
              destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
              iEnemy = RISK_GetOwnerOfCountry(destCountry);
              if (    (iEnemy != iPlayer)
                   && (RISK_GetTotalArmiesOfPlayer(RISK_GetOwnerOfCountry(destCountry))
                       < (   RISK_GetNumArmiesOfPlayer(iPlayer)
                           + RISK_GetNumArmiesOfCountry(iCountry) - 5)))
                {
                  AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
                  boool = TRUE;
                }
              else
                  i++;
            }
        }
      if (!boool)
          iCountry++;
    }

  /* Try to conquier an entire continent */
  destCountry = 0;
  while ( !boool && (destCountry < NUM_COUNTRIES))
    {
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (    (RISK_GetContinentOfCountry(destCountry) == iContinent)
           && (iEnemy != iPlayer))
        {
          i = 0;
          while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(destCountry, i) != -1))
            {
              j = RISK_GetAdjCountryOfCountry(destCountry, i);
              if (RISK_GetOwnerOfCountry(j) == iPlayer)
                {
                  iCountry = j;
                  boool = TRUE;
                }
              i++;
            }
          if (boool)
            AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
        }
      destCountry++;
    }

  /* Try to defend an entire continent */
  if (!boool && (nb > 0))
    {
      boool = TRUE;
      while (boool && (RISK_GetNumArmiesOfPlayer(iPlayer) > 0))
        {
          iCountry = 0;
          min = 0;
          boool = FALSE;
          while (iCountry < NUM_COUNTRIES)
            {
              if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
                   && (RISK_GetContinentOfCountry(iCountry) == iContinent)
                   &&  GAME_IsEnemyAdjacent(iCountry))
                {
                  nb = NbToEqualEnenyAdjacent(iCountry);
                  if (nb < min)
                    {
                      min = nb ;
                      destCountry = iCountry;
                      boool = TRUE;
                    }
                }
              iCountry++;
            }
          if (boool)
              AI_Place (destCountry, 1);
        }
      boool = (RISK_GetNumArmiesOfPlayer(iPlayer) <= 0);
    }

  iContinent = GetContinentToConquier(iPlayer, &nb);
  /* Try to conquier an entire continent, attack enemy */
  iCountry = 0;
  while ( !boool && (iCountry < NUM_COUNTRIES))
    {
      if ((RISK_GetOwnerOfCountry(iCountry) == iPlayer) &&
          (RISK_GetContinentOfCountry(iCountry) == iContinent) &&
           GAME_IsEnemyAdjacent(iCountry))
        {
          i = 0;
          while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
            {
              destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
              iEnemy = RISK_GetOwnerOfCountry(destCountry);
              if (    (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy)
                   && (RISK_GetContinentOfCountry(destCountry) == iContinent))
                {
                  AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
                  boool = TRUE;
                }
              else
                  i++;
            }
        }
      if (!boool)
          iCountry++;
    }

  /* Try to conquier an entire continent */
  iCountry = 0;
  while ( !boool && (iCountry < NUM_COUNTRIES))
    {
      if ((RISK_GetOwnerOfCountry(iCountry) == iPlayer) &&
          (RISK_GetContinentOfCountry(iCountry) == iContinent) &&
           GAME_IsEnemyAdjacent(iCountry))
        {
          i = 0;
          while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
            {
              destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
              iEnemy = RISK_GetOwnerOfCountry(destCountry);
              if (    (iEnemy != iPlayer)
                   && (RISK_GetContinentOfCountry(destCountry) == iContinent))
                {
                  AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
                  boool = TRUE;
                }
              else
                  i++;
            }
        }
      if (!boool)
          iCountry++;
    }

  /* Try to defend an entire continent */
  if (!boool && (nb > 0))
    {
      iCountry = 0;
      min = 1000;
      boool = FALSE;
      while (iCountry < NUM_COUNTRIES)
        {
          if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
               && (RISK_GetContinentOfCountry(iCountry) == iContinent)
               &&  GAME_IsEnemyAdjacent(iCountry))
            {
              nb = NbToEqualEnenyAdjacent(iCountry);
              if (nb < min)
                {
                  min = RISK_GetNumArmiesOfCountry(iCountry);
                  destCountry = iCountry;
                  boool = TRUE;
                }
            }
          iCountry++;
        }
      if (boool)
          AI_Place (destCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
    }

  /* Try to prepare an enemy attack, find a lowest defence */
  if (!boool)
    {
      iCountry = 0;
      while (!boool && (iCountry < NUM_COUNTRIES))
        {
          if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
               &&  GAME_IsEnemyAdjacent(iCountry))
            {
              boool = FALSE;
              i = 0;
              while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
                {
                  destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
                  iEnemy = RISK_GetOwnerOfCountry(destCountry);
                  if (    (RISK_GetContinentOfCountry(destCountry) == iContinent)
                       && (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy)
                       && (     (   RISK_GetNumArmiesOfCountry(iCountry)
                                  > RISK_GetNumArmiesOfCountry(destCountry)+3)
                             || (RISK_GetNumArmiesOfCountry(destCountry)==1)))
                      boool = TRUE;
                  if (!boool)
                      i++;
                }
              if (boool)
                  AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
            }
          if (!boool)
              iCountry++;
        }
    }

  /* Try to prepare an enemy attack */
  if (!boool)
    {
      iCountry = 0;
      while (!boool && (iCountry < NUM_COUNTRIES))
        {
          if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
               &&  GAME_IsEnemyAdjacent(iCountry))
            {
              boool = FALSE;
              i = 0;
              while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
                {
                  destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
                  iEnemy = RISK_GetOwnerOfCountry(destCountry);
                  if ((iEnemy != iPlayer) && IsEnemyPlayer(iEnemy))
                      boool = TRUE;
                  if (!boool)
                      i++;
                }
              if (boool)
                  AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
            }
          if (!boool)
              iCountry++;
        }
    }

  /* Try to prepare an attack */
  if (!boool)
    {
      iCountry = 0;
      while (!boool && (iCountry < NUM_COUNTRIES))
        {
          if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
               &&  GAME_IsEnemyAdjacent(iCountry))
            {
              AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
              boool = TRUE;
            }
          else
              iCountry++;
        }
    }

  /* Try to place */
  if (!boool)
    {
      iCountry = 0;
      while ( !boool && (iCountry < NUM_COUNTRIES))
        {
          if (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
            {
              AI_Place (iCountry, RISK_GetNumArmiesOfPlayer(iPlayer));
              boool = TRUE;
            }
          else
              iCountry++;
        }
    }
}


/************************************************************************ 
 *  FUNCTION: AttacEnemy
 *  HISTORY: 
 *     01.09.95  JC  Created from Attack.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void AttacEnemy(Int32 iPlayer)
{
  Int32 iContinent, destCountry, iEnemy, nb;

  iContinent = GetContinentToFortify(iPlayer, &nb);

  /* Try to conquier an entire continent, attack player of other species */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (    (RISK_GetContinentOfCountry(destCountry) == iContinent)
           && (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy))
        {
          if (ComputerAttack (iPlayer, destCountry, TRUE,
                                   (RISK_GetNumArmiesOfCountry(destCountry) < 5)?1:
                                       ((nb > 4)?RISK_GetNumArmiesOfCountry(destCountry):3),
                                   ARMIES_MOVE_MANUAL))
            {
              destCountry = 0;
              nb++;
            }
          else
              destCountry++;
        }
      else
          destCountry++;
    }

  /* Try to conquier an entire continent */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      if (    (RISK_GetContinentOfCountry(destCountry) == iContinent)
           && (RISK_GetOwnerOfCountry(destCountry) != iPlayer))
        {
          if (ComputerAttack (iPlayer, destCountry, TRUE,
                              (RISK_GetNumArmiesOfCountry(destCountry) < 3)?1:50,
                               ARMIES_MOVE_MANUAL))
            {
              destCountry = 0;
              nb++;
            }
          else
              destCountry++;
        }
      else
          destCountry++;
    }

  if (    !IsContinentOfPlayer(iContinent, iPlayer)
       && (numTurn[iPlayer] <= 2)
       && (RISK_GetNumLivePlayers() > NUM_CONTINENTS/2))
      return;

  iContinent = GetContinentToConquier(iPlayer, &nb);

  nb = 0;
  /* Try to destroy a human player */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      if (    (RISK_GetOwnerOfCountry(destCountry) != iPlayer)
           && (RISK_GetSpeciesOfPlayer(RISK_GetOwnerOfCountry(destCountry))
               == SPECIES_HUMAN)
           && (RISK_GetNumCountriesOfPlayer(RISK_GetOwnerOfCountry(destCountry)) == 1))
          if (ComputerAttack (iPlayer, destCountry, TRUE,
                                   (nb > 2)?10:2, ARMIES_MOVE_MANUAL))
              nb++;
      destCountry++;
    }

  /* Try to destroy a enemy player */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      if (   IsEnemyPlayer(RISK_GetOwnerOfCountry(destCountry))
           && (RISK_GetNumCountriesOfPlayer(RISK_GetOwnerOfCountry(destCountry)) == 1))
          if (ComputerAttack (iPlayer, destCountry, TRUE,
                                   (nb > 2)?20:2, ARMIES_MOVE_MANUAL))
              nb++;
      destCountry++;
    }

  /* Try to destroy a player */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      if (    (RISK_GetOwnerOfCountry(destCountry) != iPlayer)
           && (RISK_GetNumCountriesOfPlayer(RISK_GetOwnerOfCountry(destCountry)) == 1))
          if (ComputerAttack (iPlayer, destCountry, TRUE,
                                   (nb > 2)?20:2, ARMIES_MOVE_MANUAL))
              nb++;
      destCountry++;
    }

  /* Try to conquier an entire continent, attack player of other species */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      if (    (RISK_GetContinentOfCountry(destCountry) == iContinent)
           && (RISK_GetOwnerOfCountry(destCountry) != iPlayer)
           &&  IsEnemyPlayer(RISK_GetOwnerOfCountry(destCountry)))
        {
          if (ComputerAttack (iPlayer, destCountry, TRUE,
                                   (RISK_GetNumArmiesOfCountry(destCountry) < 5)?1:
                                       ((nb > 4)?RISK_GetNumArmiesOfCountry(destCountry):3),
                                   ARMIES_MOVE_MANUAL))
            {
              destCountry = 0;
              nb++;
            }
          else
              destCountry++;
        }
      else
          destCountry++;
    }

  /* Try to conquier an entire continent */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      if (    (RISK_GetContinentOfCountry(destCountry) == iContinent)
           && (RISK_GetOwnerOfCountry(destCountry) != iPlayer))
        {
          if (ComputerAttack (iPlayer, destCountry, TRUE,
                              (RISK_GetNumArmiesOfCountry(destCountry) < 3)?1:50,
                               ARMIES_MOVE_MANUAL))
            {
              destCountry = 0;
              nb++;
            }
          else
              destCountry++;
        }
      else
          destCountry++;
    }

  /* Try to attack a stronger human player for a card */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (    (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy)
           && (RISK_GetSpeciesOfPlayer(iEnemy) == SPECIES_HUMAN)
           && IsStrongerPlayer(RISK_GetOwnerOfCountry(destCountry)))
          if (ComputerAttack (iPlayer, destCountry, FALSE,
                              (RISK_GetNumArmiesOfCountry(destCountry) < 3)?1:((nb > 2)?10:2),
                              ARMIES_MOVE_MANUAL))
              nb++;
      destCountry++;
    }

  /* Try to attack an human player for a card */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (    (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy)
           && (RISK_GetSpeciesOfPlayer(iEnemy) == SPECIES_HUMAN))
          if (ComputerAttack (iPlayer, destCountry, FALSE,
                              (RISK_GetNumArmiesOfCountry(destCountry) < 3)?1:((nb > 2)?10:2),
                              ARMIES_MOVE_MANUAL))
              nb++;
      destCountry++;
    }

  /* Try to attack enemy player for a card */
  destCountry = 0;
  while (destCountry < NUM_COUNTRIES)
    {
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (    (iEnemy != iPlayer) && IsEnemyPlayer(iEnemy))
          if (ComputerAttack (iPlayer, destCountry, FALSE,
                              (RISK_GetNumArmiesOfCountry(destCountry) < 3)?1:((nb > 2)?10:2),
                              ARMIES_MOVE_MANUAL))
              nb++;
      destCountry++;
    }

  /* Try to attack for a card, attack a stronger player */
  destCountry = 0;
  while ((nb < 1) && (destCountry < NUM_COUNTRIES))
    {
      iEnemy = RISK_GetOwnerOfCountry(destCountry);
      if (    (iEnemy != iPlayer)
           && IsStrongerPlayer(iEnemy))
          if (ComputerAttack (iPlayer, destCountry, FALSE,
                              (RISK_GetNumArmiesOfCountry(destCountry) < 2)?1:100,
                              ARMIES_MOVE_MANUAL))
                  nb++;
      destCountry++;
    }

  /* Try to attack for a card */
  destCountry = 0;
  while ((nb < 1) && (destCountry < NUM_COUNTRIES))
    {
      if (RISK_GetOwnerOfCountry(destCountry) != iPlayer)
          if (ComputerAttack (iPlayer, destCountry, FALSE,
                              (RISK_GetNumArmiesOfCountry(destCountry) < 2)?1:100,
                              ARMIES_MOVE_MIN))
                  nb++;
      destCountry++;
    }
}


/************************************************************************ 
 *  FUNCTION: Attack
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void Attack(Int32 iPlayer)
{
  Int32 enemyAlive;

  if (RISK_GetAttackModeOfPlayer (iPlayer) != ACTION_DOORDIE)
      RISK_SetAttackModeOfPlayer (iPlayer, ACTION_DOORDIE);
  if (RISK_GetDiceModeOfPlayer (iPlayer) != ATTACK_AUTO)
      RISK_SetDiceModeOfPlayer (iPlayer, ATTACK_AUTO);

  enemyAlive = GetNumEnemy(iPlayer);
  while ((enemyAlive == 0) && (levelEnemy >0))
    {
      levelEnemy--;
      enemyAlive = GetNumEnemy(iPlayer);
    }

  AttacEnemy(iPlayer);
}


/************************************************************************ 
 *  FUNCTION: HowManyArmiesToMove
 *  HISTORY: 
 *     23.08.95  JC  Created.
 *     30.08.95  JC  Place the number of armies to move in nb.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void HowManyArmiesToMove(Int32 iPlayer, Int32 *nb)
{
  UNUSED(iPlayer);
  if ((Attack_SrcCountry == -1) || (Attack_DestCountry == -1))
      return;

  if (!GAME_IsEnemyAdjacent(Attack_SrcCountry))
      *nb = 0;
  else if (!GAME_IsEnemyAdjacent(Attack_DestCountry))
      *nb = *nb;
  else
      *nb = *nb/2;
  Attack_SrcCountry  = -1;
  Attack_DestCountry = -1;
}


/************************************************************************ 
 *  FUNCTION: Move
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void Move(Int32 iPlayer)
{
  Int32 iCountry, destCountry, i, max;
  Flag  boool;

  /* Try to move an unused max army in a frontier */
  boool = FALSE;
  i = 0;
  max = 1;
  while (i < NUM_COUNTRIES)
    {
      if (    (RISK_GetOwnerOfCountry(i) == iPlayer)
           && (RISK_GetNumArmiesOfCountry(i) > max)
           && !GAME_IsEnemyAdjacent(i))
        {
          max = RISK_GetNumArmiesOfCountry(i);
          iCountry = i;
          boool = TRUE;
        }
      i++;
    }
  if (boool)
    {
      destCountry = GAME_FindEnemyAdjacent(iCountry);
      if (destCountry >= 0)
          AI_Move (iCountry, destCountry,
                   RISK_GetNumArmiesOfCountry(iCountry)-1);
      else
          boool = FALSE;
    }

  /* Try to move an unused army in a country witch have a frontier */
  iCountry = 0;
  while (!boool && (iCountry < NUM_COUNTRIES))
    {
      if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
           && (RISK_GetNumArmiesOfCountry(iCountry) > 2)
           && !GAME_IsEnemyAdjacent(iCountry))
        {
          destCountry = GAME_FindEnemyAdjacent(iCountry);
          if (destCountry >= 0)
            {
              AI_Move (iCountry, destCountry,
                        RISK_GetNumArmiesOfCountry(iCountry)-1);
              boool = TRUE;
            }
        }
      iCountry++;
    }

  iCountry = 0;
  while (!boool && (iCountry < NUM_COUNTRIES))
    {
      if (    (RISK_GetOwnerOfCountry(iCountry) == iPlayer)
           && (RISK_GetNumArmiesOfCountry(iCountry) > 2)
           && !GAME_IsEnemyAdjacent(iCountry))
        {
          i = 0;
          while (!boool && (i < 6) && (RISK_GetAdjCountryOfCountry(iCountry, i) != -1))
            {
              destCountry = RISK_GetAdjCountryOfCountry(iCountry, i);
              if (RISK_GetNumArmiesOfCountry(destCountry) == 1)
                {
                  AI_Move (iCountry, destCountry,
                           RISK_GetNumArmiesOfCountry(iCountry)/2);
                  boool = TRUE;
                }
              if (!boool)
                  i++;
            }
        }
      iCountry++;
    }
}


/************************************************************************ 
 *  FUNCTION: ExchangeCards
 *  HISTORY: 
 *     18.08.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void ExchangeCards(Int32 iPlayer)
{
  Int32 i, j, nb, typ, piCards[4], nbCards[4], piCardValues[MAX_CARDS];
  Flag fOptimal, fStronger, fSmaller;

  fStronger = IsStrongerPlayer(iPlayer);
  fSmaller = IsSmallerPlayer(iPlayer);
  nb = RISK_GetNumCardsOfPlayer(iPlayer);
  do
    {
      piCards[0]=piCards[1]=piCards[2]=piCards[3]=-1;
      nbCards[0]=nbCards[1]=nbCards[2]=nbCards[3]=0;
      fOptimal = FALSE;
      for (i=0; i<nb; i++)
        {
          piCardValues[i] = RISK_GetCardOfPlayer(iPlayer, i);
          /* Set the type of the card */
          if (piCardValues[i]<NUM_COUNTRIES)
            {
              typ = piCardValues[i] % 3;
	      nbCards[typ]++;
              if (RISK_GetOwnerOfCountry(piCardValues[i]) == iPlayer)
        	  piCards[typ] = i;
              else if (piCards[typ] == -1)
         	  piCards[typ] = i;
            }
	  else  /* Joker */
            {
	      piCards[3] = i;
	      nbCards[3]++;
	    }
        }
      if ((nbCards[0]>0)&&(nbCards[1]>0)&&(nbCards[2]>0))
        {
          AI_ExchangeCards(piCards);
          fOptimal = TRUE;
        }
      else if ((nbCards[3]>=2)&&((nbCards[0]>0)||(nbCards[1]>0)||(nbCards[2]>0)))
        {
          if (nbCards[2]>1)
            {
              piCards[1] = piCards[3];
              j = 0;
            }
          else if (nbCards[1]>1)
            {
              piCards[0] = piCards[3];
              j = 2;
            }
          else if (nbCards[0]>1)
            {
              piCards[2] = piCards[3];
              j = 1;
            }
          else if (nbCards[2]>0)
            {
              piCards[1] = piCards[3];
              j = 0;
            }
          else if (nbCards[1]>0)
            {
              piCards[0] = piCards[3];
              j = 2;
            }
          else
            {
              piCards[2] = piCards[3];
              j = 1;
            }
          i = 0;
          while (i<nb)
            {
              if (piCardValues[i] >= NUM_COUNTRIES)
                {
                  piCards[j]=i;
                  i = nb;
                }
              else
                  i++;
            }
          AI_ExchangeCards(piCards);
          fOptimal = TRUE;
        }
      else if (!fStronger && (nbCards[0]>=3))
        {
          j = 0;
          for (i=0; i<nb; i++)
              if ((piCardValues[i] % 3) == 0)
                  piCards[j++]=i;
          AI_ExchangeCards(piCards);
        }
      else if (fSmaller && (nbCards[1]>=3))
        {
          j = 0;
          for (i=0; i<nb; i++)
              if ((piCardValues[i] % 3) == 1)
                  piCards[j++]=i;
          AI_ExchangeCards(piCards);
        }
      else if (fSmaller && (nbCards[2]>=3))
        {
          j = 0;
          for (i=0; i<nb; i++)
              if ((piCardValues[i] % 3) == 2)
                  piCards[j++]=i;
          AI_ExchangeCards(piCards);
        }
      else if (nb >= 5)
        {
          if ((nbCards[0]>0)&&(nbCards[1]>0)&&(nbCards[3]>0))
              piCards[2] = piCards[3];
          else if ((nbCards[0]>0)&&(nbCards[3]>0)&&(nbCards[2]>0))
              piCards[1] = piCards[3];
          else if ((nbCards[3]>0)&&(nbCards[1]>0)&&(nbCards[2]>0))
              piCards[0] = piCards[3];
          else if (nbCards[0]>=3)
            {
              j = 0;
              for (i=0; i<nb; i++)
                  if ((piCardValues[i] % 3) == 0)
                      piCards[j++]=i;
            }
          else if (nbCards[1]>=3)
            {
              j = 0;
              for (i=0; i<nb; i++)
                  if ((piCardValues[i] % 3) == 1)
                      piCards[j++]=i;
            }
          else if (nbCards[2]>=3)
            {
              j = 0;
              for (i=0; i<nb; i++)
                  if ((piCardValues[i] % 3) == 2)
                      piCards[j++]=i;
            }
          else
            {
              piCards[0] = 0;
              piCards[1] = 1;
              piCards[2] = 2;
            }
          AI_ExchangeCards(piCards);
        }
      nb = RISK_GetNumCardsOfPlayer(iPlayer);
    }
  while ((fOptimal && (nb >= 3)) || (nb >= 5));
}


/************************************************************************ 
 *  FUNCTION: COLSON_Play
 *  HISTORY: 
 *     21.07.95  JC  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void *COLSON_Play(void *pData, Int32 iCommand, void *pArgs)
{
  switch(iCommand)
    {
    case AI_INIT_ONCE:
      {
        /* InitClient((Int32)pArgs); */
      }
      break;

    case AI_INIT_GAME:
      {
        InitGame((Int32)pArgs);
      }
      break;

    case AI_INIT_TURN:
      {
        numTurn[iCurrentPlayer]++;
      }
      break;

    case AI_FORTIFY:
      {
        Fortify(iCurrentPlayer);
      }
      break;

    case AI_PLACE:
      {
        Place(iCurrentPlayer);
      }
      break;

    case AI_ATTACK:
      {
        Attack(iCurrentPlayer);
      }
      break;

    case AI_MOVE_MANUAL:
      {
        HowManyArmiesToMove(iCurrentPlayer, (Int32 *)pArgs);
      }
      break;

    case AI_MOVE:
      {
        Move(iCurrentPlayer);
      }
      break;

    case AI_EXCHANGE_CARDS:
      {
        ExchangeCards(iCurrentPlayer);
      }
      break;

    case AI_SERVER_MESSAGE:
      {
      }
      break;

    case AI_MESSAGE:
      {
        MsgMessagePacket *msgMessagePacket = (MsgMessagePacket *)pArgs;
        Char strScratch[256];

        if (strstr(msgMessagePacket->strMessage, "pacte") != NULL)
          {
            snprintf(strScratch, sizeof(strScratch),
		     "%s a tenté de faire un pacte avec moi.",
                                RISK_GetNameOfPlayer(msgMessagePacket->iFrom));
            AI_SendMessage(DST_ALLPLAYERS, strScratch);
          }
        else
            AI_SendMessage(msgMessagePacket->iFrom, "Je ne répond pas");
      }
      break;
    }

  return pData;
}
