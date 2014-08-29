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
 *   $Id: debug.c,v 1.5 1999/11/13 21:58:31 morphy Exp $
 */

#if defined (MEM_DEBUG) || defined (ASSERTIONS)
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "types.h"
#include "debug.h"

/* Debug output */
FILE *hDebugFile;
#endif

#ifdef MEM_DEBUG
#define ptrFakeToReal(a) (void *)(&((Pointer *)a)[0] - 1)
#define ptrRealToFake(a) (void *)(&((Pointer *)a)[0] + 1)

#define MAGIC_COOKIE1 0xab
#define MAGIC_COOKIE2 0xcd
#define MAGIC_COOKIE3 0xef
#define MAGIC_COOKIE4 0x12

#define MEM_FILL 0xCC
#define MAX_DATA 32

#define PAD_SIZE 8 /* In bytes, on each side of block. */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Data structure for hashing */
#define HASH_TABLE_SIZE 101

Flag fBootStrapped = FALSE;

/* Data type for storing line/file info for non stack trace debugging */
typedef struct _Trace
{
  UInt32   uiLine;
  Char   szFile[MAX_DATA];
} Trace;

/* A cell object for a singly linked list for the chain in chained hashing */
typedef struct _CELL
{
  Pointer        uiKey;       /* pointer to memory */
  size_t         size;        /* amount of memory */
  Trace          trcTrace;    /* trace data */
  struct _CELL  *next;
} Cell;

/* Declare the hash table */
struct
{
  UInt32      uiEntries;
  UInt32      uiTotalMem;
  UInt32      uiRealMem;
  Cell       *ppChain[HASH_TABLE_SIZE];
} HashTable;

/* Here is the magic cookie */
unsigned char MagicCookie[] =
{
 MAGIC_COOKIE1,
 MAGIC_COOKIE2,
 MAGIC_COOKIE3,
 MAGIC_COOKIE4
};

#endif

#ifdef ASSERTIONS
/************************************************************************ 
 *  FUNCTION: _D_AssertFailed
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *     10.03.94  ESF  Added check for hFile.
 *     06.05.97  DAH  hFile -> hDebugFile
 *  PURPOSE:
 *     Private function called when an assertion fails.
 *  NOTES: 
 ************************************************************************/
void _D_AssertFailed(CString strFile, UInt32 iLine, CString strAssert,
		     CString strError)
{
  Char  szTemp[128];
  Char *pNull = 0;

  MEM_BootStrap("debug.log");
  snprintf(szTemp, sizeof(szTemp), 
	   "Assertion Failed (%s) @ (%s, %d) :: %s\n", strAssert,
	  strFile, iLine, strError);
  if (hDebugFile)
    fprintf(hDebugFile, szTemp);
  printf("%s", szTemp);
  
  /* Dereference a NULL pointer to cause seg fault. */
  *pNull = 0;

  /* Just in case that didn't work. */
  exit(-1);
}
#endif
#ifdef MEM_DEBUG
/************************************************************************ 
 *  FUNCTION: D_BootStrap
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *     10.03.94  ESF  Added check for hFile.
 *     06.05.97  DAH  hFile -> hDebugFile
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void D_BootStrap(CString strFileName)
{
  /* Setup the hash table, and the debug log file */
  D_MemInitHashTable();
  hDebugFile = fopen(strFileName, "w");
  D_Assert(hDebugFile, "Could not open debugging file!");
  fBootStrapped = TRUE;
}


/************************************************************************ 
 *  FUNCTION: D_MemAlloc
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *  NOTES: 
 *     Allocate 8 bytes more than is called for and use this to check 
 *   for overflows.  Init it to MAGIC_COOKIE, then it got overwritten.
 *   Allocate 8 bytes so that we can put one after the memory region 
 *   returned, and one before, and 8 instead of 1 because some 
 *   architectures (namely the PA-RISC) barf if we don't.  Also fill 
 *   the memory with the value MEM_FILL.
 ************************************************************************/
