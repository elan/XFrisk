/***********************************************************************
 *
 *   14-6-90  Created by Andrew Conway.
 *   22-4-95  ESF commented, changed function names, removed campuses.
 *
 *   Passive (defensive) RISK game player.
 *
 ***********************************************************************/

#include <stdlib.h>

#include "aiClient.h"
#include "aiConway.h"
#include "debug.h"
#include "types.h"
#include "riskgame.h"
#include "game.h"
#include "utils.h"

/* The species */
DefineSpecies(
	      CONWAY_Play,
	      "Conway's `Mean Player'",
	      "Andrew Conway",
	      "0.99",
	      "His best player.  It's vicious, and tends to attack you"
	      "in whatever places you show interest."
	      )

/* External data */
extern Int32  iCurrentPlayer;

static int      enemyarmies;
static int      playerarmies;
static COUNTRY *mindest, *minroute;
static int      mindist;
static COUNTRYLIST    *pDesiredCountries=NULL, *pAllCountries=NULL;
static Flag     fCurrentPlayerIsWinning;
static CONTINENT       pContinents[NUM_CONTINENTS]; 
static PLAYER          players[MAX_PLAYERS];
static PLAYERTYPE      playerType = { "Mean", 0, 0, 1, 0, 0, 2, 5, 
					 0, 1, 1, 0, 1, 1, 0, 0, 1, 5, 0 };

/************************************************************************ 
 *  FUNCTION: 
 *  HISTORY: 
 *     04.23.95  ESF  Created.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void *CONWAY_Play(void *pData, Int32 iCommand, void *pArgs)
{
  UNUSED(pArgs);
  switch (iCommand)
    {
    /*********************/
    case AI_INIT_ONCE:
      {
	/* Set up the world to appear as Conway wants it to */
	printf("AI-CONWAY: Creating Conway-world...\n");
	CONWAY_InitWorld();

	/* Create a list of all the countries */
	pAllCountries = CLIST_CreateEmpty();
	make_wholelist();
      }
      break;

    /*********************/
    case AI_INIT_TURN:
      {
	/* Free old list */
	if (pDesiredCountries)
	  CLIST_Destroy (pDesiredCountries);

	fCurrentPlayerIsWinning  = CONT_FindDesired (iCurrentPlayer);
	pDesiredCountries        = CLIST_GetDesiredCountries (iCurrentPlayer);
	
	CLIST_CalculateUsefulness (iCurrentPlayer, pDesiredCountries);
      }
      break;      

    /*********************/
    case AI_FORTIFY:
      CONWAY_FortifyTerritories (iCurrentPlayer, 
				 1, /* for now */
				 pDesiredCountries);
      break;      

    /*********************/
    case AI_PLACE:
      CONWAY_FortifyTerritories (iCurrentPlayer, 
				 RISK_GetNumArmiesOfPlayer(iCurrentPlayer),
				 pDesiredCountries);
      break;

    /*********************/
    case AI_ATTACK:
      {
	if (mindest)
	  {
	    if (passive_attack (mindest, minroute))
	      {			
		/* RAMPAGE!!! */
		find_destination (pDesiredCountries, iCurrentPlayer);
		while (mindest && 
		       (RISK_GetNumArmiesOfCountry(mindest->iIndex) > 2))
		  {
		    if (passive_attack (mindest, minroute))
		      CLIST_RemoveCountry (pDesiredCountries, minroute);
		    find_destination (pDesiredCountries, iCurrentPlayer);
		  }
		CLIST_CalculateUsefulness (iCurrentPlayer, pDesiredCountries);
	      }
	  }
	copy_armies_movable ();
      }
      break;

    /*********************/
    case AI_MOVE:
      CONWAY_MoveArmies (iCurrentPlayer);
      break;

    /*********************/
    case AI_MESSAGE:
      break;
    }
  
  return pData;
}


#define NEARNESS_WEIGHT 10
#define BORDER_WEIGHT   10
#define NEXT_BORD_WEIGHT 2

