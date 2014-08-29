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
 *   $Id: buildmap.c,v 1.11 2000/01/20 17:56:57 morphy Exp $
 *
 *   $Log: buildmap.c,v $
 *   Revision 1.11  2000/01/20 17:56:57  morphy
 *   Made this thing a bit less verbose, it's running too fast for the old
 *   style progress indications to be of use anymore.
 *
 *   Revision 1.10  2000/01/10 22:47:40  tony
 *   made colorstuff more private to colormap.c, only scrollbars get set wrong, rest seems to work ok now
 *
 *   Revision 1.9  2000/01/09 18:20:15  morphy
 *   Backed out change done in rev 1.7: colourmap entries should not be terminated with newline
 *
 * ACHTUNG!! iNumColors used here is not the global one!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "types.h"

typedef struct _Color
{
  Byte r, g, b;
} Color;

typedef struct _BoundingBox
{
  Int32 l, r, t, b;
} BoundingBox;

struct Directory
{
  Int32   iWidth, iHeight, iLength;
  Int32   lOffset;
} pDirectory[256];


/* No matter where these are, the pseudocolor image always has these
 * colors at these entries.
 */

#define BLACK  255
#define WHITE  254

Color        pColormap[256];
BoundingBox  pBoundingBox[256];

/************************************************************************
 * Purpose:  Take an image in raw format (i.e. width height rgb rgb rgb)
 * with up to 256 unique colors and compress it in a run length encoded
 * fashion, saving the colormap at the beginning of the file.  The new
 * format is as follows (plain numbers, raw form, and [] means nothing):
 *
 *  [Width Height]
 *  [number of consecutive pixels] [color]
 *  [number of consecutive pixels] [color]
 *  .....
 *  [# of colors]
 *  [R G B] 
 *  [R G B] 
 *  ...
 * 
 * Note that this is not entirely efficient as a run of dissimilar
 * pixels will double in length.  However, for images that I am going
 * to be compressing this is not the case (maps).
 *
 *   2.14.94  ESF  Sped up by reading in the file all at once.
 *
 *   2.14.94  ESF  Added purpose.  Generate a country file, that contains
 *                 all of the countries, with color 0 backdrop, to use as
 *                 the backing for the cards.
 * 
 *   2.22.94  ESF  Added a pseudocolor image so that the country compression
 *                 works correctly.
 *
 *   2.22.94  ESF  Fixed country saving bug.
 *   3.07.94  ESF  Started work on colormap extras (detecting black and white).
 *   5.04.94  ESF  Fixed bugs related to colormap extras.
 *   6.25.94  ESF  Fixed country file color bug.
 *   1.15.95  ESF  Fixed memory leak.
 ************************************************************************/

void Exit(CString strError);
int  Compress(Byte *pbBuffer, Int32 iNumBytes, FILE *hOut);

