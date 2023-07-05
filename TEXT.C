/***************************************************************************
** text.c
**
** This module contains routines to handle the output of text in both the
** graphics and text AGI modes.
**
** (c) Lance Ewing, 1998.   - Original code (9 Jan 1998)
***************************************************************************/

#include <allegro.h>
#include <sys\movedata.h>
#include <string.h>
#include <conio.h>
#include <dos.h>

#include "general.h"
#include "picture.h"

void drawChar(BITMAP *scn, byte charNum, int x, int y, int foreColour, int backColour)
{
   unsigned short fontSegment, fontOffset;
   int fontByte, byteNum, bytePos;
   union REGS r;

   r.x.ax = 0x1123;
   r.h.bl = 0x02;
   int86(0x10, &r, &r);

   dosmemget(0x43*4+2, 2, &fontSegment);
   dosmemget(0x43*4, 2, &fontOffset);

   for (byteNum=(charNum*8); byteNum<=(charNum*8+7); byteNum++) {
      dosmemget(fontSegment*16+fontOffset+byteNum, 1, &fontByte);
      for (bytePos=7; bytePos>=0; bytePos--) {
         if (fontByte & (1 << bytePos))
            scn->line[y+(byteNum-(charNum*8))][x+(7-bytePos)] = foreColour;
         else
            scn->line[y+(byteNum-(charNum*8))][x+(7-bytePos)] = backColour;
      }
   }
}

void drawBigChar(BITMAP *scn, byte charNum, int x, int y, int foreColour, int backColour)
{
   unsigned short fontSegment, fontOffset;
   byte fontByte;
   unsigned short byteNum;
   int bytePos;
   union REGS r;

   r.x.ax = 0x1123;
   r.h.bl = 0x02;
   int86(0x10, &r, &r);

   dosmemget(0x43*4+2, 2, &fontSegment);
   dosmemget(0x43*4, 2, &fontOffset);

   for (byteNum=(charNum*8); byteNum<=(charNum*8+7); byteNum++) {
      dosmemget(fontSegment*16+fontOffset+byteNum, 1, &fontByte);
      for (bytePos=7; bytePos>=0; bytePos--) {
         if (fontByte & (1 << bytePos)) {
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-(bytePos*2))] = foreColour;
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-((bytePos*2)+1))] = foreColour;
         }
         else {
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-(bytePos*2))] = backColour;
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-((bytePos*2)+1))] = backColour;
         }
      }
      for (bytePos=7; bytePos>=0; bytePos--) {
         if (fontByte & (1 << bytePos)) {
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-(bytePos*2))] = foreColour;
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-((bytePos*2)+1))] = foreColour;
         }
         else {
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-(bytePos*2))] = backColour;
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-((bytePos*2)+1))] = backColour;
         }
      }
   }
}

void drawString(BITMAP *scn, char *data, int x, int y, int foreColour, int backColour)
{
   int charPos;

   for (charPos=0; charPos<strlen(data); charPos++)
      drawChar(scn, data[charPos], x+(charPos*8), y, foreColour, backColour);
}

void drawBigString(BITMAP *scn, char *data, int x, int y, int foreColour, int backColour)
{
   int charPos, dataLen;
   BITMAP *temp = create_bitmap(640, 16);

   clear_to_color(temp, 12);
   dataLen = strlen(data);
   for (charPos=0; charPos<dataLen; charPos++)
      drawBigChar(temp, data[charPos], (charPos*16), 0, foreColour, backColour);

   show_mouse(NULL);
   blit(temp, scn, 0, 0, x, y, (dataLen<<4), 16);
   show_mouse(screen);
   destroy_bitmap(temp);
}

void printInBox(char *theString)
{
   BITMAP *temp = create_bitmap(640, 336);
   char tempString[256];
   int counter=0, maxLen=0, lineNum=0, i, boxWidth, boxHeight;

   gui_bg_color = 7;
   clear_to_color(temp, 8);
   *tempString = 0;

   for (i=0; i<strlen(theString); i++) {
      if (((counter++ > 50) && (theString[i] == ' ')) || (theString[i] == 0x0A)) {
         drawString(temp, tempString, 8, (lineNum+1)*8, 1, 8);
         *tempString = 0;
         lineNum++;
         if (counter > maxLen) maxLen = counter;
	      counter = 0;
      }
      else
         sprintf(tempString, "%s%c", tempString, theString[i]);
   }

   if (counter > maxLen) maxLen = counter;
   drawString(temp, tempString, 8, (lineNum+1)*8, 1, 8);
   boxWidth = (maxLen+2)*8;
   boxHeight = (lineNum+3)*8;
   rect(temp, 3, 3, boxWidth-4, boxHeight-4, 15);
   show_mouse(NULL);
   blit(temp, agi_screen, 0, 0, (640-boxWidth)/2, (336-boxHeight)/2, boxWidth, boxHeight);
   show_mouse(screen);
   destroy_bitmap(temp);
}