void *D_MemAlloc(size_t size, UInt32 uiLine, CString strFile)
{
  void           *pvTemp;
  Byte           *pbTemp;
  Char            szTemp[256];
  size_t          s;

  MEM_BootStrap("debug.log");

  /* Are we allocating a > 0 size block? */
  D_AssertWhere(size>0, "Trying to allocate 0 or less bytes of memory!", 
		uiLine, strFile);

  /* Allocate the block */
  pvTemp = (void *)malloc(size + PAD_SIZE*2);
  pbTemp = (Byte *)pvTemp;

  /* Did it work? */
  snprintf(szTemp, sizeof(szTemp),
	   "Out of memory (%d bytes requested).", (Pointer)size);
  D_AssertWhere(pbTemp != NULL, szTemp, uiLine, strFile);

  /* Fill the memory with MEM_FILL */
  memset(pvTemp, MEM_FILL, size+PAD_SIZE*2);

  /* Init the safety Fields */
  for(s=0; s!=4; s++)
    pbTemp[s] = pbTemp[size+PAD_SIZE*2-s-1] = MagicCookie[s];

  /* Output debug info */
  snprintf(szTemp, sizeof(szTemp),
	   "MEM_Alloc():  Created at (%s, %d) ==> %d bytes, at "
	   "0x%08x\n", strFile, uiLine, size, (Pointer)ptrRealToFake(pbTemp));
  D_PrintStr(szTemp);

  /* Put the new pointer into the hash table and return it */
  D_MemHashNewEntry((Pointer)pbTemp, size, uiLine, strFile);
  return(ptrRealToFake(pbTemp));
}


/************************************************************************ 
 *  FUNCTION: D_MemFree
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     Frees memory.
 *  NOTES: 
 ************************************************************************/
void D_MemFree(FakePtr vPtr, UInt32 uiLine, CString strFile)
{
  size_t size;

  MEM_BootStrap("debug.log");

  /* Note that we're freeing a chunk */
  D_PrintStrLong("MEM_Free():  Freeing chunk [0x%08x] in (", (Pointer)vPtr);
  D_PrintStr(strFile);
  D_PrintStrLong(", %d)\n", uiLine);

  /* If the pointer is valid, check it for corruptness */
  D_AssertWhere(D_MemKeyInTable((Pointer)ptrFakeToReal(vPtr)),
                "Attempting to free bogus pointer!", uiLine, strFile);
  D_MemCheckPointer(vPtr, uiLine, strFile, 0);

  /* Update the memory usage info */
  size = D_MemGetBlockSize(vPtr);
  HashTable.uiTotalMem -= size;
  HashTable.uiRealMem  -= size - PAD_SIZE*2;

  /* Fill it with the value we use for new memory */
  /* Some platform was touchy about this line (!?!?!?) */
  /* memset(ptrFakeToReal(vPtr), MEM_FILL, size); */

  /* Take it out of the hash table */
  D_MemDeleteEntry((Pointer)ptrFakeToReal(vPtr));

  /* Actually free it */
  free(ptrFakeToReal(vPtr));
}


/************************************************************************ 
 *  FUNCTION: D_MemShrink
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     Realloc with the additional condition that we must be shrinking
 *   the size of the region allocated. 
 *  NOTES: 
 ************************************************************************/
void *D_MemShrink(FakePtr fakePtr, size_t sizeNew)
{
  Byte    *pbPtr;
  size_t   size;

  MEM_BootStrap("debug.log");

  /* Make sure we're shrinking memory */
  size = D_MemGetBlockSize(fakePtr);
  D_Assert(size>sizeNew, "Not Shrinking Memory!");

  /* Do the reallocation */
  pbPtr = (Byte *)MEM_Alloc(sizeNew);
  memcpy(pbPtr, ptrFakeToReal(fakePtr), size);
  MEM_Free(fakePtr);
  return (FakePtr)pbPtr;
}