typedef enum {Min, Max} Kind;

typedef int (*EvalFunc)(COUNTRY *);
int CLIST_Evaluate (COUNTRYLIST *from, EvalFunc foo);

COUNTRYLIST *CLIST_Create (COUNTRY *country)
{
  COUNTRYLIST *p;
  
  p = (COUNTRYLIST *) MEM_Alloc (sizeof (COUNTRYLIST));
  p->country = country;
  p->next = NULL;
  return p;
}

COUNTRYLIST *CLIST_CreateEmpty (void)
{
  return CLIST_Create (NULL);
}

void CLIST_AddCountry (COUNTRYLIST *cl, COUNTRY *country)
{
  while (cl->next)
    cl = cl->next;
  cl->next = CLIST_Create (country);
}

void CLIST_RemoveCountry (COUNTRYLIST *cl, COUNTRY *c)
{
  COUNTRYLIST *p;

  if (cl == NULL)
      return;
  for (p=cl, cl=cl->next; 
       cl; 
       p=cl, cl=cl->next)
    {
      if (cl->country == c)
	{
	  p->next = cl->next;
	  MEM_Free (cl);
	  return; /* Used to not return! */
	}
    }
}

void CLIST_Destroy (COUNTRYLIST *cl)
{
  COUNTRYLIST *p;
  for (; cl; cl = p)
    {
      p = cl->next;
      MEM_Free (cl);
    }
}

static COUNTRY *savedcountry;

int CNT_NextToSaved (COUNTRY *country)
{
  return CNT_IsNextTo (savedcountry, country);
}

int CLIST_Evaluate (COUNTRYLIST *from, EvalFunc function)
{
  int value = 0;
  for (from = from->next; from; from = from->next)
    value += ((*function) (from->country));
  return value;
}

COUNTRYLIST *CLIST_Extract (COUNTRYLIST *from, int (*function)())
{
  COUNTRYLIST *cl;
  cl = CLIST_CreateEmpty ();
  for (from = from->next; from; from = from->next)
    if ((*function) (from->country))
      {
	CLIST_AddCountry (cl, from->country);
      }
  return cl;
}

int CNT_IsOwned (COUNTRY *country)
{
  int res;
  res = (iCurrentPlayer == RISK_GetOwnerOfCountry(country->iIndex));
  return res;
}

COUNTRYLIST *CLIST_GetOwned (int player)
{
  iCurrentPlayer = player;
  return CLIST_Extract (pAllCountries, CNT_IsOwned);
}

int CNT_IsNearlyOwned (COUNTRY *country)
{
  return ((iCurrentPlayer == RISK_GetOwnerOfCountry(country->iIndex)) || 
	  CLIST_Evaluate (country->adjacentlist, (EvalFunc)CNT_IsOwned));
}

int CNT_IsDesired (COUNTRY *country)
{
  const int owner = RISK_GetOwnerOfCountry(country->iIndex);

  /* If we already own it --> No */
  if (iCurrentPlayer == owner)
    return 0;

  /* If we are winning and we want to go for everything --> Yes */
  if (fCurrentPlayerIsWinning && playerType.wantallwinning)
    return 1;
  
  /* If we already own the continent --> Yes */
  if (country->continent->almost_owned)
    return 1;

  /* If we last owned it and we like getting back things --> Yes */
  if (iCurrentPlayer == CONWAY_GetLastOwnerOfCountry(country) && 
      playerType.wantback)
    return 1;

  /* If the continent's owned... */
  if (country->continent->owner != -1)
    {
      /* ...and we own things around it --> Yes */
      if (playerType.break1 && CLIST_Evaluate (country->adjacentlist, 
						   (EvalFunc)CNT_IsOwned))
	return 1;

      /* ...and we almost own things around it --> Yes */
      if (playerType.break2 && CLIST_Evaluate (country->adjacentlist, 
						  (EvalFunc)CNT_IsNearlyOwned))
	return 1;
    }

  /* No */
  return 0;
}