int main(int argc, char **argv) {
  Int32     iNumColors=0, iWidth, iHeight, iCWidth, iCHeight;
  Color     colorCache;
  FILE     *hIn, *hOut, *hOut2;
  Int32     x, y, i, iTotalPixels=0;
  Byte      r, g, b, bRLE_Color, bRLE_Length;
  Byte      fFound, fCacheValid;
  Byte     *pbTrueColor, *pbPseudoColor, *pbCountry, *pbMisc, *pbTrue;
  Byte     *pbPseudo;
  Char      strBuf[80];

  /* Init. and open all the needed files */
  if (argc!=4)
    Exit("Usage is \"compress <infile> <outfile> <countryfile>\"");

  if ((hIn=fopen(argv[1], "r"))==NULL)
    Exit("Cannot open input file");
  
  if ((hOut=fopen(argv[2], "w"))==NULL)
    Exit("Cannot open output file");
  
  if ((hOut2=fopen(argv[3], "w"))==NULL)
    Exit("Cannot open output file for countries");
  
  /* Init cache */
  fCacheValid = 0;

  /* Init RLE vars */
  bRLE_Color =  0; 
  bRLE_Length = 0;

  /* Init bounding boxes */
  for (i=0; i!=256; i++)
    {
      pBoundingBox[i].l = pBoundingBox[i].t = 65535; 
      pBoundingBox[i].b = pBoundingBox[i].r == -1; 
    }

  /* Read the header and make sure that it's the right type of ppm image */
  fgets(strBuf, sizeof(strBuf), hIn);
  if (strcmp(strBuf, "P6\n"))
    Exit("Not the right type of ppm (need 24 bit raw image)!");

  /* Strip out comments */
  do
    fgets(strBuf, sizeof(strBuf), hIn);
  while(strBuf[0]=='#');
  
  /* Find out the dimensions of the image */
  sscanf(strBuf, "%d %d", &iWidth, &iHeight);
  
  /* Get the number of colors */
  fgets(strBuf, sizeof(strBuf), hIn);
  sscanf(strBuf, "%d", &iNumColors);

  /* Height and width info */
  printf("Image is [%dx%d].\n", iWidth, iHeight);
  printf("Image reportedly has %d colors.\n", iNumColors);
  iNumColors=0;

  /* Save room for number of colors */
  fprintf(hOut, "%4d %4d %3d\n", iWidth, iHeight, 0);
  
  /* Allocate memory for the images */
  if ((pbTrue=pbTrueColor=
       (unsigned char *)MEM_Alloc(iHeight*iWidth*3))==NULL)
    Exit("Could not allocate memory for the TrueColor image");
  if ((pbPseudo=pbPseudoColor=
       (unsigned char *)MEM_Alloc(iHeight*iWidth))==NULL)
    Exit("Could not allocate memory for the PseudoColor image");

  /* Read in the image */
  fread(pbTrueColor, 1, iHeight*iWidth*3, hIn);

  fprintf(stdout, "Compressing: ");
  fflush(stdout);
  /* Read in the file, allocating colormap entries as neccessary */
  for (y=0; y!=iHeight; y++)
    {
      for (x=0; x!=iWidth; x++)
	{
	  /* Read in a pixel */
	  r = *pbTrue++;
	  g = *pbTrue++;
	  b = *pbTrue++;
	  
	  /* If it is equal to the color cached, then simply output 
	   * the translation, otherwise look it up.
	   */
	  
	  if (fCacheValid && 
	      r==colorCache.r && g==colorCache.g && b==colorCache.b)
	    {
	      /* Color in cache, has RLE line reached its maximum length? */
	      if (++bRLE_Length==255)
		{
		  fprintf(hOut, "%c%c", bRLE_Length, bRLE_Color);
		  
		  /* Create the pseudocolor image */
		  memset(pbPseudo, bRLE_Color, bRLE_Length); 
		  pbPseudo+=bRLE_Length;
		  
		  /* Book-keeping */
		  iTotalPixels+=bRLE_Length;
		  bRLE_Length=0;
		}
	    }
	  else 
	    {
	      /* Color changed, flush RLE line! */
	      if (fCacheValid)
		{
		  fprintf(hOut, "%c%c", bRLE_Length, bRLE_Color);

		  /* Create the pseudocolor image */
		  memset(pbPseudo, bRLE_Color, bRLE_Length); 
		  pbPseudo+=bRLE_Length;

		  iTotalPixels+=bRLE_Length;
		}

	      bRLE_Length=1;
	      
	      /* Search, checking for special colors first */
	      if (r==0 && g==0 && b==0)
		fFound=1, i=BLACK;
	      else if (r==255 && g==255 && b==255)
		fFound=1, i=WHITE;
	      else
		for (i=0, fFound=0; i!=iNumColors && !fFound; i++)
		  if (r==pColormap[i].r && 
		      g==pColormap[i].g && 
		      b==pColormap[i].b)
		    fFound=1, i--;
	      
	      /* The new color */
	      bRLE_Color = i;
	    
	      if (!fFound) 
		{
		  /* Add color to colormap */
		  pColormap[iNumColors].r = r;
		  pColormap[iNumColors].g = g;
		  pColormap[iNumColors].b = b;
		  iNumColors++;
		}
	      
	      /* Put the color into the cache */
	      colorCache.r=r; colorCache.g=g; colorCache.b=b;
	      fCacheValid = 1;
	    }
	  
	  /* Do bounding box stuff */
	  if (pBoundingBox[bRLE_Color].t > y)
	    pBoundingBox[bRLE_Color].t = y;
	  else if (pBoundingBox[bRLE_Color].b < y)
	    pBoundingBox[bRLE_Color].b = y;
	  
	  if (pBoundingBox[bRLE_Color].l > x)
	    pBoundingBox[bRLE_Color].l = x;
	  else if (pBoundingBox[bRLE_Color].r < x)
	    pBoundingBox[bRLE_Color].r = x;
	}
      /* Stats... */
      if (!(y % 5))
	{
	  fprintf(stdout, ".");
	  fflush(stdout);
	}
    }
  printf(" done\n");
  
  /* The last segment */
  if (bRLE_Length)
    {
      fprintf(hOut, "%c%c", bRLE_Length, bRLE_Color);

      /* Create the pseudocolor image */
      memset(pbPseudo, bRLE_Color, bRLE_Length); 
      pbPseudo+=bRLE_Length;

      iTotalPixels+=bRLE_Length;
    }

  /* Known colormap entries, this is background/ocean (shouldn't be hardwired) */
  pColormap[iNumColors].r = 0;
  pColormap[iNumColors].g = 0;
  pColormap[iNumColors].b = 115;
  iNumColors++;

  pColormap[iNumColors].r = 255;
  pColormap[iNumColors].g = 255;
  pColormap[iNumColors].b = 255; iNumColors++;

  /* Print out colormap information */
  for (i=0; i!=iNumColors; i++)
      fprintf(hOut, "%c%c%c", pColormap[i].r, pColormap[i].g, pColormap[i].b);

  fseek(hOut, 10, SEEK_SET);
  fprintf(hOut, "%3d\n", iNumColors);

  /* The End */
  fclose(hIn);
  fclose(hOut);

  printf("There are %d pixels, I compressed %d of them, %s\n", iWidth*iHeight,
	 iTotalPixels, iWidth*iHeight==iTotalPixels ? "Excellent!" : "Ouch!");

  printf("Working on countries file: ");

  /* Adjust iNumColors down by two, since black/white don't count */
  iNumColors -= 2;

  /* Write a dummy header */
  fprintf(hOut2, "%d\n", iNumColors);
  fwrite(pDirectory, iNumColors, sizeof(pDirectory[0]), hOut2);
  
  /* Get all of the countries, except for 0, which is supposed to be
   * the background.
   * tony: this might be it!!
   */
  
  for (i=0; i < iNumColors; i++)
    {
      fprintf(stdout, ".", i);
      fflush(stdout);

      /* Allocate memory for the country structure */
      iCWidth  = pBoundingBox[i].r - pBoundingBox[i].l;
      iCHeight = pBoundingBox[i].b - pBoundingBox[i].t;
      
      if ((pbMisc=pbCountry=
	   (unsigned char *)MEM_Alloc(iCWidth*iCHeight))==NULL)
	Exit("Could not allocate memory for country pixmap...\n");

      /* Copy the portion over from the main map, and change all colors 
       * s.t. color != i to 0.
       */

      for (y=pBoundingBox[i].t; y<pBoundingBox[i].b; y++)
	for (x=pBoundingBox[i].l; x<pBoundingBox[i].r; x++)
	  if ((pbPseudoColor[y*iWidth + x])!=i)
	    *pbMisc++ = 255;
	  else
	    *pbMisc++ = i;
      
      /* Compress and save */
      pDirectory[i].lOffset = ftell(hOut2);
      pDirectory[i].iWidth  = iCWidth;
      pDirectory[i].iHeight = iCHeight;
      pDirectory[i].iLength = Compress(pbCountry, iCWidth*iCHeight, hOut2);

      MEM_Free(pbCountry);
    }
  printf(" done\n");

  /* Now go back and write the completed directory and then finish up */
  fseek(hOut2, 0, SEEK_SET);
  fprintf(hOut2, "%d\n", iNumColors);
  fwrite(pDirectory, iNumColors, sizeof(pDirectory[0]), hOut2);
  fclose(hOut2);

  printf("Construction of Map and Country files completed.\n");

  /* Free up memory */
  MEM_Free(pbTrueColor);
  MEM_Free(pbPseudoColor);
  MEM_TheEnd();
  exit(0);
}