/************************************************************************ 
 *  FUNCTION: D_MemGrow
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     Realloc with the additional condition that we must be enlarging
 *   the size of the region allocated. 
 *  NOTES: 
 ************************************************************************/
void *D_MemGrow(FakePtr fakePtr, size_t sizeNew)
{
  Byte    *pbPtr;
  size_t   size;

  MEM_BootStrap("debug.log");

  /* Make sure we're enlarging the size */
  size = D_MemGetBlockSize(fakePtr);
  D_Assert(size<sizeNew, "Not Growing Memory!");

  /* Do the reallocation */
  pbPtr = (Byte *)MEM_Alloc(sizeNew);
  memcpy(pbPtr, ptrFakeToReal(fakePtr), size);
  MEM_Free(fakePtr);
  return (FakePtr)pbPtr;
}


/************************************************************************ 
 *  FUNCTION: D_MemHashNewEntry
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     Hashes a key into the hash table using a specified hash function
 *   a specified index to a primary hashing function.  Uses chained
 *   hashing to resolve collisions.  Updates length of chain in table.
 *   Returns the length of the chain collided with (before adding new
 *   key).
 *  NOTES: 
 ************************************************************************/
void D_MemHashNewEntry(Pointer uiKey, size_t size, UInt32 uiLine, CString strFile)
{
  UInt32   uiIndex;
  Cell    *pCell = (Cell *)malloc(sizeof(Cell));

  /* was there enough memory? */
  D_Assert(pCell!=NULL, "Out of memory!");

  /* stick the new cell on the beginning of the list */
  uiIndex = D_MemHashFunc(uiKey);

  /* Init the fields of the Trace data */
  pCell->trcTrace.uiLine =  uiLine;
  StringCopy(pCell->trcTrace.szFile, strFile, MAX_DATA);

  /* Update the memory usage */
  HashTable.uiEntries ++;
  HashTable.uiTotalMem += size;
  HashTable.uiRealMem  += size + PAD_SIZE*2;

  /* set up a new cell with the key in it */
  pCell->uiKey = uiKey;
  pCell->next  = HashTable.ppChain[uiIndex];
  pCell->size  = size;

  /* link the new cell onto the old chain */
  HashTable.ppChain[uiIndex] = pCell;
}


/************************************************************************ 
 *  FUNCTION: D_MemDeleteEntry
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     This function cannot fail!  Deletes a memory table entry
 *   from the hash table.
 *  NOTES: 
 ************************************************************************/
void D_MemDeleteEntry(Pointer uiKey)
{
  UInt32     uiIndex;      /* slot in hash table that the key is hashed to */
  Cell      *pBob, *pJoe;  /* just your average pointers */

  uiIndex = D_MemHashFunc(uiKey);

  /* Searching the chain to find the key, if present */
  for(pJoe=pBob=HashTable.ppChain[uiIndex]; 
      pBob && pBob->uiKey!=uiKey; 
      pJoe=pBob, pBob=pBob->next)
    /* TwiddleThumbs() */;

  /* Are we in trouble? */
  D_Assert(pBob && pBob->uiKey == uiKey, "Element not in hash table (?!)");

  if(pBob==pJoe)
   {
     HashTable.ppChain[uiIndex] = pBob->next;
     free(pJoe);
   }
  else
   {
     pJoe->next = pJoe->next->next;
     free(pBob);
   }
}


/************************************************************************ 
 *  FUNCTION: D_MemInitHashTable
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     Initializes the hash table by setting the keys equal to zero and the
 *   chain pointers to NULL.
 *  NOTES: 
 ************************************************************************/
void D_MemInitHashTable(void)
{
  UInt32 i;

  /* Go through, set the chain pointer to NULL */
  for(i=0; i!=HASH_TABLE_SIZE; i++)
    HashTable.ppChain[i] = NULL;

  /* Reset other parameters */
  HashTable.uiEntries = 0;
  HashTable.uiTotalMem = HashTable.uiRealMem = (UInt32)0;
}