COUNTRYLIST *CLIST_GetDesiredCountries (int player)
{
  iCurrentPlayer = player;
  return CLIST_Extract (pAllCountries, CNT_IsDesired);
}

int CNT_NumAttackers (COUNTRY *country)
{
  if (RISK_GetOwnerOfCountry(country->iIndex) == iCurrentPlayer)
    return 0;
  else
    return RISK_GetNumArmiesOfCountry(country->iIndex) + 1;
}

static COUNTRY *pDefendedCountry;
static int      iExtremeDefense;
static Kind     kind;

void CNT_Vulnerability (COUNTRY *pCountry)
{
  int iBadDefense;

  /* If we don't own it, forget about it... */
  if (RISK_GetOwnerOfCountry(pCountry->iIndex) != iCurrentPlayer)
    return;
  
  /* If we've already defended it as much as possible, forget it */
  if (RISK_GetNumArmiesOfCountry(pCountry->iIndex) == 99)
    return;

  /* See how badly defended this country is.  Give points for attackers
   * adjacent to it.  Add points for usefulness of the country, and take
   * points away for armies that we have on the country.
   */

  iBadDefense = CLIST_Evaluate (pCountry->adjacentlist, 
				(EvalFunc)CNT_NumAttackers) - 
				  RISK_GetNumArmiesOfCountry(pCountry->iIndex)
				    + pCountry->usefulness;
  
  if ((!pDefendedCountry) || 
      (kind == Max && iBadDefense > iExtremeDefense) ||
      (kind == Min && iBadDefense < iExtremeDefense))
    {
      /* We've found a worse defended country */
      iExtremeDefense  = iBadDefense;
      pDefendedCountry = pCountry;
    }
}

COUNTRY *CNT_WorstDefended (COUNTRYLIST *have, int player)
{
  iCurrentPlayer = player;
  pDefendedCountry = NULL;
  kind = Max;
  CLIST_Evaluate (have, (EvalFunc)CNT_Vulnerability);
  return pDefendedCountry;
}

COUNTRY *CNT_BestDefended (COUNTRYLIST *have, int player)
{
  iCurrentPlayer = player;
  pDefendedCountry = NULL;
  kind = Min;
  CLIST_Evaluate (have, (EvalFunc)CNT_Vulnerability);
  return pDefendedCountry;
}

void CNT_SetZeroFlag (COUNTRY *country)
{
  country->flag = 0;
}

static COUNTRY *pStartCountry, *pBestSrcCountry, *pSrcCountry;
static COUNTRY *pBestDestinationCountry;
static int saveddist, iBestDistance;
static COUNTRYLIST *worklist;

void CNT_AddToWorklist (COUNTRY *country)
{
  COUNTRYLIST *p;
  p = worklist;
  while (p->next)
    {
      p = p->next;
      if (p->country == country)
	return;
    }
  p->next = CLIST_Create (country);
}


void CNT_AddIfConquerable (COUNTRY *country)
{
  int dist;

  /* If we're at the start, return */
  if (country == pStartCountry)
    return;

  /* If we're at the source, return */
  if (country == pSrcCountry)
    return;

  /* If the country is owned by the current player */
  if (RISK_GetOwnerOfCountry(country->iIndex) == iCurrentPlayer)
    {
      /* It's cheaper to get through, since it already has armies */
      dist = saveddist - RISK_GetNumArmiesOfCountry(country->iIndex);

      /* If this is the best route we've found, use it */
      if ((!pBestDestinationCountry) || (dist < iBestDistance))
	{
	  pBestSrcCountry = pSrcCountry;
	  pBestDestinationCountry = country;
	  iBestDistance = dist;
	}
    }
  else /* Enemy country to get through */
    {
      /* Approximate cost to get through:  2+armies+meanness */
      dist = saveddist + 2 + RISK_GetNumArmiesOfCountry(country->iIndex);

      /*
	dist += ptype[players[RISK_GetOwnerOfCountry(country->iIndex)].type].
	adddist;
	*/

      /* If the cost is more than we could possible have -- forget it */
      if ((pBestDestinationCountry) && (dist >= iBestDistance + 99))
	return;

      /* If the cost is more then we have flagged, forget it */
      if ((country->flag) && (country->flag <= dist))
	return;

      /* Otherwise, add it to our worklist */
      country->flag = dist;
      CNT_AddToWorklist (country);
    }
}

