/***************************************************************************
** parser.c
**
** These functions are related to the 'said' parser.
**
** (c) 1997 Lance Ewing - Initial code (27 Aug 97)
***************************************************************************/

#include <pc.h>
#include <string.h>
#include <stdio.h>
#include <allegro.h>

#include "general.h"
#include "words.h"
#include "view.h"
#include "picture.h"
#include "parser.h"

#define  PLAYER_CONTROL   0
#define  PROGRAM_CONTROL  1

extern boolean flag[];
extern byte var[];
extern char string[12][40];
extern int dirnOfEgo;
extern int controlMode;

int numInputWords, inputWords[10];
char wordText[10][80];
//boolean wordsAreWaiting=FALSE;

byte keyState[256], asciiState[256];
boolean haveKey=FALSE;
char cursorChar='_';
char lastLine[80];
eventType events[256];  /* controller(), set.key(), set.menu.item() */
byte directions[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int lastKey;

void lookupWords(char *inputLine);

void initEvents()
{
   int eventNum;

   for (eventNum=0; eventNum<256; eventNum++) {
      events[eventNum].activated = FALSE;
      events[eventNum].type = NO_EVENT;
   }
}

void handleDirection(int dirn)
{
   if (viewtab[0].flags & MOTION) {
      if (directions[dirn] == 0) {
         memset(directions, 0, 9);
         directions[dirn] = 1;
         viewtab[0].direction = dirn;
         //viewtab[0].flags |= MOTION;
         var[6] = dirnOfEgo = dirn;
      }
      else {
         memset(directions, 0, 9);
         viewtab[0].direction = 0;
         //viewtab[0].flags &= ~MOTION;
         var[6] = dirnOfEgo = 0;
      }
   }
}

/* used by get.string and get.num */
void getString(char *promptStr, char *returnStr, int x, int y, int l)
{
   char currentInputStr[80]="", strPos=0, outputString[80], temp[256];
   int ch, gx, gy;
   boolean stillInputing = TRUE;

   gx = (x-1) * 16;
   gy = ((y-1) * 16) + 20;
   processString(promptStr, temp);
   sprintf(outputString, "%s%s%c", temp, currentInputStr, cursorChar);
   drawBigString(screen, outputString, gx, gy, 8, 1);

   while (stillInputing) {
      ch = readkey();
      if ((ch >> 8) == 0x1C) ch |= 0x0D; /* Handle keypad ENTER */
      switch (ch & 0xff) {
         case 0:     /* Ignore these when building input string */
         case 0x09:
         case 0x1B:
            break;
         case 0x0D:  /* ENTER */
            strcpy(returnStr, currentInputStr);
            drawBigString(screen, "                                       ", gx, gy, 7, 0);
            while (key[KEY_ENTER]) { /* Wait until ENTER released */ }
            return;
         case 0x08: /* Backspace */
            if (strPos > 0)  {
               strPos--;
               currentInputStr[strPos] = 0;
               sprintf(outputString, "%s%s%c ", temp, currentInputStr, cursorChar);
               drawBigString(screen, outputString, gx, gy, 8, 1);
            }
            break;
         case 0x0A: break;
         default:
            if (strPos == l) break;
            currentInputStr[strPos] = (ch & 0xff);
            strPos++;
            currentInputStr[strPos] = 0;
            sprintf(outputString, "%s%s%c", temp, currentInputStr, cursorChar);
            drawBigString(screen, outputString, gx, gy, 8, 1);
            break;
      }
   }
}

/***************************************************************************
** pollKeyboard
***************************************************************************/
void pollKeyboard()
{
   static char currentInputStr[80]="", strPos=0;
   char outputString[80], temp[256];
   int ch, dummy, gx, gy;

   char tempStr[256];

   /* Clear keyboard buffers */
   haveKey = FALSE;
   memset(keyState, 0, 256);
   memset(asciiState, 0, 256);
   processString(string[0], temp);
   gx = 0;
   gy = ((user_input_line-1) * 16) + 20;
   if (inputLineDisplayed) {
      sprintf(outputString, "%s%s%c", temp, currentInputStr, cursorChar);
      drawBigString(screen, outputString, gx, gy, 8, 1);
   }
   else {
      drawBigString(screen, "                                       ", gx, gy, 7, 0);
   }

   while (keypressed()) {
      haveKey = TRUE;
      ch = readkey();
      lastKey = (ch >> 8);  /* Store key value for have.key() cmd */
      if ((ch >> 8) == 0x1C) ch |= 0x0D; /* Handle keypad ENTER */
      keyState[ch >> 8] = 1;     /* Mark scancode as activated */
      /* if ((ch & 0x00) != 0x00) asciiState[ch & 0xff] = 1; */
      asciiState[ch & 0xff] = 1;

      if ((ch >> 8) == KEY_F11) saveSnapShot();

      /* Handle arrow keys */
      if (controlMode == PLAYER_CONTROL) {
         switch (ch >> 8) {
            case KEY_UP: handleDirection(1); return;
            case KEY_PGUP: handleDirection(2); return;
            case KEY_RIGHT: handleDirection(3); return;
            case KEY_PGDN: handleDirection(4); return;
            case KEY_DOWN: handleDirection(5); return;
            case KEY_END: handleDirection(6); return;
            case KEY_LEFT: handleDirection(7); return;
            case KEY_HOME: handleDirection(8); return;
         }
      }

      if (inputLineDisplayed) {
         switch (ch & 0xff) {
            case 0:
            case 3:
               return;
            case 0x09:  /* Ignore these when building input string */
            case 0x1B:
               /* closedown(); */
               break;
            case 0x0D:  /* ENTER */
               lookupWords(currentInputStr);
               currentInputStr[0] = 0;
               strPos = 0;
               drawBigString(screen, "                                       ", gx, gy, 7, 0);
               sprintf(outputString, "%s%s%c", temp, currentInputStr, cursorChar);
               drawBigString(screen, outputString, gx, gy, 8, 1);
               while (key[KEY_ENTER]) { /* Wait until ENTER released */ }
               strcpy(lastLine, currentInputStr);
               break;
            case 0x08:
               if (ch == 0x0E08) {   /* Backspace */
                  if (strPos > 0)  {
                     strPos--;
                     currentInputStr[strPos] = 0;
                     sprintf(outputString, "%s%s%c ", temp, currentInputStr, cursorChar);
                     drawBigString(screen, outputString, gx, gy, 8, 1);
                     //drawBigString(screen, " ", (strPos*16), 448, 7, 0);
                  }
               }
               else
                  return;
               break;
            case 0x0A: break;
            default:
               currentInputStr[strPos] = (ch & 0xff);
               strPos++;
               currentInputStr[strPos] = 0;
               sprintf(outputString, "%s%s%c", temp, currentInputStr, cursorChar);
               drawBigString(screen, outputString, gx, gy, 8, 1);
               break;
         }
      }
      else {
         drawBigString(screen, "                                       ", gx, gy, 7, 0);
      }
   }
}

/***************************************************************************
** stripExtraChars
**
** Purpose: This function strips out all the punctuation and unneeded
** spaces from the user input, and converts all the characters to
** lowercase.
***************************************************************************/
void stripExtraChars(char *userInput)
{
   char tempString[41]="";
   int strPos, tempPos=0;
   boolean lastCharSpace = FALSE;

   for (strPos=0; strPos<strlen(userInput); strPos++) {
      switch (userInput[strPos]) {
         case ' ':
            if (!lastCharSpace) tempString[tempPos++] = ' ';
            lastCharSpace = TRUE;
            break;
         case ',':
         case '.':
         case '?':
         case '!':
         case '(':
         case ')':
         case ';':
         case ':':
         case '[':
         case ']':
         case '{':
         case '}':
         case '\'':
         case '`':
         case '-':
         case '"':
            /* Ignore */
            break;
         default:
            lastCharSpace = FALSE;
            tempString[tempPos++] = userInput[strPos];
            break;
      }
   }

   tempString[tempPos] = 0;
   strcpy(userInput, tempString);
}

/***************************************************************************
** lookupWords
**
** Purpose: To convert the users input string into a list of synonym
** numbers as used in WORDS.TOK. This functions is called when the ENTER
** key is hit in the normal flow of game play (as opposed to using the menu,
** browsing the inventory, etc).
***************************************************************************/
void lookupWords(char *inputLine)
{
   char *token, userInput[41];
   word synNum;
   boolean allWordsFound = TRUE;

   strcpy(userInput, inputLine);
   stripExtraChars(userInput);
   numInputWords = 0;

   //while (allWordsFound && ((token = strtok(userInput, " ")) != NULL)) {
   for (token = strtok(userInput, " "); (token && allWordsFound); token=strtok(0, " ")) {
      switch (synNum = findSynonymNum(token)) {
         case -1: /* Word not found */
            var[9] = numInputWords;
            allWordsFound = FALSE;
            break;
         case 0:  /* Ignore words with synonym number zero */
            break;
         default:
            inputWords[numInputWords] = synNum;
            strcpy(wordText[numInputWords], token);
            numInputWords++;
            break;
      }
   }

   if (allWordsFound && (numInputWords > 0)) {
      flag[2] = TRUE;  /* The user has entered an input line */
      flag[4] = FALSE; /* Said command has not yet accepted the line */
   }
}

/***************************************************************************
** said
**
** Purpose: To execute the said() test command. Each two-byte argument is
** compared with the relevant words entered by the user. If there is an
** exact match, it returns TRUE, otherwise it returns false.
***************************************************************************/
boolean said(byte **data)
{
   int numOfArgs, wordNum, argValue;
   boolean wordsMatch = TRUE;
   byte argLo, argHi;

   numOfArgs = *(*data)++;

   if ((flag[2] == 0) || (flag[4] == 1)) {  /* Not valid input waiting */
      *data += (numOfArgs*2); /* Jump over arguments */
      return FALSE;
   }

   /* Needs to deal with ANYWORD and ROL */
   for (wordNum=0; wordNum < numOfArgs; wordNum++) {
      argLo = *(*data)++;
      argHi = *(*data)++;
      argValue = (argLo + (argHi << 8));
      if (argValue == 9999) break; /* Should always be last argument */
      if (argValue == 1) continue; /* Word comparison does not matter */
      if (inputWords[wordNum] != argValue) wordsMatch = FALSE;
   }

   if ((numInputWords != numOfArgs) && (argValue != 9999)) return FALSE;

   if (wordsMatch)  {
      flag[4] = TRUE;    /* said() accepted input */
      numInputWords = 0;
      flag[2] = FALSE;   /* not sure about this one */
   }

   return (wordsMatch);
}