/************************************************************************ 
 *  FUNCTION: D_MemGetBlockSize
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     If entry in table, return size.  If not, assert and exit.
 *   Should always be in table when we call.
 *  NOTES: 
 ************************************************************************/
size_t D_MemGetBlockSize(FakePtr fakePtr)
{
  UInt32     uiIndex;  /* slot in the hash table that the key is hashed to */
  Cell      *pBob;     /* just your average pointer */
  Pointer    uiTemp = (Pointer)ptrFakeToReal(fakePtr);

  uiIndex = D_MemHashFunc(uiTemp);

  /* searching the chain to find the key, if present */
  for(pBob=HashTable.ppChain[uiIndex]; 
      pBob && pBob->uiKey!=uiTemp; 
      pBob=pBob->next)
    /* TwiddleThumbs() */ ;

  if(!(pBob && pBob->uiKey == uiTemp))
    return (size_t)0;

  return (size_t)pBob->size;
}


/************************************************************************ 
 *  FUNCTION: D_MemKeyInTable
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *     Searches in a hash table for a key and returns TRUE is the key
 *   is present and FALSE otherwise.
 *  NOTES: 
 ************************************************************************/
Flag D_MemKeyInTable(Pointer uiKey)
{
  UInt32   uiIndex;  /* slot in the hash table that the key is hashed to */
  Cell    *pBob;   /* just your average pointer */

  uiIndex = D_MemHashFunc(uiKey);

  /* searching the chain to find the key, if present */
  for(pBob=HashTable.ppChain[uiIndex]; pBob; pBob=pBob->next)
    if(pBob->uiKey==uiKey)
      return(TRUE);

  return(FALSE);
}


/************************************************************************ 
 *  FUNCTION: D_MemCheckPointer
 *  HISTORY:
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  
 *  PURPOSE: 
 *     Checks a pointer for data overrun.  NOTE!!!!! This is a fake type
 *   pointer that is checked, not areal pointer, since it assumes that
 *   the region of memory directly after and before one dword is valid!
 *  NOTES: 
 ************************************************************************/
void D_MemCheckPointer(FakePtr fakePtr, UInt32 uiLine, CString strFile, 
		       Flag fDisp)
{
  Byte     *bPtr = (Byte *)ptrFakeToReal(fakePtr);
  size_t    size, s;

  MEM_BootStrap("debug.log");

  /* Be verbose about it if so desired */
  if(fDisp)
   {
     D_PrintStrLong("MEM_CheckPointer():  Chunk [0x%08x] in (", 
		    (Pointer)fakePtr);
     D_PrintStr(strFile);
     D_PrintStrInt32(", %d) ", uiLine);
   }

  /* Is it even in the hash table? */
  D_AssertWhere((size=D_MemGetBlockSize(fakePtr)), 
		"Bogus pointer!", uiLine, strFile);;

  if(fDisp)
    D_PrintStrLong(" [%d bytes]\n", (size_t)size);

  /* Check for the magic cookie */
  for(s=0; s!=4; s++)
   {
     if(bPtr[s] != MagicCookie[s])
      {
        /* Print the cookie */
        D_PrintStr("Lower Cookie: [");
        for(s=0; s!=4; s++)
          D_PrintStrLong("0x%02x|", bPtr[s]);
        D_PrintStr("]\n");

        D_AssertWhere(FALSE, "Corrupt Memory (Underflow)!", uiLine, strFile);
      }

     if(bPtr[size+PAD_SIZE*2-s-1] != MagicCookie[s])
      {
        /* Print the cookie */
        D_PrintStr("Upper Cookie: [");
        for(s=0; s!=4; s++)
          D_PrintStrLong("0x%02x|", (Pointer)bPtr[size+PAD_SIZE*2-s-1]);
        D_PrintStr("]\n");

        D_AssertWhere(FALSE, "Corrupt Memory (Overflow)!", uiLine, strFile);
      }
   }
}


