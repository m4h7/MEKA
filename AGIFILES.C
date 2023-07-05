/***************************************************************************
** AGIFILES.C
**
** Routines to handle AGI file system. These functions should enable you
** to load individual LOGIC, VIEW, PIC, and SOUND files into memory. The
** data is stored in a structure of type AGIFile. There is no code that
** is specific to the above types of data file though.
**
** (c) 1997 Lance Ewing
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "general.h"
#include "agifiles.h"
#include "decomp.h"

#define  AVIS_DURGAN  "Avis Durgan"

AGIFilePosType logdir[256], picdir[256], viewdir[256], snddir[256];
int numLogics, numPictures, numViews, numSounds;
boolean version3=FALSE;
char gameSig[10]="";

/***************************************************************************
** initFiles
**
** Purpose: To load the AGI resource directories and to determine whether
** the AGI files are for an AGIv3 game or for an AGIv2 game. It will also
** initialize the game signature in the case of a version 3 game.
***************************************************************************/
void initFiles()
{
   loadGameSig(gameSig);
   if (strlen(gameSig) > 0) version3 = TRUE;
   loadAGIDirs();
}

/***************************************************************************
** loadAGIDir
**
** Purpose: To read in an individual AGI directory file. This function
** should only be called by loadAGIDirs() below.
***************************************************************************/
void loadAGIDir(int dirNum, char *fName, int *count)
{
   FILE *fp;
   unsigned char byte1, byte2, byte3;
   AGIFilePosType tempPos;
   fpos_t pos;

   if ((fp = fopen(fName, "rb")) == NULL) {
      printf("Could not find file : %s.\n", fName);
      printf("Make sure you are in an AGI version 2 game directory.\n");
      exit(0);
   }

   while (!feof(fp)) {
      byte1 = fgetc(fp);
      byte2 = fgetc(fp);
      byte3 = fgetc(fp);

      fgetpos(fp, &pos);

      tempPos.fileName = (char *)malloc(10);
      sprintf(tempPos.fileName, "VOL.%d", ((byte1 & 0xF0) >> 4));
      tempPos.filePos = ((long)((byte1 & 0x0F) << 16) +
                         (long)((byte2 & 0xFF) << 8) +
                         (long)(byte3 & 0xFF));

      switch (dirNum) {
         case 0: logdir[*count] = tempPos; break;
         case 1: picdir[*count] = tempPos; break;
         case 2: viewdir[*count] = tempPos; break;
         case 3: snddir[*count] = tempPos; break;
      }
      (*count)++;
   }

   fclose(fp);
}

/***************************************************************************
** loadAGIv3Dir
***************************************************************************/
void loadAGIv3Dir()
{
   FILE *dirFile;
   unsigned char dirName[15], *marker, *dirData, *endPos, *dataPos;
   int resType=0, resNum=0, dirLength;
   AGIFilePosType tempPos;

   sprintf(dirName, "%sDIR", gameSig);
   if ((dirFile = fopen(dirName, "rb")) == NULL) {
      printf("File not found : %s\n", dirName);
      exit(1);
   }

   fseek(dirFile, 0, SEEK_END);
   dirLength = ftell(dirFile);
   fseek(dirFile, 0, SEEK_SET);
   dirData = (char *)malloc(sizeof(char)*dirLength);
   fread(dirData, sizeof(char), dirLength, dirFile);
   fclose(dirFile);
   marker = dirData;

   for (resType=0, marker=dirData; resType<4; resType++, marker+=2) {
      dataPos = dirData + (*marker + *(marker+1)*256);
      endPos = ((resType<3)? (dirData + (*(marker+2) + *(marker+3)*256))
         :(dirData+dirLength));
      resNum = 0;
      for (; dataPos<endPos; dataPos+=3, resNum++) {
         tempPos.fileName = (char *)malloc(10);
         sprintf(tempPos.fileName, "%sVOL.%d", gameSig,
            ((dataPos[0] & 0xF0) >> 4));
         tempPos.filePos = ((long)((dataPos[0] & 0x0F) << 16) +
                            (long)((dataPos[1] & 0xFF) << 8) +
                            (long)(dataPos[2] & 0xFF));

         switch (resType) {
            case 0: logdir[resNum] = tempPos; break;
            case 1: picdir[resNum] = tempPos; break;
            case 2: viewdir[resNum] = tempPos; break;
            case 3: snddir[resNum] = tempPos; break;
         }
      }
      if (resNum > 256) {
         printf("Error loading directory file.\n");
         printf("Too many resources.\n");
         exit(1);
      }

      switch (resType) {
         case 0: numLogics = resNum; break;
         case 1: numPictures = resNum; break;
         case 2: numViews = resNum; break;
         case 3: numSounds = resNum; break;
      }
   }

   free(dirData);
}

/***************************************************************************
** loadAGIDirs
**
** Purpose: To read the AGI directory files LOGDIR, PICDIR, VIEWDIR, and
** SNDDIR and store the information in a usable format. This function must
** be called once at the start of the interpreter.
***************************************************************************/
void loadAGIDirs()
{
   numLogics = numPictures = numViews = numSounds = 0;
   if (version3) {
      loadAGIv3Dir();
   }
   else {
      loadAGIDir(0, "LOGDIR", &numLogics);
      loadAGIDir(1, "PICDIR", &numPictures);
      loadAGIDir(2, "VIEWDIR", &numViews);
      loadAGIDir(3, "SNDDIR", &numSounds);
   }
}

