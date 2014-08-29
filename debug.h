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
 *   $Id: debug.h,v 1.4 1999/11/13 21:58:31 morphy Exp $
 */

#ifndef _DEBUG
#define _DEBUG

/* Turn assertions on if MEM_DEBUG is on */
#ifdef MEM_DEBUG
#ifndef ASSERTIONS
#define ASSERTIONS
#endif
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include "types.h"

/* This is the public header file and should be used by programmers
 * in their files.  Any functions not declared in here, as well as
 * any data structures in debug.c are OFF LIMITS!  Functions should
 * be accesed by their MEM_ equivalents to ensure that they disappear
 * when DEBUG is not defined.
 */

extern Flag  fBootStrapped;
extern FILE    *hDebugFile;
typedef void   *FakePtr;

#ifdef ASSERTIONS
#define D_Assert(expr, szError)  (expr) ? (void)0 : \
  (void)_D_AssertFailed(__FILE__, __LINE__, #expr, szError)
#define D_AssertWhere(expr, szError, iLine, szFile)  (expr) ? (void)0 : \
  (void)_D_AssertFailed(szFile, iLine, #expr, szError)
#define D_PrintStr(str) { fprintf(hDebugFile, str); fflush(hDebugFile); }
#define D_PrintStrInt32(str, iInt32) { fprintf(hDebugFile, str, iInt32); \
					 fflush(hDebugFile); }
#define D_PrintStrLong(str, lLong) { fprintf(hDebugFile, str, lLong); \
				       fflush(hDebugFile); }
#else
#define D_PrintStr(szString)          ;
#define D_PrintStrInt(szString,   i)  ;
#define D_PrintStrLong(szString, l)   ;
#define D_Assert(expr, str)           ;
#define D_AssertWhere(expr, s, i, e)  ;
#endif

#ifdef MEM_DEBUG
#define MEM_BootStrap(str)  if(fBootStrapped==FALSE) D_BootStrap(str)
#define MEM_Alloc(uSize) D_MemAlloc(uSize, __LINE__, __FILE__)
#define MEM_Free(vPtr) { D_MemFree((void *)vPtr, __LINE__, __FILE__); vPtr=NULL; }
#define MEM_CheckPointer(vPtr)  D_MemCheckPointer(vPtr, __LINE__, __FILE__, 1)
#define MEM_Shrink(vPtr, iNewSize)  D_MemShrink(vPtr, iNewSize)
#define MEM_Grow(vPtr, iNewSize)  D_MemGrow(vPtr, iNewSize)
#define MEM_DumpUsage()  D_MemDumpUsage()
#define MEM_CheckAllPointers()  D_MemCheckAllPointers(__LINE__, __FILE__)
#define MEM_DumpPointers() D_MemDumpPointers()
#define MEM_TheEnd()  D_TheEnd()
#else
#define MEM_Alloc(uSize)              malloc(uSize)
#define MEM_Free(vPtr)                free((void *)vPtr)
#define MEM_CheckPointer(vPtr)        ;
#define MEM_CheckAllPointers()        ;
#define MEM_Shrink(vPtr, iNewSize)    realloc(vPtr, iNewSize)
#define MEM_Grow(vPtr, iNewSize)      realloc(vPtr, iNewSize)
#define MEM_DumpUsage()               ;
#define MEM_BootStrap(str)            ;
#define MEM_TheEnd()                  ;
#endif

/* Random assertion helpers */
void _D_AssertFailed(CString strFile, UInt32 uiLine, CString strAssertion,
		     CString strError);

/* prototypes for debugging memory functions */
void   *D_MemAlloc(size_t uSize, UInt32 uiLine, CString strFile);
void    D_MemFree(FakePtr fakePtr, UInt32 uiLine, CString strFile);
void   *D_MemShrink(void *vPtr, size_t sizeNew);
void   *D_MemGrow(void *vPtr, size_t sizeNew);
void    D_MemCheck(void);
void    D_MemDumpUsage(void);

/* Pointer Checker stuff */
void    D_MemCheckPointer(FakePtr fakePtr, UInt32 uiLine, CString strFile, 
			  Flag fDisp);
void    D_MemCheckAllPointers(UInt32 uiLine, CString strFile);
void    D_MemDumpPointers(void);
size_t  D_MemGetBlockSize(FakePtr fakePtr);

/* Prototypes for hashing functions used */
void    D_MemHashNewEntry(Pointer uiKey, size_t sizeBlock, UInt32 uiLine, 
			  CString strFile);
void    D_MemInitHashTable(void);
Flag    D_MemKeyInTable(Pointer uiKey);
UInt32  D_MemHashFunc(Pointer uiNumber);
void    D_MemDeleteEntry(Pointer uiKey);

/* Misc prototypes */
void    D_BootStrap(CString str);
void    D_TheEnd(void);
char   *StringCopy(CString str1, CString str2, UInt16 usLength);

#endif
