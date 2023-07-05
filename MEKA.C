/***************************************************************************
** meka.c
**
** Working name for an AGI interpreter based on the work done by J.Moller,
** L.Ewing, and P.Kelly, thus the name M.E.K.A. (A is Adventure). These
** three formed the AGI group and did the bulk of the initial investigation
** but we have since been grateful to the others who have joined the team
** in a practical way.
**
** (c) Lance Ewing, 1998.
***************************************************************************/

#include <allegro.h>
#include <unistd.h>

#include "general.h"
#include "agifiles.h"
#include "logic.h"
#include "view.h"
#include "object.h"
#include "words.h"
#include "picture.h"
#include "parser.h"
#include "sound.h"

boolean stillRunning = TRUE, hasEnteredNewRoom=FALSE, exitAllLogics=FALSE;
byte var[256];
boolean flag[256];
char string[12][40];
byte horizon;

#define  PLAYER_CONTROL   0
#define  PROGRAM_CONTROL  1

volatile int counter;              /* Used for timer control */
volatile int hund;                 /* Used for interpreters clock */

int controlMode=PLAYER_CONTROL;    /* player.control or program.control */
int dirnOfEgo, newRoomNum, score;

extern int picFNum;    // Debugging. Delete at some stage!!

void adjustEgoPosition()
{
   switch (var[2]) {
      case 1:
         viewtab[0].yPos = 167;
         break;
      case 2:
         viewtab[0].xPos = 0;
         break;
      case 3:
         viewtab[0].yPos = 37;  /* Note: This is default horizon + 1 */
         break;
      case 4:
         viewtab[0].xPos = 160 - (viewtab[0].xsize);
         break;
   }

   /* Might need to stop motion of ego */
}

void discardResources()
{
   int i;

   for (i=1; i<256; i++) discardLogicFile(i);
   for (i=0; i<256; i++) discardView(i);
   for (i=0; i<256; i++) discardPictureFile(i);
   for (i=0; i<256; i++) discardSoundFile(i);
}

/***************************************************************************
** new_room
**
** This performs the necessary actions for the AGI new_room command. For
** some reason this needs to be executed right at the end of the AGI cycle
** and not when it is first encountered. I have included new_room in the
** main module for this reason and also because it is one of the most
** important of the AGI commands.
***************************************************************************/
void newRoom()
{
   resetViews();
   //stop_update_all();
   //unanimate_all();
   discardResources();
   controlMode = PLAYER_CONTROL;
   //unblock();
   horizon = 36;
   var[1] = var[0];
   var[0] = newRoomNum;
   var[4] = 0;
   var[5] = 0;
   var[9] = 0;
   var[16] = 0;
   adjustEgoPosition();
   var[2] = 0;
   flag[2] = 0;
   flag[5] = 1;
   score = var[3];

   memset(directions, 0, 9);
   /* rectfill(screen, 0, 20+(22*16), 639, 463, 0); */   /* Clear screen */
   clear(screen);
}

/***************************************************************************
** updateStatusLine
**
** The status line shows the score and sound at the top of the screen.
***************************************************************************/
void updateStatusLine()
{
   char scoreStr[256], soundStr[256];

   if (statusLineDisplayed) {
      sprintf(scoreStr, "Score: %d of %d", var[3], var[7]);
      sprintf(soundStr,  "Sound:%-3s", (flag[9]? "on" : "off"));
      drawBigString(screen, scoreStr, 16, 0, 8, 1);
      drawBigString(screen, soundStr, 496, 0, 8, 1);
   } else {
      rectfill(screen, 0, 0, 639, 15, 0);   /* Clear status line */
   }
}

/***************************************************************************
** interpret
**
** The main routine that gets called everytime the timing procedure is
** activated.
***************************************************************************/
void interpret()
{
   flag[2] = FALSE;   /* The player has issued a command line */
   flag[4] = FALSE;   /* The 'said' command has accepted the input */
   pollKeyboard();
   //if (controlMode == PROGRAM_CONTROL)
   //   dirnOfEgo = var[6];
   //else
   //   var[6] = dirnOfEgo;
   viewtab[0].direction = var[6];
   calcObjMotion();
   // <<-- Update status line here (score & sound)
   updateStatusLine();

   do {
      hasEnteredNewRoom = FALSE;
      exitAllLogics = FALSE;
      executeLogic(0);
      //dirnOfEgo = var[6];
      viewtab[0].direction = var[6];
      // <<-- Update status line here (score & sound)
      updateStatusLine();
      var[5] = 0;
      var[4] = 0;
      flag[5] = 0;
      flag[6] = FALSE;
      flag[12] = FALSE;
      if (!hasEnteredNewRoom) updateObjects();
      if (hasEnteredNewRoom) newRoom();
   } while (hasEnteredNewRoom);
}

