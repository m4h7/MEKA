/***************************************************************************
** object.c
**
** Routines to load the OBJECT file. Firstly it needs to determine whether
** the file is encrypted or not. This is to accommodate some of the early
** AGIv2 games that didn't bother about having it encrypted.
**
** (c) 1997 Lance Ewing - Inital code (26 Aug 97)
***************************************************************************/

#include <stdio.h>

#include "general.h"
#include "object.h"

int numObjects;
objectType *objects;

/**************************************************************************
** isObjCrypt
**
** Purpose: Checks whether the OBJECT file is encrypted with Avis Durgan
** or not. I havn't fully tested this routine, but it seems to work with
** all the AGI games that I've tried it on. What it does is check the
** end of the OBJECT file which should be all text characters if it is
** not encrypted. On the other hand, if the OBJECT file is encrypted,
** there is usually a lot of characters less than 0x20.
**************************************************************************/
boolean isObjCrypt(long fileLen, byte *objData)
{
   int i, checkLen;

   checkLen = ((fileLen < 20)? 10 : 20);

   /* Needs a fix here for Mixed Up Mother Goose */
   // ->>>

   for (i=fileLen-1; i>(fileLen - checkLen); i--) {
      if (((objData[i] < 0x20) || (objData[i] > 0x7F)) && (objData[i] != 0))
	      return TRUE;
   }

   return FALSE;
}

/**************************************************************************
** loadObjectFile
**
** Purpose: Load the names of the inventory items from the OBJECT file and
** their starting rooms.
**************************************************************************/
void loadObjectFile()
{
   FILE *objFile;
   int avisPos=0, objNum, i, strPos=0;
   long fileLen;
   byte *objData, *marker, tempString[80];
   word index;

   if ((objFile = fopen("OBJECT","rb")) == NULL) {
      printf("Cannot find file : OBJECT\n");
      exit(1);
   }

   fseek(objFile, 0, SEEK_END);
   fileLen = ftell(objFile);
   fseek(objFile, 0, SEEK_SET);
   marker = (objData = (char *)malloc(fileLen)) + 3;
   fread(objData, sizeof(byte), fileLen, objFile);
   fclose(objFile);

   if (isObjCrypt(fileLen, objData))
      for (i=0; i<fileLen; i++) objData[i] ^= "Avis Durgan"[avisPos++ % 11];

   numObjects = (((objData[1] * 256) + objData[0]) / 3);
   objects = (objectType *)malloc(sizeof(objectType)*numObjects);

   for (objNum=0; objNum<numObjects; objNum++, strPos=0, marker+=3) {
      index = *(marker) + 256*(*(marker+1)) + 3;
      while ((tempString[strPos++] = objData[index++]) != 0) {}
      objects[objNum].name = (char *)strdup(tempString);
      objects[objNum].roomNum = *(marker+2);
   }

   free(objData);
}

/***************************************************************************
** discardObjects
**
** Purpose: To deallocate the memory associated with the objects array.
***************************************************************************/
void discardObjects()
{
   int objNum;

   for (objNum=0; objNum<numObjects; objNum++)
      free(objects[objNum].name);

   free(objects);
}

void showObjects()
{
   int objNum;

   for (objNum=0; objNum<numObjects; objNum++) {
      printf("%-40s", objects[objNum].name);
   }
}

void testObjects()
{
   loadObjectFile();
   showObjects();
   discardObjects();
}

