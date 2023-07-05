/***************************************************************************
** graphics.c
**
** Contains some extra graphics routines that I couldn't find in the
** allegro library.
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
         if (fontByte & (1 << bytePos)) {
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-(bytePos*2))] = foreColour;
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-((bytePos*2)-1))] = foreColour;
         }
         else {
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-(bytePos*2))] = backColour;
            scn->line[y+(byteNum-(charNum*8))*2][x+(15-((bytePos*2)-1))] = backColour;
         }
      }
      for (bytePos=7; bytePos>=0; bytePos--) {
         if (fontByte & (1 << bytePos)) {
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-(bytePos*2))] = foreColour;
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-((bytePos*2)-1))] = foreColour;
         }
         else {
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-(bytePos*2))] = backColour;
            scn->line[y+(byteNum-(charNum*8))*2+1][x+(15-((bytePos*2)-1))] = backColour;
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
   int charPos;

   for (charPos=0; charPos<strlen(data); charPos++)
      drawBigChar(scn, data[charPos], x+(charPos*16), y, foreColour, backColour);
}

void printInBox(char *theString)
{
   BITMAP *temp = create_bitmap(640, 336);
   char tempString[256];
   int counter=0, maxLen=0, lineNum=0, i, boxWidth, boxHeight;

   gui_bg_color = 7;
   clear_to_color(temp, 7);
   *tempString = 0;

   for (i=0; i<strlen(theString); i++) {
      if (((counter++ > 50) && (theString[i] == ' ')) || (theString[i] == 0x0A)) {
         drawString(temp, tempString, 8, (lineNum+1)*8, 0, 7);
         *tempString = 0;
         lineNum++;
         if (counter > maxLen) maxLen = counter;
	      counter = 0;
      }
      else
         sprintf(tempString, "%s%c", tempString, theString[i]);
   }

   if (counter > maxLen) maxLen = counter;
   drawString(temp, tempString, 8, (lineNum+1)*8, 0, 7);
   boxWidth = (maxLen+1)*8;
   boxHeight = (lineNum+3)*8;
   rect(temp, 3, 3, boxWidth-4, boxHeight-4, 15);
   blit(temp, agi_screen, 0, 0, (640-boxWidth)/2, (336-boxHeight)/2, boxWidth, boxHeight);
   destroy_bitmap(temp);
   getch();
   showPicture();
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