void timing_proc()
{
   counter++;
   hund += 5;
   if (hund >= 100) { /* One second has passed */
      var[11]++;
      if (var[11] >= 60) {  /* One minute has passed */
         var[12]++;
         if (var[12] >= 60) { /* One hour has passed */
            var[13]++;
            if (var[13] >= 24) { /* One day has passed */
               var[14]++;
               var[13] = 0;
            }
            var[12] = 0;
         }
         var[11] = 0;
      }
      hund = 0;
   }
}
END_OF_FUNCTION(timing_proc)

void initialise()
{
   int i;

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();
   initFiles();             /* Load resource directories */
   // <<--  Determine exact version in here
   for (i=0; i<255; i++) {  /* Initialize variables and flags */
      var[i] = 0;
      flag[i] = FALSE;
   }
   flag[5] = TRUE;
   var[24] = 0x29;
   var[22] = 3;
   var[26] = 3;
   var[8] = 255;     /* Number of free 256 byte pages of memory */
   var[10] = 2;      /* Normal speed */

   /* SQ2 patch. I don't know where these are set in the game. */
   /* var[86] = 1; var[87] = 2; var[88] = 3; */

   initAGIScreen();
   initPalette();
   initLogics();
   initPicture();
   initPictures();
   initSound();
   initViews();
   initObjects();
   loadObjectFile();
   loadWords();
   initEvents();
   horizon = 36;

   /* Set up timer. The timer controls the interpreter speed. */
   counter = 0;
   LOCK_VARIABLE(counter);
   LOCK_FUNCTION(timing_proc);
   install_int_ex(timing_proc, MSEC_TO_TIMER(50));
}

void closedown()
{
   freeMenuItems();
   discardObjects();
   discardWords();
   closePicture();
   allegro_exit();
   exit(0);
}

void main()
{
   int ret, oldCount=0;

   //chdir("..\\KQ1-2917");
   //chdir("..\\COMPILER\\NEW\\SAMPLE\\TEMPLATE");
   //chdir("\\GAMES\\SIERRA\\MH2");
   //chdir("\\GAMES\\SIERRA\\MH1");
   chdir("\\GAMES\\SIERRA\\SQ2");
   //chdir("\\GAMES\\SIERRA\\LSL1");
   //chdir("..\\KQ2-2917");

   initialise();
   while (TRUE) {
      /* Cycle initiator. Controlled by delay variable (var[10). */
      if (counter >= var[10]) {
        interpret();
        counter=0;
      }
   }

   chdir("\\HACK\\AGI\\D\\AGI\\MEKA");
   closedown();
}

void main2()
{
   AGIFile AGIData;
   BITMAP *temp = create_bitmap(640, 32);
   char string1[80], string2[80];

   allegro_init();
   install_keyboard();
   initFiles();
   //loadAGIFile(PICTURE, picdir[129], &AGIData);
   loadPictureFile(3);
   initPicture();
   //drawPic(AGIData.data, AGIData.size, TRUE);
   initAGIScreen();
   initPalette();
   install_timer();
   initSound();
   picFNum = 3;
   drawPic(loadedPictures[3].data, loadedPictures[3].size, TRUE);
   loadViewFile(0);
   addToPic(0, 0, 0, 55, 50, 7, 0);
   discardView(0);
   showPic();

   //drawBigChar(temp, 'A', 0, 0, 7, 15);
   //drawBigString(temp, "This is a test", 0, 0, 7, 15);
   //blit(temp, agi_screen, 0, 0, 0, 0, 16*14, 16);
   //getch();
   remove_keyboard();
   printInBoxBig("The quick brown fox jumps over the lazy dog.", -1, -1, 30);
   loadSoundFile(1);
   playSound(1);
   getch();
   closePicture();
   allegro_exit();
   //strcpy(string1, "Variable 1: %v1|2 %%");
   //processString(string1, string2);
}