/************************************************************************ 
 *  FUNCTION: D_MemCheckAllPointers
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void D_MemCheckAllPointers(UInt32 uiLine, CString strFile)
{
  UInt32   i;
  Cell    *pCell;

  MEM_BootStrap("debug.log");

  /* Dump some verbose information */
  D_PrintStr("MEM_CheckAllPointers():  Performing memory check in (");
  D_PrintStr(strFile);
  D_PrintStrLong(", %d)...\n", uiLine);
  D_PrintStrInt32("  Total Memory allocated by client: %d\n", 
		HashTable.uiTotalMem);

  /* Run through the whole hash pointer, dumping and checking the pointers */
  for(i=0; i!=HASH_TABLE_SIZE; i++)
    for(pCell=HashTable.ppChain[i]; pCell; pCell=pCell->next)
     {
       D_PrintStr("  ");
       D_MemCheckPointer(ptrRealToFake(pCell->uiKey), pCell->trcTrace.uiLine,
                         pCell->trcTrace.szFile, 1);
     }
}


/************************************************************************ 
 *  FUNCTION: D_MemDumpPointers
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void D_MemDumpPointers(void)
{
  UInt32   i, uiCount;
  Cell    *pCell;

  MEM_BootStrap("debug.log");
  D_PrintStr("MEM_DumpPointers():  Dumping all pointers...\n");

  /* Run through the whole hash table */
  for(i=uiCount=0; i!=HASH_TABLE_SIZE; i++)
   {
    D_PrintStrInt32("%d: ", i);
    for(pCell=HashTable.ppChain[i]; pCell; pCell=pCell->next, uiCount++)
      D_PrintStrLong("0x%08x, ", (Pointer)pCell->uiKey);
    D_PrintStr("\n");
   }
  D_PrintStrInt32("***** Total Chunks in Hash Table: %d\n", uiCount);
}


/************************************************************************ 
 *  FUNCTION: StringCopy
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
CString StringCopy(CString strDst, CString strSrc, UInt16 usNum)
{
  strncpy(strDst, strSrc, usNum-1);
  strDst[usNum-1] = '\0';
  return(strDst);
}


/************************************************************************ 
 *  FUNCTION: D_TheEnd
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
void D_TheEnd(void)
{
  UInt32   i;
  Cell    *p, *q, *pCell;  /* Mind your P's and Q's... */

  /* Check for memory leaks */
  D_PrintStr("************ Memory Leaks ***********\n");
  D_PrintStrInt32("  Total Memory allocated by client: %d\n", 
		HashTable.uiTotalMem);

  /* Run through the whole hash pointer, freeing all of the pointers */
  for(i=0; i!=HASH_TABLE_SIZE; i++)
    for(pCell=HashTable.ppChain[i]; pCell; pCell=pCell->next)
     {
       D_PrintStr("  ");
       D_MemCheckPointer(ptrRealToFake(pCell->uiKey), pCell->trcTrace.uiLine,
                         pCell->trcTrace.szFile, 1);
       /* EThreads was touchy about this line (!?!?) */
       /* MEM_Free(ptrRealToFake(pCell->uiKey)); */
     }

  for (i=0; i!=HASH_TABLE_SIZE; i++)
    for (p=HashTable.ppChain[i]; p; p=q)
     {
       q = p->next;
       free(p);
     }
}


/************************************************************************ 
 *  FUNCTION: D_MemHashFunc
 *  HISTORY: 
 *     ??.??.93  ESF  Created.
 *     05.19.94  ESF  Cleaned up.
 *  PURPOSE: 
 *  NOTES: 
 ************************************************************************/
UInt32 D_MemHashFunc(Pointer x) 
{ return (UInt32)(x % (Pointer)HASH_TABLE_SIZE); }

#else
#define NOT_EMPTY
#endif

