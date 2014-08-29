#ifndef _AICONWAYH
#define _AICONWAYH

#define CONWAY_GetLastOwnerOfCountry(c) (c->lastowner)

typedef struct country
  {
    struct continent *continent;           
    int lastowner;                         /**/ 
    struct _COUNTRYLIST *adjacentlist;     /**/
    int movable;
    int pass_through;
    int *is_connected;
    int flag;
    int usefulness;
    int criticalness;
    int nearness;

    int iIndex;
  }
COUNTRY;

typedef struct _COUNTRYLIST
{
  COUNTRY *country;
  struct _COUNTRYLIST *next;
} COUNTRYLIST;

typedef struct join
  {
    int continentfrom;
    int continentto;
    int countryfrom;
    int countryto;
  }
JOIN;

typedef struct continent
  {
    int       owner;       /* D */
    int       almost_owned;

    int       numcountries;
    COUNTRY  *countries;
    int       numjoins;                  /**/
    JOIN     *joins;                     /**/
  }
CONTINENT;

typedef struct player
  {
    COUNTRY *lastattacked;
  }
PLAYER;

typedef struct playertype
  {
    char *name;
    int def_strength;		/* Defensive strength  */
    int att_strength;		/* Attacking strength  */
    int dolastattacked;		/*  1 if want to change lastattacked on a
				 *  fight.
				 */
    int human;
    int dead;
    int almostowned;		/* Number of unowned sectors to want a
				 * continent  
				 */

    int excessarmies;		/* Number of excess armies to want a
				 * continent 
				 */
    int wantbestcont;		/* Want the continent you are strongest on */
    int wantback;		/* Want to get countries back */
    int wantallwinning;		/* Want to get all when winning */
    int passive;
    int passivewantbackfrom;	/* Will get a country from this person */
    int smart;
    int smartwantbackfrom;	/* Will get a country back from this person */

    int break1;			/* Break continent from 1 away */
    int break2;			/* Break continent from 2 away */
    int adddist;		/* add to distance  score for attacking this
				 * type  
				 */
    int concentratelastattack;	/* Concentrate on last attacked space */
  }
PLAYERTYPE;

/* Functions */
COUNTRYLIST *CLIST_CreateEmpty (void);
COUNTRYLIST *CLIST_GetDesiredCountries (int player);
void CLIST_RemoveCountry (COUNTRYLIST *cl, COUNTRY *c);
void CLIST_AddCountry (COUNTRYLIST *cl, COUNTRY *country);
void CLIST_Destroy (COUNTRYLIST *cl);
void CLIST_CalculateUsefulness (int player, COUNTRYLIST *pDesiredCountries);

CONTINENT *CONT_GetContinent(int iCont);
int  CONT_FindDesired (int player);
int  CONT_GetOwner(int iCont);
int  CONT_AlmostOwned (CONTINENT *cont, int player);

void  CONWAY_MoveArmies (int iPlayer);
int   CONWAY_NextPlayer(int);
void  CONWAY_InitWorld(void);
void *CONWAY_Play(void *pData, Int32 iCommand, void *pArgs);
void  CONWAY_FortifyTerritories (int player, int amount, 
				 COUNTRYLIST *pDesiredCountries);

int  passive_attack (COUNTRY *from, COUNTRY *to);
void find_destination (COUNTRYLIST *pDesiredCountries, int player);
void copy_armies_movable (void);
int  join_contains_country (JOIN *joins, int numjoins, int contno, int num);
void make_wholelist (void);

void CNT_CalculateUsefulness (COUNTRY *c);
int  CNT_IsBorder (COUNTRY *c);
void CNT_DistributeArmiesEvenly (int player, int amount);
void CNT_CalculateUsefulness (COUNTRY *c);
void CNT_IsAdjacent(COUNTRY *c);
int  CNT_IsNextTo(COUNTRY *c0, COUNTRY *c1);

#endif
