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
 *   $Id: findtypes.c,v 1.4 1999/11/13 21:58:31 morphy Exp $
 */

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  FILE *file = fopen("types.h", "w");
  char *intType;
  char *byteType;
  char *shortType;
  char *pointerType;

  if (!file)
    {
      printf("Could not open types.h for writing...\n");
      exit(-1);
    }

  printf("Analyzing architecture, to determine types...");

  /* Find the types we need, which are:
   *
   *   32 bit [signed|unsigned] integer -- [U]Int32
   *   A type the size of a pointer -- Pointer  
   *   8 bit type [signed|unsigned] -- [Char|Byte]
   *   16 bit integer [[signed|unsigned] -- [U]Short
   *
   *   In addition, derive some utility types:
   *
   *   Pointer to a char (NULL terminated) -- CString
   *   Boolean value, how about a byte -- Bool
   */

  fprintf(file, "#ifndef _TYPES\n");
  fprintf(file, "#define _TYPES\n\n");

  /*************************************/
  fprintf(file, "/* 32 bit integer */\n");
  
  /* Find the types */
  if (sizeof(int) == 4)
    intType = "int";
  else if (sizeof(long int) == 4)
    intType = "long int";
  else if (sizeof(short int) == 4)
    intType = "short int";
  else
    {
      printf("Cannot find a 32 bit integer type on this machine.\n");
      exit(-1);
    }
  
  fprintf(file, "typedef %s Int32;\n", intType);
  fprintf(file, "typedef unsigned %s UInt32;\n\n", intType);

  /*************************************/
  fprintf(file, "/* Pointer type */\n");
  
  /* Find the types */
  if (sizeof(void *) == sizeof (int))
    pointerType = "int";
  else if (sizeof(void *) == sizeof(long int))
    pointerType = "long int";
  else if (sizeof(void *) == sizeof(short int))
    pointerType = "short int";
  else
    {
      printf("Cannot find integer the size of a pointer on this machine.\n");
      exit(-1);
    }
  
  fprintf(file, "typedef %s Pointer;\n\n", pointerType);

  /*************************************/
  fprintf(file, "/* 16 bit integer */\n");
  
  /* Find the types */
  if (sizeof(int) == 2)
    shortType = "int";
  else if (sizeof(long int) == 2)
    shortType = "long int";
  else if (sizeof(short int) == 2)
    shortType = "short int";
  else
    {
      printf("Cannot find a 16 bit integer type on this machine.\n");
      exit(-1);
    }
  
  fprintf(file, "typedef %s Int16;\n", shortType);
  fprintf(file, "typedef unsigned %s UInt16;\n\n", shortType);

  /*************************************/
  fprintf(file, "/* 8 bit integer */\n");
  
  /* Find the types */
  if (sizeof(int) == 1)
    byteType = "int";
  else if (sizeof(long int) == 1)
    byteType = "long int";
  else if (sizeof(short int) == 1)
    byteType = "short int";
  else if (sizeof(char) == 1)
    byteType = "char";
  else
    {
      printf("Cannot find an 8 bit integer type on this machine.\n");
      exit(-1);
    }
  
  fprintf(file, "typedef %s Char;\n", byteType);
  fprintf(file, "typedef unsigned %s Byte;\n\n", byteType);

  /*******************************************/
  fprintf(file, "/* Supplimentary types */\n");
  fprintf(file, "typedef %s *CString;\n", byteType);
  fprintf(file, "typedef unsigned %s Flag;\n\n", byteType);

  fprintf(file, "#endif\n");
  fclose(file);

  printf("done.\n");
  exit(0);
}