#define  NORMAL     0
#define  ALTERNATE  1

/**************************************************************************
** convertPic
**
** Purpose: To convert an AGIv3 resource to the AGIv2 format.
**************************************************************************/
void convertPic(unsigned char *input, unsigned char *output, int dataLen)
{
   unsigned char data, oldData, outData;
   int mode = NORMAL, i=0;

   while (i++ < dataLen) {
      data = *input++;

      if (mode == ALTERNATE)
	      outData = ((data & 0xF0) >> 4) + ((oldData & 0x0F) << 4);
      else
	      outData = data;

      if ((outData == 0xF0) || (outData == 0xF2)) {
         *output++ = outData;
	      if (mode == NORMAL) {
            data = *input++;
            *output++ = ((data & 0xF0) >> 4);
	         mode = ALTERNATE;
	      }
	      else {
            *output++ = (data & 0x0F);
	         mode = NORMAL;
	      }
      }
      else
         *output++ = outData;

      oldData = data;
   }
}

/**************************************************************************
** loadAGIFile
**
** Purpose: To read an AGI data file out of a VOL file and store the data
** and data size into the AGIFile structure whose pointer is passed in.
** This function handles AGIv2 and AGIv3. It makes sure that the resources
** are converted to a common format to deal with the differences between
** the two versions. The conversions needed are as follows:
**
**  - All encrypted LOGIC message sections are decrypted. This includes
**    uncompressed AGIv3 LOGICs as well as AGIv2 LOGICs.
**  - AGIv3 PICTUREs are decompressed.
**
** In both cases the format that is easier to deal with is returned.
**************************************************************************/
void loadAGIFile(int resType, AGIFilePosType location, AGIFile *AGIData)
{
   FILE *fp;
   short int sig, compSize, startPos, endPos, numMess, avisPos=0, i;
   unsigned char byte1, byte2, volNum, *compBuf, *fileData;

   if (location.filePos == EMPTY) {
      printf("Could not find requested AGI file.\n");
      printf("This could indicate problems with your game data files\n");
      printf("or there may be something wrong with MEKA.\n");
      exit(0);
   }

   if ((fp = fopen(location.fileName, "rb")) == NULL) {
      printf("Could not find file : %s.\n", location.fileName);
      printf("Make sure you are in an AGI version 2 game directory.\n");
      exit(0);
   }

   fseek(fp, location.filePos, SEEK_SET);
   fread(&sig, 2, 1, fp);
   if (sig != 0x3412) {  /* All AGI data files start with 0x1234 */
      printf("Data error reading %s.\n", location.fileName);
      printf("The requested AGI file did not have a signature.\n");
      printf("Check if your game files are corrupt.\n");
      exit(0);
   }
   volNum = fgetc(fp);
   byte1 = fgetc(fp);
   byte2 = fgetc(fp);
   AGIData->size = (unsigned int)(byte1) + (unsigned int)(byte2 << 8);
   AGIData->data = (char *)malloc(AGIData->size);

   if (version3) {
      byte1 = fgetc(fp);
      byte2 = fgetc(fp);
      compSize = (unsigned int)(byte1) + (unsigned int)(byte2 << 8);
      compBuf = (unsigned char *)malloc((compSize+10)*sizeof(char));
      fread(compBuf, sizeof(char), compSize, fp);

      //initLZW();

      if (volNum & 0x80) {
         convertPic(compBuf, AGIData->data, compSize);
      }
      else if (AGIData->size == compSize) {  /* Not compressed */
         memcpy(AGIData->data, compBuf, compSize);

         if (resType == LOGIC) {
            /* Uncompressed AGIv3 logic files have their message sections
               encrypted, so we decrypt it here */
            fileData = AGIData->data;
            startPos = *fileData + (*(fileData+1))*256 + 2;
            numMess = fileData[startPos];
            endPos = fileData[startPos+1] + fileData[startPos+2]*256;
            fileData += (startPos + 3);
            startPos = (numMess * 2) + 0;

            for (i=startPos; i<endPos; i++)
	            fileData[i] ^= AVIS_DURGAN[avisPos++ % 11];
         }

         free(compBuf);
      }
      else {
         expand(compBuf, AGIData->data, AGIData->size);
      }

      free(compBuf);
      //closeLZW();
   }
   else {
      fread(AGIData->data, AGIData->size, 1, fp);
      if (resType == LOGIC) {
         /* Decrypt message section */
         fileData = AGIData->data;
         startPos = *fileData + (*(fileData+1))*256 + 2;
         numMess = fileData[startPos];
         endPos = fileData[startPos+1] + fileData[startPos+2]*256;
         fileData += (startPos + 3);
         startPos = (numMess * 2) + 0;

         for (i=startPos; i<endPos; i++)
	         fileData[i] ^= AVIS_DURGAN[avisPos++ % 11];
      }
   }

   fclose(fp);
}