int CONT_AlmostOwned (CONTINENT *cont, int player)
{
  int country, unowned = 0, iNumEnemyArmies = 0, ownarmies = 0;
  
  D_Assert(player == iCurrentPlayer, "Wrong player!");

  for (country = 0; country < cont->numcountries; country++)
    {
      /* If we don't own a country in the continent */
      if (player != RISK_GetOwnerOfCountry(cont->countries[country].iIndex))
	{
	  iNumEnemyArmies += RISK_GetNumArmiesOfCountry(cont->
							countries[country].
							iIndex);
	  unowned++;
	}
      else
	ownarmies += RISK_GetNumArmiesOfCountry(cont->countries[country].
						iIndex);
    }
  
  cont->almost_owned = (unowned <= playerType.almostowned);
  cont->almost_owned |= (iNumEnemyArmies + 4*unowned + 
			 playerType.excessarmies < ownarmies);
  enemyarmies += iNumEnemyArmies + 4*unowned;
  playerarmies += ownarmies;

  return (iNumEnemyArmies - ownarmies + 2*unowned);
}


int CONT_FindDesired (int player)   /* check continents wanted */
{
  int         cont;
  int         numarmies, numarmies_sofar;
  CONTINENT  *wantcont = NULL;

  enemyarmies     = 0;
  playerarmies    = 0;
  numarmies_sofar = 0;

  for (cont = 0; cont < NUM_CONTINENTS; cont++)
    {
      numarmies = CONT_AlmostOwned (CONT_GetContinent(cont), player);
      if (player == CONT_GetOwner(cont))
	continue;
      if ((!wantcont) || (numarmies < numarmies_sofar))
	{
	  wantcont = CONT_GetContinent(cont);
	  numarmies_sofar = numarmies;
	}
    }
  
  /* Agressive */
  if ((wantcont) && playerType.wantbestcont)
    wantcont->almost_owned = 1;
  
  return (enemyarmies < 2 * playerarmies);
}

void find_nearest (COUNTRY *pCountry)
{
  COUNTRYLIST *p;

  CLIST_Evaluate (pAllCountries, (EvalFunc)CNT_SetZeroFlag);

  pBestDestinationCountry = NULL;
  pStartCountry = pCountry;
  worklist = CLIST_Create (pCountry);
  pCountry->flag = RISK_GetNumArmiesOfCountry(pCountry->iIndex);
  
  while (worklist)
    {
      if(CNT_IsOwned(worklist->country)) {
        worklist = worklist->next;
        continue;
      }
      
      pSrcCountry = worklist->country;
      saveddist = pSrcCountry->flag;
      CLIST_Evaluate (pSrcCountry->adjacentlist, 
		      (EvalFunc)CNT_AddIfConquerable);
      p = worklist;
      worklist = worklist->next;
      MEM_Free (p);
    }
}

void copy_armies_movable (void)
{
  int cont, country;
  CONTINENT *thiscont;
  COUNTRY *thiscountry;

  for (cont = 0; cont < NUM_CONTINENTS; cont++)
    {
      thiscont = CONT_GetContinent(cont);
      for (country = 0; country < thiscont->numcountries; country++)
	{
	  thiscountry = thiscont->countries + country;
	  thiscountry->movable 
	    = RISK_GetNumArmiesOfCountry(thiscountry->iIndex);
	  thiscountry->pass_through = 0;
	}
    }
}

