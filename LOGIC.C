/**************************************************************************
** LOGIC.C
**
** These functions are used to load LOGIC files into a structure of type
** LOGICFile. It uses the general function LoadAGIFile to load the data
** from the VOL file itself.
**
** (c) 1997 Lance Ewing - Original code (2 July 97)
**                        Changed (26 Aug 97)
**************************************************************************/

#include <string.h>

#include "agifiles.h"
#include "general.h"
#include "logic.h"

/* The logics array is the array that holds all the information about the
** logic files. A boolean flag determines whether the logic is loaded or
** not. If it isn't loaded, then the data is not in memory. */
LOGICEntry logics[256];

/***************************************************************************
** initLogics
**
** Purpose: To initialize the array that holds information on the loaded
** logic files and also to load LOGIC.0 into that array. All other logics
** are marked as unloaded. Make sure you call initFiles() before calling
** this function.
***************************************************************************/
void initLogics()
{
   int i;

   for (i=0; i<256; i++) {
      logics[i].loaded = FALSE;
      logics[i].entryPoint = 0;
      logics[i].currentPoint = 0;
      logics[i].data = NULL;
   }

   loadLogicFile(0);
}

/**************************************************************************
** loadMessages
**
** Purpose: Since message decryption is done when the LOGIC is loaded, this
** function simply reads the message section and stores the messages in an
** array.
**************************************************************************/
void loadMessages(byte *fileData, LOGICFile *logicData)
{
   word startPos, messNum;
   short int index;
   byte *marker;

   /* Calculate start and end indices of message data, and
   ** the number of messages in message data, and then allocate
   ** memory for the array of message strings. */
   startPos = fileData[0] + fileData[1]*256 + 2;
   logicData->numMessages = fileData[startPos];
   logicData->messages = (byte **)malloc(logicData->numMessages*sizeof(byte *));
   fileData += (startPos + 3);

   /* Step through the message data copying message strings to the
   ** logicData struct. */
   for (messNum=0, marker=fileData; messNum<logicData->numMessages; messNum++, marker+=2) {
      index = marker[0] + marker[1]*256 - 2;
      logicData->messages[messNum] = ((index<0)? strdup("")
         : strdup(&fileData[index]));
   }
}

/**************************************************************************
** loadLogicFile
**
** Purpose: To load a LOGIC file, decode the messages, and store in a
** suitable structure.
**************************************************************************/
void loadLogicFile(int logFileNum)
{
   AGIFile tempAGI;
   LOGICFile *logicData;
   int dummy;

   discardLogicFile(logFileNum);
   logics[logFileNum].data = (LOGICFile *)malloc(sizeof(LOGICFile));
   logicData = logics[logFileNum].data;

   /* Load LOGIC file, calculate logic code length, and copy
   ** logic code into tempLOGIC. */
   loadAGIFile(LOGIC, logdir[logFileNum], &tempAGI);
   logicData->codeSize = tempAGI.data[0] + tempAGI.data[1]*256;
   logicData->logicCode = (byte *)malloc(logicData->codeSize);
   memcpy(logicData->logicCode, &tempAGI.data[2], logicData->codeSize);

   /* Decode message section of LOGIC file and store in tempLOGIC. */
   loadMessages(tempAGI.data, logicData);

   logics[logFileNum].loaded = TRUE;
   free(tempAGI.data);   /* Deallocate original buffer. */
}

/**************************************************************************
** discardLogicFile
**
** Purpose: To deallocate all the memory associated with a LOGICFile
** struct. This function can only be used with dynamically allocated
** LOGICFile structs which is what I plan to use.
**************************************************************************/
void discardLogicFile(int logFileNum)
{
   int messNum;
   LOGICFile *logicData;

   if (logics[logFileNum].loaded) {
      logicData = logics[logFileNum].data;

      for (messNum=0; messNum<logicData->numMessages; messNum++)
         free(logicData->messages[messNum]);

      free(logicData->messages);
      free(logicData->logicCode);
      free(logicData);
      logics[logFileNum].loaded = FALSE;
   }
}

void testLogic()
{
   initFiles();
   initLogics();
   loadLogicFile(0);
   discardLogicFile(0);
}