void printInBoxBig2(char *theString)
{
   BITMAP *temp = create_bitmap(640, 336);
   char tempString[256];
   int counter=0, maxLen=0, lineNum=0, i, boxWidth, boxHeight;

   gui_bg_color = 7;
   clear_to_color(temp, 8);
   *tempString = 0;

   for (i=0; i<strlen(theString); i++) {
      if (((counter++ > 22) && (theString[i] == ' ')) || (theString[i] == 0x0A)) {
         drawBigString(temp, tempString, 16, (lineNum+1)*16, 1, 8);
         *tempString = 0;
         lineNum++;
         if (counter > maxLen) maxLen = counter;
	      counter = 0;
      }
      else
         sprintf(tempString, "%s%c", tempString, theString[i]);
   }

   if (counter > maxLen) maxLen = counter;
   drawBigString(temp, tempString, 16, (lineNum+1)*16, 1, 8);
   boxWidth = (maxLen+2)*16;
   boxHeight = (lineNum+3)*16;
   rect(temp, 3, 3, boxWidth-4, boxHeight-4, 16);
   show_mouse(NULL);
   blit(temp, agi_screen, 0, 0, (640-boxWidth)/2, (336-boxHeight)/2, boxWidth, boxHeight);
   show_mouse(screen);
   destroy_bitmap(temp);
}

void wrapWord(char *s, int *pos)
{
   int i;

   for (i=strlen(s)-1; i>=0; i--) {
      if (s[i] == ' ') {
         *pos -= (strlen(s) - i);
         s[i] = 0;
         return;
      }
   }
}

/***************************************************************************
** printInBoxBig
**
** Purpose: To display the given message in a box. This version is given
** a max line length parameter and starting x and y parameters. It uses
** word wrap to make sure that the max line length is never exceeded.
***************************************************************************/
void printInBoxBig(char *theString, int x, int y, int lineLen)
{
   BITMAP *temp = create_bitmap(640, 336);
   char tempString[256];
   int counter=0, lineNum=0, i, boxWidth, boxHeight, maxLen=0;

   clear_to_color(temp, 8);
   *tempString = 0;

   for (i=0; i<strlen(theString); i++) {
      if (theString[i] == 0x0A) {
         drawBigString(temp, tempString, 16, (lineNum+1)*16, 1, 8);
         if (strlen(tempString) > maxLen) maxLen = strlen(tempString);
         *tempString = 0;
         lineNum++;
	      counter = 0;
      }
      else if (counter++ >= lineLen) {  /* Word wrap */
         wrapWord(tempString, &i);
         drawBigString(temp, tempString, 16, (lineNum+1)*16, 1, 8);
         if (strlen(tempString) > maxLen) maxLen = strlen(tempString);
         *tempString = 0;
         lineNum++;
	      counter = 0;
      }
      else
         sprintf(tempString, "%s%c", tempString, theString[i]);
   }

   if (strlen(tempString) > maxLen) maxLen = strlen(tempString);
   drawBigString(temp, tempString, 16, (lineNum+1)*16, 1, 8);
   boxWidth = (maxLen+2)*16;
   boxHeight = (lineNum+3)*16;
   rect(temp, 3, 3, boxWidth-4, boxHeight-4, 16);
   show_mouse(NULL);
   blit(temp, agi_screen, 0, 0, (640-boxWidth)/2, (336-boxHeight)/2, boxWidth, boxHeight);
   show_mouse(screen);
   destroy_bitmap(temp);
}

void clearTextScreen()
{

}

void test()
{
   BITMAP *picture;

   allegro_init();
   picture = create_bitmap(320, 200);
   clear(picture);
   set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0);
   //drawChar(picture, 'A', 0, 0, 4, 3);
   drawString(picture, "The String", 0, 0, 4, 3);
   picture->line[0][8] = 15;
   picture->line[8][0] = 15;
   blit(picture, screen, 0, 0, 0, 0, 320, 200);
   getch();
   allegro_exit();
   textmode(C80);
}