void CNT_IsNearest (COUNTRY *country)
{
  find_nearest (country);
  if ((!mindest) || (iBestDistance < mindist))
    {
      mindist = iBestDistance;
      mindest = pBestDestinationCountry;
      minroute = pBestSrcCountry;
    }
}

void find_destination (COUNTRYLIST *pDesiredCountries, int player)
{
  iCurrentPlayer = player;
  if (players[player].lastattacked && 
      playerType.concentratelastattack)
    {
      find_nearest (players[player].lastattacked);
      mindest = pBestDestinationCountry;
      minroute = pBestSrcCountry;
      mindist = iBestDistance * 2 / 3;
    }
  else
    mindest = NULL;
  CLIST_Evaluate (pDesiredCountries, (EvalFunc)CNT_IsNearest);
}

void CNT_DistributeArmiesEvenly (int player, int amount)
{
  COUNTRYLIST  *have;
  COUNTRY      *country;

  /* Get the list of owned countries */
  have = CLIST_GetOwned (player);

  /* While we have armies to place */
  while (amount > 0)
    {
      /* Get the worst defended one */
      country = CNT_WorstDefended (have, player);
      if (!country)
	return;

      /* Put an army on that country */
      AI_Place(country->iIndex, 1);
      
      /* amount--;
	 country->numarmies++;
	 draw_country (country);
	 */
    }
  CLIST_Destroy (have);
}

void CONWAY_FortifyTerritories (int player, int amount, 
			    COUNTRYLIST *pDesiredCountries)
{
  if ((!pDesiredCountries->next) && (!players[player].lastattacked))
    {
      CNT_DistributeArmiesEvenly (player, amount);
      mindest = NULL;
    }
  else
    {
      find_destination (pDesiredCountries, player);
      if (mindest)
	{
	  AI_Place(mindest->iIndex, amount);
	  /*
	    mindest->numarmies += amount;
	    draw_country (mindest);
	    */
	}
      else
	CNT_DistributeArmiesEvenly (player, amount);
    }
}

int passive_attack (COUNTRY *from, COUNTRY *to)
{
  Int32 iOldDestOwner = RISK_GetOwnerOfCountry(to->iIndex);

  /* Perform the attack */
  if(AI_Attack(from->iIndex, to->iIndex, 
	       ATTACK_DOORDIE, DICE_MAXIMUM, ARMIES_MOVE_MAX) == FALSE)
    return 0;
  
  /* Book-keeping */
  if (RISK_GetOwnerOfCountry(to->iIndex) != iOldDestOwner)
    {
      /* Who owned the country last? */
      to->lastowner = iOldDestOwner;

      /* Last attacked (and taken) from */
      players[iOldDestOwner].lastattacked = to;
    }
  
  if (to == players[RISK_GetOwnerOfCountry(from->iIndex)].lastattacked)
    {
      /* Got It! */
      players[RISK_GetOwnerOfCountry(from->iIndex)].lastattacked = NULL;
      return 1;
    }
  else
    return 0;
}

void CNT_FortifyNeighbor (COUNTRY *country)
{
  COUNTRY *pWorstDefendedCountry;
  if (RISK_GetOwnerOfCountry(country->iIndex) != iCurrentPlayer)
    return;

  while (country->movable)
  {
    pWorstDefendedCountry = CNT_WorstDefended (country->adjacentlist, 
					       iCurrentPlayer);
    
    /* If there is no worst defended, or it's the one we're looking
     * at, then forget it.
     */
    if ((!pWorstDefendedCountry) || (pWorstDefendedCountry == country))
      break;
    
    /* Move an army from the country to the worst defended country
     * of the ones it borders on.
     */
    
    AI_Move(country->iIndex, pWorstDefendedCountry->iIndex, 1);
    country->movable--;

    /*
      country->numarmies--;
      pWorstDefendedCountry->numarmies++;
      draw_country (country);
      draw_country (pWorstDefendedCountry);
      */
    
    if (!pWorstDefendedCountry->pass_through)
      {
	pWorstDefendedCountry->pass_through = 1;
	pWorstDefendedCountry->movable++;
      }
  }
}