/***********************/
void Exit(char *strError)
{
  printf("Error: %s\n", strError);
  exit(1);
}


/*****************************************************************/
/* Created  2.14.94 ESF                                          */
/* Finished 2.19.94 ESF                                          */
/* Finished 8.25.94 ESF                                          */
/*****************************************************************/
int Compress(Byte *pbBuffer, Int32 iNumBytes, FILE *hOut)
{
  Int32  i, iTotalPixels=0, iCompressedLength=0;
  Int32  iLastPixel=-1, iRunLength=0, iNewPixel;

  for (i=0; i!=iNumBytes; i++)
    {
      iNewPixel=(int)*pbBuffer++;

      if (iNewPixel==iLastPixel)
	{
	  /* Color in cache, has RLE line reached its maximum length? */
	  if (++iRunLength==255)
	    {
	      fprintf(hOut, "%c%c", iRunLength, iNewPixel);
	      iTotalPixels+=iRunLength;
	      iCompressedLength+=2;
	      iRunLength=0;
	    }
	}
      else 
	{
	  /* Color changed, flush RLE line if there is one! */
	  if (iLastPixel>=0)
	    {
	      fprintf(hOut, "%c%c", iRunLength, iLastPixel);
	      iCompressedLength+=2;
	      iTotalPixels+=iRunLength;
	    }
	  
	  iRunLength=1;
	  iLastPixel=iNewPixel;
	}
    }
  
  /* The last segment */
  if (iRunLength)
    {
      fprintf(hOut, "%c%c", iRunLength, iLastPixel);
      iTotalPixels+=iRunLength;
      iCompressedLength+=2;
    }

  if (iTotalPixels != iNumBytes)
    printf("Error!  Values don't match! (%d!=%d)\n", iTotalPixels, iNumBytes);

  return (iCompressedLength);
}