void CNT_SetZeroNearness (COUNTRY *country)
{
  country->nearness = 0;
}

void CNT_CalculateUsefulness (COUNTRY *country)
{
  if (RISK_GetOwnerOfCountry(country->iIndex) != iCurrentPlayer)
    return;
  country->usefulness =
    ((country->continent->owner == iCurrentPlayer) 
     ? country->criticalness 
     : 0)
      + country->nearness;
}

void do_nearness (COUNTRY *country)
{
  find_nearest (country);
  if (pBestDestinationCountry)
    pBestDestinationCountry->nearness += NEARNESS_WEIGHT;
}

void CLIST_CalculateUsefulness (int player, COUNTRYLIST *pDesiredCountries)
{
  iCurrentPlayer = player;
  CLIST_Evaluate (pAllCountries, (EvalFunc)CNT_SetZeroNearness);
  CLIST_Evaluate (pDesiredCountries, (EvalFunc)do_nearness);
  CLIST_Evaluate (pAllCountries, (EvalFunc)CNT_CalculateUsefulness);
}

void CONWAY_MoveArmies (int iPlayer)
{
  /* Find the worst defended country, use a neighbor to fortify it */
  COUNTRYLIST  *pNeighbors, *pAllMyCountries = CLIST_GetOwned(iPlayer);
  COUNTRY      *pWorstDefended, *pBestDefended;
  int working = 10;

  while (working && pAllMyCountries->next)
    {
      pWorstDefended  = CNT_WorstDefended(pAllMyCountries, iPlayer);
      pNeighbors      = pWorstDefended->adjacentlist;
      pBestDefended   = CNT_BestDefended(pNeighbors, iPlayer);

      if (pBestDefended && 
	  RISK_GetNumArmiesOfCountry(pBestDefended->iIndex) >= 2)
	{
	  int iNumArmies = RISK_GetNumArmiesOfCountry(pBestDefended->iIndex)/2;
	  working = 0;
	  
	  /* Do it */
	  AI_Move(pBestDefended->iIndex, pWorstDefended->iIndex, iNumArmies);
	}
      else
	{
	  working--;
	  if (pWorstDefended)
	    CLIST_RemoveCountry(pAllMyCountries, pWorstDefended);
	}
    }
}

void distribute_passive_initial (int player, int amount)
{
  COUNTRYLIST *pDesiredCountries;

  fCurrentPlayerIsWinning = CONT_FindDesired (player);
  pDesiredCountries = CLIST_GetDesiredCountries (player);
  CLIST_CalculateUsefulness (player, pDesiredCountries);
  CONWAY_FortifyTerritories (player, amount, pDesiredCountries);
  CLIST_Destroy (pDesiredCountries);
}

/* Related to AI */

void CNT_IsAdjacent (COUNTRY *country)
{
  savedcountry = country;
  country->adjacentlist = CLIST_Extract (pAllCountries, CNT_NextToSaved);
}

void CNT_IsCritical (COUNTRY *country)
{
  if (CNT_IsBorder (country))
    country->criticalness = BORDER_WEIGHT;
  else
    country->criticalness = NEXT_BORD_WEIGHT * 
      CLIST_Evaluate (country->adjacentlist, (EvalFunc)CNT_IsBorder);
}

int CNT_IsBorder (COUNTRY *country)
{
  int contcount;
  int iCountry, contno;
  CONTINENT *cont, *thiscont;

  cont = country->continent;

  iCountry = country - cont->countries;
  contno = (int)(cont - pContinents);

  for (contcount = 0; contcount < NUM_CONTINENTS; contcount++)
    {
      thiscont = CONT_GetContinent(contcount);
      if (join_contains_country (thiscont->joins, 
				 thiscont->numjoins, 
				 contno, 
				 iCountry))
	return 1;
    }
  return 0;
}

void make_wholelist (void)
{
  int         cont, country;
  CONTINENT  *thiscont;
  COUNTRY    *thiscountry;

  for (cont = 0; cont < NUM_CONTINENTS; cont++)
    {
      thiscont = &pContinents[cont];
      for (country = 0; country < thiscont->numcountries; country++)
	{
	  thiscountry = thiscont->countries + country;
	  CLIST_AddCountry (pAllCountries, thiscountry);
	  thiscountry->pass_through = 0;
	}
    }

  /* Calculate all of the adjacent countries */
  CLIST_Evaluate (pAllCountries, (EvalFunc)CNT_IsAdjacent);

  /* Calculate the criticalness of the countries */
  CLIST_Evaluate (pAllCountries, (EvalFunc)CNT_IsCritical);
}


int join_contains_country (JOIN *joins, int numjoins, int contno, int num)
{
  while (numjoins--)
    {
      if (((joins->continentto == contno) && (joins->countryto == num)) ||
	  ((joins->continentfrom == contno) && (joins->countryfrom == num)))
	return 1;
      joins++;
    }
  return 0;
}

static int nextplayer;

void reown (COUNTRY *country)
{
  if (CONWAY_GetLastOwnerOfCountry(country) == iCurrentPlayer)
    country->lastowner = nextplayer = CONWAY_NextPlayer (nextplayer);
}

void redistribute_last (int player)
{
  iCurrentPlayer = player;
  nextplayer = -1;
  CLIST_Evaluate (pAllCountries, (EvalFunc)reown);
}

CONTINENT *CONT_GetContinent(int iCont)
{
  return &pContinents[iCont];
}

int CONT_GetOwner(int iCont)
{
  return pContinents[iCont].owner;
}

int CONWAY_NextPlayer(int iCurrentPlayer)
{
  do 
    {
      /* Go to the next player, wrap-around if we're at the end */
      iCurrentPlayer = (iCurrentPlayer+1) % MAX_PLAYERS;
    } 
  while (RISK_GetStateOfPlayer(iCurrentPlayer) == FALSE ||
	 RISK_GetAllocationStateOfPlayer(iCurrentPlayer) != ALLOC_COMPLETE);
  
  /* Return the player who's turn it is */
  return iCurrentPlayer;
}

int CNT_IsNextTo(COUNTRY *countrya, COUNTRY *countryb)
{
  int numa, numb, contnoa, contnob, i;
  CONTINENT *conta, *contb;
  JOIN *join;

  conta = countrya->continent;
  contb = countryb->continent;
  numa = countrya - conta->countries;
  numb = countryb - contb->countries;
  contnoa = conta - pContinents;
  contnob = contb - pContinents;

  if (conta == contb)
    return (countrya->is_connected[numb]);

  for (i=0; i<conta->numjoins; i++)
    {
      join = &conta->joins[i];
      if ((join->continentto == contnob) &&
	  (join->countryfrom == numa) &&
	  (join->countryto == numb))
	return 1;
    }
  for (i=0; i<contb->numjoins; i++)
    {
      join = &contb->joins[i];
      if ((join->continentto == contnoa) &&
	  (join->countryfrom == numb) &&
	  (join->countryto == numa))
	return 1;
    }
  return 0;
}

void CONWAY_InitWorld(void)
{
  Int32     i, j, k;
  Int32     iCountry, iCountry2;
  Int32     iNumCountries, iNumJoins;
  Int32     pTempCountries[NUM_COUNTRIES];
  JOIN      pTempJoins[NUM_COUNTRIES];

  /* For each continent, find number of countries */
  for (i=0; i!=NUM_CONTINENTS; i++)
    {
      for (j=iNumCountries=0; j!=NUM_COUNTRIES; j++)
	if (RISK_GetContinentOfCountry(j) == i)
	  pTempCountries[iNumCountries++] = j;
      
      /* Allocate the countries */
      pContinents[i].numcountries = iNumCountries;  
      pContinents[i].countries = (COUNTRY *)MEM_Alloc(sizeof(COUNTRY)*
						      iNumCountries);
      
      printf("AI-CONWAY: `%s' has %d countries.\n", 
	     RISK_GetNameOfContinent(i), iNumCountries);
      
      /* Fill in the countries */
      for (j=0; j!=iNumCountries; j++)
	{
	  iCountry = pTempCountries[j];

	  printf("             o `%s'\n", RISK_GetNameOfCountry(iCountry));
	  
	  pContinents[i].countries[j].continent = &pContinents[i];
	  pContinents[i].countries[j].iIndex = iCountry;
	  pContinents[i].countries[j].movable = 0;
	  pContinents[i].countries[j].lastowner = -1;
	  pContinents[i].countries[j].usefulness = 0;
	}
    }

  /* Now calculate the number of joins */
  for (i=0; i!=NUM_CONTINENTS; i++)
    {
      for (j=iNumJoins=0; j!=pContinents[i].numcountries; j++)
	{
	  iCountry = pContinents[i].countries[j].iIndex;
	  for (k=0; RISK_GetAdjCountryOfCountry(iCountry, k)!=-1 && k<6; k++)
	    {
	      iCountry2 = RISK_GetAdjCountryOfCountry(iCountry, k);
	      
	      if (RISK_GetContinentOfCountry(iCountry2) != i) 
		{
		  Int32 k, iCont1, iCont2;

		  iCont1 = i;
		  iCont2 = RISK_GetContinentOfCountry(iCountry2);
		  
		  /* Store countries as offset from ->countries */
		  for (k=0; k!=pContinents[iCont1].numcountries; k++)
		    if (pContinents[iCont1].countries[k].iIndex ==
			iCountry)
		      {
			pTempJoins[iNumJoins].countryfrom = k;
		      }
		  
		  for (k=0; k!=pContinents[iCont2].numcountries; k++)
		    if (pContinents[iCont2].countries[k].iIndex ==
			     iCountry2)
		      {
			pTempJoins[iNumJoins].countryto = k;
		      }

		  pTempJoins[iNumJoins].continentfrom = i;
		  pTempJoins[iNumJoins].continentto = 
		    RISK_GetContinentOfCountry(iCountry2);

		  iNumJoins++;
		}
	    }
	}

      /* Allocate the joins */
      pContinents[i].joins = (JOIN *)MEM_Alloc(sizeof(JOIN)*iNumJoins);
      pContinents[i].numjoins = iNumJoins;

      printf("AI-CONWAY: Continent `%s' has %d joins.\n", 
	     RISK_GetNameOfContinent(i), iNumJoins);

      for (j=0; j!=iNumJoins; j++)
	memcpy(&pContinents[i].joins[j], &pTempJoins[j], sizeof(JOIN));
    }

  /* Calculate the `is_connected' data member */
  for (i=0; i!=NUM_CONTINENTS; i++)
    {
      /* For each country in the continent */
      for (j=0; j!=pContinents[i].numcountries; j++)
	{
	  /* Allocate space for the array */
	  pContinents[i].countries[j].is_connected = 
	    (int *)MEM_Alloc(sizeof(Int32)*pContinents[i].numcountries);

	  /* Now, for each of the countries in the continent, answer the 
	   * question: Is country j connect to it?
	   */
	  
	  for (k=0; k!=pContinents[i].numcountries; k++)
	    if (GAME_CanAttack(pContinents[i].countries[j].iIndex, 
			       pContinents[i].countries[k].iIndex))
	      pContinents[i].countries[j].is_connected[k] = 1;
	    else 
	      pContinents[i].countries[j].is_connected[k] = 0;
	}
    }	
}

#if 0
    int pass_through; ??

    int flag;
    int movable;
#endif
