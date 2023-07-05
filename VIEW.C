/***************************************************************************
** VIEW.C
**
** These functions load VIEWs into memory. VIEWs are not automatically
** stored into the VIEW table when they are loaded. The VIEW table only
** holds those VIEWs that have been allocated a slot via the set.view()
** AGI function. The views that are stored in the VIEW table appear to
** be those that are going to be animated. Compare this with add-to-pics
** which are placed on the screen but are never dealt with again, or
** inventory item VIEWs that are shown but not controlled (animated).
**
** (c) 1997 Lance Ewing - Original code (3 July 97)
**                      - Changes       (25 Aug 97)
**                                      (15 Jan 98)
**                                      (22 Jul 98)
***************************************************************************/

#include <allegro.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "general.h"
#include "agifiles.h"
#include "view.h"
#include "picture.h"

ViewTable viewtab[TABLESIZE];
View loadedViews[MAXVIEW];
BITMAP *spriteScreen;

extern byte var[256];
extern boolean flag[256];
extern char string[12][40];
extern byte horizon;
extern int dirnOfEgo;

void initViews()
{
   int i;

   for (i=0; i<256; i++) {
      loadedViews[i].loaded = FALSE;
      loadedViews[i].numberOfLoops = 0;
   }

   spriteScreen = create_bitmap(160, 168);
}

void initObjects()
{
   int entryNum;

   //spriteScreen = create_bitmap(160, 168);

   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      viewtab[entryNum].stepTime = 1;
      viewtab[entryNum].stepTimeCount = 1;
      viewtab[entryNum].xPos = 0;
      viewtab[entryNum].yPos = 0;
      viewtab[entryNum].currentView = 0;
      viewtab[entryNum].viewData = NULL;
      viewtab[entryNum].currentLoop = 0;
      viewtab[entryNum].numberOfLoops = 0;
      viewtab[entryNum].loopData = NULL;
      viewtab[entryNum].currentCel = 0;
      viewtab[entryNum].numberOfCels = 0;
      viewtab[entryNum].celData = NULL;
      viewtab[entryNum].bgPic = NULL;
      viewtab[entryNum].bgPri = NULL;
      viewtab[entryNum].bgX = 0;
      viewtab[entryNum].bgY = 0;
      viewtab[entryNum].xsize = 0;
      viewtab[entryNum].ysize = 0;
      viewtab[entryNum].stepSize = 1;
      viewtab[entryNum].cycleTime = 1;
      viewtab[entryNum].cycleTimeCount = 1;
      viewtab[entryNum].direction = 0;
      viewtab[entryNum].motion = 0;
      viewtab[entryNum].cycleStatus = 0;
      viewtab[entryNum].priority = 0;
      viewtab[entryNum].flags = 0;
   }
}

void resetViews()     /* Called after new.room */
{
   int entryNum;

   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      destroy_bitmap(viewtab[entryNum].bgPic);
      viewtab[entryNum].bgPic = NULL;
      destroy_bitmap(viewtab[entryNum].bgPri);
      viewtab[entryNum].bgPri = NULL;
      viewtab[entryNum].flags &= ~(UPDATE | ANIMATED);
   }
}

/**************************************************************************
** loadViewFile
**
** Purpose: Loads a VIEW file into memory storing it in the loadedViews
** array.
**************************************************************************/
void loadViewFile(byte viewNum)
{
   AGIFile tempAGI;
   byte *viewStart, *loopStart, *celStart, cWidth;
   byte l, c, x, y, chunk, xTotal, colour, len, loopIndex, viewIndex, trans;

   loadAGIFile(VIEW, viewdir[viewNum], &tempAGI);
   viewStart = tempAGI.data;
   loadedViews[viewNum].description = ((viewStart[3] || viewStart[4])?
      strdup((byte *)(viewStart+viewStart[3]+viewStart[4]*256)) : strdup(""));
   loadedViews[viewNum].numberOfLoops = viewStart[2];
   loadedViews[viewNum].loops = (Loop *)malloc(viewStart[2]*sizeof(Loop));

   for (l=0, viewIndex=5; l<loadedViews[viewNum].numberOfLoops; l++, viewIndex+=2) {
      loopStart = (byte *)(viewStart + viewStart[viewIndex] + viewStart[viewIndex+1]*256);
      loadedViews[viewNum].loops[l].numberOfCels = loopStart[0];
      loadedViews[viewNum].loops[l].cels = (Cel *)malloc(loopStart[0]*sizeof(Cel));

      for (c=0, loopIndex=1; c<loadedViews[viewNum].loops[l].numberOfCels; c++, loopIndex+=2) {
         celStart = (byte *)(loopStart + loopStart[loopIndex] + loopStart[loopIndex+1]*256);
         memcpy(&loadedViews[viewNum].loops[l].cels[c], celStart, 3);
         loadedViews[viewNum].loops[l].cels[c].bmp = create_bitmap(celStart[0], celStart[1]);
         celStart+=3;
         trans = loadedViews[viewNum].loops[l].cels[c].transparency;
         cWidth = loadedViews[viewNum].loops[l].cels[c].width;

         for (y=0; y<loadedViews[viewNum].loops[l].cels[c].height; y++) {
            xTotal=0;
            if ((trans & 0x80) && (((trans & 0x70) >> 4) != l)) { /* Flip */
               while (chunk = *celStart++) { /* Until the end of the line */
                  colour = ((chunk & 0xF0) >> 4);
                  len = (chunk & 0x0F);
                  for (x=xTotal; x<(xTotal+len); x++) {
                     if (colour == (trans & 0x0f))
                        loadedViews[viewNum].loops[l].cels[c].bmp->
                           line[y][(cWidth-x)-1] = colour + 1;
                     else
                        loadedViews[viewNum].loops[l].cels[c].bmp->
                           line[y][(cWidth-x)-1] = colour + 1;
                  }
                  xTotal += len;
               }
               for (x=xTotal; x<loadedViews[viewNum].loops[l].cels[c].width; x++) {
                  loadedViews[viewNum].loops[l].cels[c].bmp->
                     line[y][(cWidth-x)-1] = (trans & 0x0F) + 1;
               }
            }
            else {
               while (chunk = *celStart++) { /* Until the end of the line */
                  colour = ((chunk & 0xF0) >> 4);
                  len = (chunk & 0x0F);
                  for (x=xTotal; x<(xTotal+len); x++) {
                     if (colour == (trans & 0x0f))
                        loadedViews[viewNum].loops[l].cels[c].bmp->
                           line[y][x] = colour + 1;
                     else
                        loadedViews[viewNum].loops[l].cels[c].bmp->
                           line[y][x] = colour + 1;
                  }
                  xTotal += len;
               }
               for (x=xTotal; x<loadedViews[viewNum].loops[l].cels[c].width; x++) {
                  loadedViews[viewNum].loops[l].cels[c].bmp->line[y][x] =
                     (trans & 0x0F) + 1;
               }
            }
         }
      }
   }

   free(tempAGI.data);   /* Deallocate original buffer. */
   loadedViews[viewNum].loaded = TRUE;
}

/***************************************************************************
** discardView
**
** Purpose: To deallocate memory associated with view number.
***************************************************************************/
void discardView(byte viewNum)
{
   byte l, c;

   if (loadedViews[viewNum].loaded) {
      for (l=0; l<loadedViews[viewNum].numberOfLoops; l++) {
         for (c=0; c<loadedViews[viewNum].loops[l].numberOfCels; c++) {
            destroy_bitmap(loadedViews[viewNum].loops[l].cels[c].bmp);
         }
         free(loadedViews[viewNum].loops[l].cels);
      }
      free(loadedViews[viewNum].description);
      free(loadedViews[viewNum].loops);
      loadedViews[viewNum].loaded = FALSE;
   }
}

void setCel(byte entryNum, byte celNum)
{
   Loop *temp;

   temp = viewtab[entryNum].loopData;
   viewtab[entryNum].currentCel = celNum;
   viewtab[entryNum].celData = &temp->cels[celNum];
   viewtab[entryNum].xsize = temp->cels[celNum].width;
   viewtab[entryNum].ysize = temp->cels[celNum].height;
}

void setLoop(byte entryNum, byte loopNum)
{
   View *temp;

   temp = viewtab[entryNum].viewData;
   viewtab[entryNum].currentLoop = loopNum;
   viewtab[entryNum].loopData = &temp->loops[loopNum];
   viewtab[entryNum].numberOfCels = temp->loops[loopNum].numberOfCels;
   /* Might have to set the cel as well */
   /* It's probably better to do that in the set_loop function */
}

/**************************************************************************
** addViewToTable
**
** Purpose: To add a loaded VIEW to the VIEW table.
**************************************************************************/
void addViewToTable(byte entryNum, byte viewNum)
{
   viewtab[entryNum].currentView = viewNum;
   viewtab[entryNum].viewData = &loadedViews[viewNum];
   viewtab[entryNum].numberOfLoops = loadedViews[viewNum].numberOfLoops;
   setLoop(entryNum, 0);
   viewtab[entryNum].numberOfCels = loadedViews[viewNum].loops[0].numberOfCels;
   setCel(entryNum, 0);
   /* Might need to set some more defaults here */
}

void addToPic(int vNum, int lNum, int cNum, int x, int y, int pNum, int bCol)
{
   int i, j, w, h, trans, c, boxWidth;

   trans = loadedViews[vNum].loops[lNum].cels[cNum].transparency & 0x0F;
   w = loadedViews[vNum].loops[lNum].cels[cNum].width;
   h = loadedViews[vNum].loops[lNum].cels[cNum].height;
   y = (y - h) + 1;

   for (i=0; i<w; i++) {
      for (j=0; j<h; j++) {
         c = loadedViews[vNum].loops[lNum].cels[cNum].bmp->line[j][i];
         if ((c != (trans+1)) && (pNum >= priority->line[y+j][x+i])) {
            priority->line[y+j][x+i] = pNum;
            picture->line[y+j][x+i] = c;
         }
      }
   }

   /* Maybe the box height only extends to the next priority band */

   boxWidth = ((h >= 7)? 7: h);
   if (bCol < 4) rect(control, x, (y+h)-(boxWidth), (x+w)-1, (y+h)-1, bCol);
}

/***************************************************************************
** agi_blit
***************************************************************************/
void agi_blit(BITMAP *bmp, int x, int y, int w, int h, byte trans, byte pNum)
{
   int i, j, c;

   for (i=0; i<w; i++) {
      for (j=0; j<h; j++) {
         c = bmp->line[j][i];
        // Next line will be removed when error is found.
        if (((y+j) < 168) && ((x+i) < 160) && ((y+j) >= 0) && ((x+i) >= 0))

         if ((c != (trans + 1)) && (pNum >= priority->line[y+j][x+i])) {
            priority->line[y+j][x+i] = pNum;
            //picture->line[y+j][x+i] = c;
            spriteScreen->line[y+j][x+i] = c;
         }
      }
   }
}

void calcDirection(int entryNum)
{
   if (!(viewtab[entryNum].flags & FIXLOOP)) {
      if (viewtab[entryNum].numberOfLoops < 4) {
         switch (viewtab[entryNum].direction) {
            case 1: break;
            case 2: setLoop(entryNum, 0); break;
            case 3: setLoop(entryNum, 0); break;
            case 4: setLoop(entryNum, 0); break;
            case 5: break;
            case 6: setLoop(entryNum, 1); break;
            case 7: setLoop(entryNum, 1); break;
            case 8: setLoop(entryNum, 1); break;
         }
      }
      else {
         switch (viewtab[entryNum].direction) {
            case 1: setLoop(entryNum, 3); break;
            case 2: setLoop(entryNum, 0); break;
            case 3: setLoop(entryNum, 0); break;
            case 4: setLoop(entryNum, 0); break;
            case 5: setLoop(entryNum, 2); break;
            case 6: setLoop(entryNum, 1); break;
            case 7: setLoop(entryNum, 1); break;
            case 8: setLoop(entryNum, 1); break;
         }
      }
   }
}

/* Called by draw() */
void drawObject(int entryNum)
{
   word objFlags;
   int dummy;

   if (entryNum == 4) {
      dummy = 1;
   }

   objFlags = viewtab[entryNum].flags;

   /* Store background */
   viewtab[entryNum].bgPic = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
   viewtab[entryNum].bgPri = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
   blit(picture, viewtab[entryNum].bgPic, viewtab[entryNum].xPos,
      viewtab[entryNum].yPos - viewtab[entryNum].ysize,
      0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
   blit(priority, viewtab[entryNum].bgPri, viewtab[entryNum].xPos,
      viewtab[entryNum].yPos - viewtab[entryNum].ysize,
      0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
   viewtab[entryNum].bgX = viewtab[entryNum].xPos;
   viewtab[entryNum].bgY = viewtab[entryNum].yPos - viewtab[entryNum].ysize;


   /* Determine priority for unfixed priorities */
   if (!(objFlags & FIXEDPRIORITY)) {
      if (viewtab[entryNum].yPos < 60)
         viewtab[entryNum].priority = 4;
      else
         viewtab[entryNum].priority = (viewtab[entryNum].yPos/12 + 1);
   }

   calcDirection(entryNum);

   agi_blit(viewtab[entryNum].celData->bmp,
      viewtab[entryNum].xPos,
      (viewtab[entryNum].yPos - viewtab[entryNum].ysize) + 1,
      viewtab[entryNum].xsize,
      viewtab[entryNum].ysize,
      viewtab[entryNum].celData->transparency & 0x0f,
      viewtab[entryNum].priority);
}

/***************************************************************************
** updateEgoDirection
**
** Purpose: To update var[6] when ego is moved with adjustPosition().
***************************************************************************/
void updateEgoDirection(int oldX, int oldY, int newX, int newY)
{
   int dx = (newX - oldX);
   int dy = (newY - oldY);

   if ((dx == 0) && (dy == 0)) var[6] = dirnOfEgo = 0;
   if ((dx == 0) && (dy == -1)) var[6] = dirnOfEgo = 1;
   if ((dx == 1) && (dy == -1)) var[6] = dirnOfEgo = 2;
   if ((dx == 1) && (dy == 0)) var[6] = dirnOfEgo = 3;
   if ((dx == 1) && (dy == 1)) var[6] = dirnOfEgo = 4;
   if ((dx == 0) && (dy == 1)) var[6] = dirnOfEgo = 5;
   if ((dx == -1) && (dy == 1)) var[6] = dirnOfEgo = 6;
   if ((dx == -1) && (dy == 0)) var[6] = dirnOfEgo = 7;
   if ((dx == -1) && (dy == -1)) var[6] = dirnOfEgo = 8;

   calcDirection(0);
}

/***************************************************************************
** adjustPosition
**
** Purpose: To adjust the given objects position so that it moves closer
** to the given position. The routine is similar to a line draw and is used
** for the move.obj. If the object is ego, then var[6] has to be updated.
***************************************************************************/
void adjustPosition(int entryNum, int fx, int fy)
{
   int height, width, startX, startY, x1, y1, x2, y2, count, stepVal, dx, dy;
   float x, y, addX, addY;
   int dummy;

   /* Set up start and end points */
   x1 = viewtab[entryNum].xPos;
   y1 = viewtab[entryNum].yPos;
   x2 = fx;
   y2 = fy;

   height = (y2 - y1);
   width = (x2 - x1);
   addX = (height==0?height:(float)width/abs(height));
   addY = (width==0?width:(float)height/abs(width));

   /* Will find the point on the line that is stepSize pixels away */
   if (abs(width) > abs(height)) {
      y = y1;
      addX = (width == 0? 0 : (width/abs(width)));
      switch ((int)addX) {
         case 0:
            if (addY < 0)
               viewtab[entryNum].direction = 1;
            else
               viewtab[entryNum].direction = 5;
            break;
         case -1:
            if (addY < 0)
               viewtab[entryNum].direction = 8;
            else if (addY > 0)
               viewtab[entryNum].direction = 6;
            else
               viewtab[entryNum].direction = 7;
            break;
         case 1:
            if (addY < 0)
               viewtab[entryNum].direction = 2;
            else if (addY > 0)
               viewtab[entryNum].direction = 4;
            else
               viewtab[entryNum].direction = 3;
            break;
      }
      count = 0;
      stepVal = viewtab[entryNum].stepSize;
      for (x=x1; (x!=x2) && (count<(stepVal+1)); x+=addX, count++) {
         dx = ceil(x);
         dy = ceil(y);
	      y+=addY;
      }
      if ((x == x2) && (count < (stepVal+1))) {
         dx = ceil(x);
         dy = ceil(y);
      }
   }
   else {
      x = x1;
      addY = (height == 0? 0 : (height/abs(height)));
      switch ((int)addY) {
         case 0:
            if (addX < 0)
               viewtab[entryNum].direction = 7;
            else
               viewtab[entryNum].direction = 3;
            break;
         case -1:
            if (addX < 0)
               viewtab[entryNum].direction = 8;
            else if (addX > 0)
               viewtab[entryNum].direction = 2;
            else
               viewtab[entryNum].direction = 1;
            break;
         case 1:
            if (addX < 0)
               viewtab[entryNum].direction = 6;
            else if (addX > 0)
               viewtab[entryNum].direction = 4;
            else
               viewtab[entryNum].direction = 5;
            break;
      }
      count = 0;
      stepVal = viewtab[entryNum].stepSize;
      for (y=y1; (y!=y2) && (count<(stepVal+1)); y+=addY, count++) {
         dx = ceil(x);
         dy = ceil(y);
	      x+=addX;
      }
      if ((y == y2) && (count < (stepVal+1))) {
         dx = ceil(x);
         dy = ceil(y);
      }
   }

   viewtab[entryNum].xPos = dx;
   viewtab[entryNum].yPos = dy;

   if (entryNum == 0) {
      updateEgoDirection(x1, y1, dx, dy);
   }
}

void followEgo(int entryNum) /* This needs to be more intelligent. */
{
   adjustPosition(entryNum, viewtab[0].xPos, viewtab[0].yPos);
}

void normalAdjust(int entryNum, int dx, int dy)
{
   int tempX, tempY, testX, startX, endX, waterCount=0;

   tempX = (viewtab[entryNum].xPos + dx);
   tempY = (viewtab[entryNum].yPos + dy);

   if (entryNum == 0) {
      if (tempX < 0) {   /* Hit left edge */
         var[2] = 4;
         return;
      }
      if (tempX > (160 - viewtab[entryNum].xsize)) {   /* Hit right edge */
         var[2] = 2;
         return;
      }
      if (tempY > 167) {   /* Hit bottom edge */
         var[2] = 3;
         return;
      }
      if (tempY < horizon) {   /* Hit horizon */
         var[2] = 1;
         return;
      }
   } else {   /* For all other objects */
      if (tempX < 0) {   /* Hit left edge */
         var[5] = 4;
         var[4] = entryNum;
         return;
      }
      if (tempX > (160 - viewtab[entryNum].xsize)) {   /* Hit right edge */
         var[5] = 2;
         var[4] = entryNum;
         return;
      }
      if (tempY > 167) {   /* Hit bottom edge */
         var[5] = 3;
         var[4] = entryNum;
         return;
      }
      if (tempY < horizon) {   /* Hit horizon */
         var[5] = 1;
         var[4] = entryNum;
         return;
      }
   }

   if (entryNum == 0) {
      flag[3] = 0;
      flag[0] = 0;

      /* End points of the base line */
      startX = tempX;
      endX = startX + viewtab[entryNum].xsize;
      for (testX=startX; testX<endX; testX++) {
         switch (control->line[tempY][testX]) {
            case 0: return;   /* Unconditional obstacle */
            case 1:
               if (viewtab[entryNum].flags & IGNOREBLOCKS) break;
               return;    /* Conditional obstacle */
            case 3:
               waterCount++;
               break;
            case 2: flag[3] = 1; /* Trigger */
               viewtab[entryNum].xPos = tempX;
               viewtab[entryNum].yPos = tempY;
               return;
         }
      }
      if (waterCount == viewtab[entryNum].xsize) {
         viewtab[entryNum].xPos = tempX;
         viewtab[entryNum].yPos = tempY;
         flag[0] = 1;
         return;
      }
   }
   else {
      /* End points of the base line */
      startX = tempX;
      endX = startX + viewtab[entryNum].xsize;
      for (testX=startX; testX<endX; testX++) {
         if ((viewtab[entryNum].flags & ONWATER) &&
             (control->line[tempY][testX] != 3)) {
            return;
         }
      }
   }

   viewtab[entryNum].xPos = tempX;
   viewtab[entryNum].yPos = tempY;
}

void updateObj(int entryNum)
{
   int oldX, oldY, celNum;
   word objFlags;

   objFlags = viewtab[entryNum].flags;

         /* Add saved background to picture\priority bitmaps */
         if (viewtab[entryNum].bgPic != NULL) {
            blit(viewtab[entryNum].bgPic, spriteScreen, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPic->w, viewtab[entryNum].bgPic->h);
            destroy_bitmap(viewtab[entryNum].bgPic);
            viewtab[entryNum].bgPic = NULL;
         }
         if (viewtab[entryNum].bgPri != NULL) {
            blit(viewtab[entryNum].bgPri, priority, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPri->w, viewtab[entryNum].bgPri->h);
            destroy_bitmap(viewtab[entryNum].bgPri);
            viewtab[entryNum].bgPri = NULL;
         }


      //if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {

         //if (objFlags & UPDATE) {

            if (objFlags & CYCLING) {
               viewtab[entryNum].cycleTimeCount++;
               if (viewtab[entryNum].cycleTimeCount >
                   viewtab[entryNum].cycleTime) {
                  viewtab[entryNum].cycleTimeCount = 1;
                  celNum = viewtab[entryNum].currentCel;
                  switch (viewtab[entryNum].cycleStatus) {
                     case 0: /* normal.cycle */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels)
                           celNum = 0;
                        setCel(entryNum, celNum);
                        break;
                     case 1: /* end.of.loop */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels) {
                           flag[viewtab[entryNum].param1] = 1;
                           /* viewtab[entryNum].flags &= ~CYCLING; */
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 2: /* reverse.loop */
                        celNum--;
                        if (celNum < 0) {
                           flag[viewtab[entryNum].param1] = 1;
                           /* viewtab[entryNum].flags &= ~CYCLING; */
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 3: /* reverse.cycle */
                        celNum--;
                        if (celNum < 0)
                           celNum = viewtab[entryNum].numberOfCels - 1;
                        setCel(entryNum, celNum);
                        break;
                  }
               }
            } /* CYCLING */

            if (objFlags & MOTION) {
               viewtab[entryNum].stepTimeCount++;
               if (viewtab[entryNum].stepTimeCount >
                   viewtab[entryNum].stepTime) {
                  viewtab[entryNum].stepTimeCount = 1;
                  switch (viewtab[entryNum].motion) {
                     case 0: /* normal.motion */
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        break;
                     case 1: /* wander */
                        oldX = viewtab[entryNum].xPos;
                        oldY = viewtab[entryNum].yPos;
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        if ((viewtab[entryNum].xPos == oldX) &&
                            (viewtab[entryNum].yPos == oldY)) {
                           viewtab[entryNum].direction = (rand() % 8) + 1;
                        }
                        break;
                     case 2: /* follow.ego */
                        break;
                     case 3: /* move.obj */
                        adjustPosition(entryNum, viewtab[entryNum].param1,
                           viewtab[entryNum].param2);
                        if ((viewtab[entryNum].xPos == viewtab[entryNum].param1) &&
                            (viewtab[entryNum].yPos == viewtab[entryNum].param2)) {
                           viewtab[entryNum].motion = 0;
                           viewtab[entryNum].flags &= ~MOTION;
                           flag[viewtab[entryNum].param4] = 1;
                        }
                        break;
                  }
               }
            } /* MOTION */

         //} /* UPDATE */

         /* Store background */
         viewtab[entryNum].bgPic = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgPri = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(picture, viewtab[entryNum].bgPic, viewtab[entryNum].xPos,
            viewtab[entryNum].yPos - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(priority, viewtab[entryNum].bgPri, viewtab[entryNum].xPos,
            viewtab[entryNum].yPos - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgX = viewtab[entryNum].xPos;
         viewtab[entryNum].bgY = viewtab[entryNum].yPos - viewtab[entryNum].ysize;

         /* Determine priority for unfixed priorities */
         if (!(objFlags & FIXEDPRIORITY)) {
            if (viewtab[entryNum].yPos < 60)
               viewtab[entryNum].priority = 4;
            else
               viewtab[entryNum].priority = (viewtab[entryNum].yPos/12 + 1);
         }


      if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {
         /* Draw new cel onto picture\priority bitmaps */
         agi_blit(viewtab[entryNum].celData->bmp,
            viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos - viewtab[entryNum].ysize) + 1,
            viewtab[entryNum].xsize,
            viewtab[entryNum].ysize,
            viewtab[entryNum].celData->transparency & 0x0f,
            viewtab[entryNum].priority);
      }

   show_mouse(NULL);
   stretch_sprite(agi_screen, spriteScreen, 0, 0, 640, 336);
   show_mouse(screen);
   //showPicture();
}


/* Called by force.update */
void updateObj2(int entryNum)
{
   int oldX, oldY, celNum;
   word objFlags;

   objFlags = viewtab[entryNum].flags;

         /* Add saved background to picture\priority bitmaps */
         if (viewtab[entryNum].bgPic != NULL) {
            blit(viewtab[entryNum].bgPic, picture, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPic->w, viewtab[entryNum].bgPic->h);
            destroy_bitmap(viewtab[entryNum].bgPic);
            viewtab[entryNum].bgPic = NULL;
         }
         if (viewtab[entryNum].bgPri != NULL) {
            blit(viewtab[entryNum].bgPri, priority, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPri->w, viewtab[entryNum].bgPri->h);
            destroy_bitmap(viewtab[entryNum].bgPri);
            viewtab[entryNum].bgPri = NULL;
         }


      //if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {

         //if (objFlags & UPDATE) {

            if (objFlags & CYCLING) {
               viewtab[entryNum].cycleTimeCount++;
               if (viewtab[entryNum].cycleTimeCount >
                   viewtab[entryNum].cycleTime) {
                  viewtab[entryNum].cycleTimeCount = 1;
                  celNum = viewtab[entryNum].currentCel;
                  switch (viewtab[entryNum].cycleStatus) {
                     case 0: /* normal.cycle */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels)
                           celNum = 0;
                        setCel(entryNum, celNum);
                        break;
                     case 1: /* end.of.loop */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels) {
                           flag[viewtab[entryNum].param1] = 1;
                           viewtab[entryNum].flags &= ~CYCLING;
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 2: /* reverse.loop */
                        celNum--;
                        if (celNum < 0) {
                           flag[viewtab[entryNum].param1] = 1;
                           viewtab[entryNum].flags &= ~CYCLING;
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 3: /* reverse.cycle */
                        celNum--;
                        if (celNum < 0)
                           celNum = viewtab[entryNum].numberOfCels - 1;
                        setCel(entryNum, celNum);
                        break;
                  }
               }
            } /* CYCLING */

            if (objFlags & MOTION) {
               viewtab[entryNum].stepTimeCount++;
               if (viewtab[entryNum].stepTimeCount >
                   viewtab[entryNum].stepTime) {
                  viewtab[entryNum].stepTimeCount = 1;
                  switch (viewtab[entryNum].motion) {
                     case 0: /* normal.motion */
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        break;
                     case 1: /* wander */
                        oldX = viewtab[entryNum].xPos;
                        oldY = viewtab[entryNum].yPos;
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        if ((viewtab[entryNum].xPos == oldX) &&
                            (viewtab[entryNum].yPos == oldY)) {
                           viewtab[entryNum].direction = (rand() % 8) + 1;
                        }
                        break;
                     case 2: /* follow.ego */
                        break;
                     case 3: /* move.obj */
                        adjustPosition(entryNum, viewtab[entryNum].param1,
                           viewtab[entryNum].param2);
                        if ((viewtab[entryNum].xPos == viewtab[entryNum].param1) &&
                            (viewtab[entryNum].yPos == viewtab[entryNum].param2)) {
                           viewtab[entryNum].motion = 0;
                           viewtab[entryNum].flags &= ~MOTION;
                           flag[viewtab[entryNum].param4] = 1;
                        }
                        break;
                  }
               }
            } /* MOTION */

         //} /* UPDATE */

         /* Store background */
         viewtab[entryNum].bgPic = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgPri = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(picture, viewtab[entryNum].bgPic, viewtab[entryNum].xPos,
            viewtab[entryNum].yPos - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(priority, viewtab[entryNum].bgPri, viewtab[entryNum].xPos,
            viewtab[entryNum].yPos - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgX = viewtab[entryNum].xPos;
         viewtab[entryNum].bgY = viewtab[entryNum].yPos - viewtab[entryNum].ysize;

         /* Determine priority for unfixed priorities */
         if (!(objFlags & FIXEDPRIORITY)) {
            if (viewtab[entryNum].yPos < 60)
               viewtab[entryNum].priority = 4;
            else
               viewtab[entryNum].priority = (viewtab[entryNum].yPos/12 + 1);
         }

         /* Draw new cel onto picture\priority bitmaps */
         /*
         agi_blit(viewtab[entryNum].celData->bmp,
            viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos - viewtab[entryNum].ysize) + 1,
            viewtab[entryNum].xsize,
            viewtab[entryNum].ysize,
            viewtab[entryNum].celData->transparency & 0x0f,
            viewtab[entryNum].priority);
         */
      //}

      if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {
         /* Draw new cel onto picture\priority bitmaps */
         agi_blit(viewtab[entryNum].celData->bmp,
            viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos - viewtab[entryNum].ysize) + 1,
            viewtab[entryNum].xsize,
            viewtab[entryNum].ysize,
            viewtab[entryNum].celData->transparency & 0x0f,
            viewtab[entryNum].priority);
      }

   showPicture();
}

void updateObjects()
{
   int entryNum, celNum, oldX, oldY;
   word objFlags;

   /* If the show.pic() command was executed, display the picture
   ** with this object update.
   */
   /*
   if (okToShowPic) {
      okToShowPic = FALSE;
      blit(picture, spriteScreen, 0, 0, 0, 0, 160, 168);
   } else {
      clear(spriteScreen);
   }*/
   clear(spriteScreen);

   /******************* Place all background bitmaps *******************/
   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      objFlags = viewtab[entryNum].flags;
      //if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {
         /* Add saved background to picture\priority bitmaps */
         if (viewtab[entryNum].bgPic != NULL) {
            blit(viewtab[entryNum].bgPic, spriteScreen, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPic->w, viewtab[entryNum].bgPic->h);
            destroy_bitmap(viewtab[entryNum].bgPic);
            viewtab[entryNum].bgPic = NULL;
         }
         if (viewtab[entryNum].bgPri != NULL) {
            blit(viewtab[entryNum].bgPri, priority, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPri->w, viewtab[entryNum].bgPri->h);
            destroy_bitmap(viewtab[entryNum].bgPri);
            viewtab[entryNum].bgPri = NULL;
         }
      //}
   }

   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      objFlags = viewtab[entryNum].flags;

      if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {

         if (objFlags & UPDATE) {

            if (objFlags & CYCLING) {
               viewtab[entryNum].cycleTimeCount++;
               if (viewtab[entryNum].cycleTimeCount >
                   viewtab[entryNum].cycleTime) {
                  viewtab[entryNum].cycleTimeCount = 1;
                  celNum = viewtab[entryNum].currentCel;
                  switch (viewtab[entryNum].cycleStatus) {
                     case 0: /* normal.cycle */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels)
                           celNum = 0;
                        setCel(entryNum, celNum);
                        break;
                     case 1: /* end.of.loop */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels) {
                           flag[viewtab[entryNum].param1] = 1;
                           /* viewtab[entryNum].flags &= ~CYCLING; */
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 2: /* reverse.loop */
                        celNum--;
                        if (celNum < 0) {
                           flag[viewtab[entryNum].param1] = 1;
                           /* viewtab[entryNum].flags &= ~CYCLING; */
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 3: /* reverse.cycle */
                        celNum--;
                        if (celNum < 0)
                           celNum = viewtab[entryNum].numberOfCels - 1;
                        setCel(entryNum, celNum);
                        break;
                  }
               }
            } /* CYCLING */
         } /* UPDATE */

         /* Store background */
         viewtab[entryNum].bgPic = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgPri = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(picture, viewtab[entryNum].bgPic, viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos+1) - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(priority, viewtab[entryNum].bgPri, viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos+1) - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgX = viewtab[entryNum].xPos;
         viewtab[entryNum].bgY = (viewtab[entryNum].yPos+1) - viewtab[entryNum].ysize;

         /* Determine priority for unfixed priorities */
         if (!(objFlags & FIXEDPRIORITY)) {
            if (viewtab[entryNum].yPos < 60)
               viewtab[entryNum].priority = 4;
            else
               viewtab[entryNum].priority = (viewtab[entryNum].yPos/12 + 1);
         }
      }
   }

   /* Draw all cels */
   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      objFlags = viewtab[entryNum].flags;
      if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {
         /* Draw new cel onto picture\priority bitmaps */
         agi_blit(viewtab[entryNum].celData->bmp,
            viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos - viewtab[entryNum].ysize) + 1,
            viewtab[entryNum].xsize,
            viewtab[entryNum].ysize,
            viewtab[entryNum].celData->transparency & 0x0f,
            viewtab[entryNum].priority);
      }
   }

   show_mouse(NULL);
   stretch_sprite(agi_screen, spriteScreen, 0, 0, 640, 336);
   show_mouse(screen);
}


void updateObjects2()
{
   int entryNum, celNum, oldX, oldY;
   word objFlags;

   /* Place all background bitmaps */
   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      objFlags = viewtab[entryNum].flags;
      //if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {
         /* Add saved background to picture\priority bitmaps */
         if (viewtab[entryNum].bgPic != NULL) {
            blit(viewtab[entryNum].bgPic, picture, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPic->w, viewtab[entryNum].bgPic->h);
            destroy_bitmap(viewtab[entryNum].bgPic);
            viewtab[entryNum].bgPic = NULL;
         }
         if (viewtab[entryNum].bgPri != NULL) {
            blit(viewtab[entryNum].bgPri, priority, 0, 0,
               viewtab[entryNum].bgX, viewtab[entryNum].bgY,
               viewtab[entryNum].bgPri->w, viewtab[entryNum].bgPri->h);
            destroy_bitmap(viewtab[entryNum].bgPri);
            viewtab[entryNum].bgPri = NULL;
         }
      //}
   }

   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      objFlags = viewtab[entryNum].flags;

      if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {

         if (objFlags & UPDATE) {

            if (objFlags & CYCLING) {
               viewtab[entryNum].cycleTimeCount++;
               if (viewtab[entryNum].cycleTimeCount >
                   viewtab[entryNum].cycleTime) {
                  viewtab[entryNum].cycleTimeCount = 1;
                  celNum = viewtab[entryNum].currentCel;
                  switch (viewtab[entryNum].cycleStatus) {
                     case 0: /* normal.cycle */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels)
                           celNum = 0;
                        setCel(entryNum, celNum);
                        break;
                     case 1: /* end.of.loop */
                        celNum++;
                        if (celNum >= viewtab[entryNum].numberOfCels) {
                           flag[viewtab[entryNum].param1] = 1;
                           viewtab[entryNum].flags &= ~CYCLING;
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 2: /* reverse.loop */
                        celNum--;
                        if (celNum < 0) {
                           flag[viewtab[entryNum].param1] = 1;
                           viewtab[entryNum].flags &= ~CYCLING;
                        }
                        else
                           setCel(entryNum, celNum);
                        break;
                     case 3: /* reverse.cycle */
                        celNum--;
                        if (celNum < 0)
                           celNum = viewtab[entryNum].numberOfCels - 1;
                        setCel(entryNum, celNum);
                        break;
                  }
               }
            } /* CYCLING */
/*
            if (objFlags & MOTION) {
               viewtab[entryNum].stepTimeCount++;
               if (viewtab[entryNum].stepTimeCount >
                   viewtab[entryNum].stepTime) {
                  viewtab[entryNum].stepTimeCount = 1;
                  switch (viewtab[entryNum].motion) {
                     case 0: // normal.motion
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        break;
                     case 1: // wander
                        oldX = viewtab[entryNum].xPos;
                        oldY = viewtab[entryNum].yPos;
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        if ((viewtab[entryNum].xPos == oldX) &&
                            (viewtab[entryNum].yPos == oldY)) {
                           viewtab[entryNum].direction = (rand() % 8) + 1;
                        }
                        break;
                     case 2: // follow.ego
                        break;
                     case 3: // move.obj
                        adjustPosition(entryNum, viewtab[entryNum].param1,
                           viewtab[entryNum].param2);
                        if ((viewtab[entryNum].xPos == viewtab[entryNum].param1) &&
                            (viewtab[entryNum].yPos == viewtab[entryNum].param2)) {
                           viewtab[entryNum].motion = 0;
                           viewtab[entryNum].flags &= ~MOTION;
                           flag[viewtab[entryNum].param4] = 1;
                        }
                        break;
                  }
               }
            } // MOTION
*/
         } /* UPDATE */

         /* Store background */
         viewtab[entryNum].bgPic = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgPri = create_bitmap(viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(picture, viewtab[entryNum].bgPic, viewtab[entryNum].xPos,
            viewtab[entryNum].yPos - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         blit(priority, viewtab[entryNum].bgPri, viewtab[entryNum].xPos,
            viewtab[entryNum].yPos - viewtab[entryNum].ysize,
            0, 0, viewtab[entryNum].xsize, viewtab[entryNum].ysize);
         viewtab[entryNum].bgX = viewtab[entryNum].xPos;
         viewtab[entryNum].bgY = viewtab[entryNum].yPos - viewtab[entryNum].ysize;

         /* Determine priority for unfixed priorities */
         if (!(objFlags & FIXEDPRIORITY)) {
            if (viewtab[entryNum].yPos < 60)
               viewtab[entryNum].priority = 4;
            else
               viewtab[entryNum].priority = (viewtab[entryNum].yPos/12 + 1);
         }

         /* Draw new cel onto picture\priority bitmaps */
         /*
         agi_blit(viewtab[entryNum].celData->bmp,
            viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos - viewtab[entryNum].ysize) + 1,
            viewtab[entryNum].xsize,
            viewtab[entryNum].ysize,
            viewtab[entryNum].celData->transparency & 0x0f,
            viewtab[entryNum].priority);
         */
      }
   }

   /* Draw all cels */
   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {
      objFlags = viewtab[entryNum].flags;
      if ((objFlags & ANIMATED) && (objFlags & DRAWN)) {
         /* Draw new cel onto picture\priority bitmaps */
         agi_blit(viewtab[entryNum].celData->bmp,
            viewtab[entryNum].xPos,
            (viewtab[entryNum].yPos - viewtab[entryNum].ysize) + 1,
            viewtab[entryNum].xsize,
            viewtab[entryNum].ysize,
            viewtab[entryNum].celData->transparency & 0x0f,
            viewtab[entryNum].priority);
      }
   }

   showPicture();
}

void calcObjMotion()
{
   int entryNum, celNum, oldX, oldY, steps=0;
   word objFlags;

   for (entryNum=0; entryNum<TABLESIZE; entryNum++) {

      objFlags = viewtab[entryNum].flags;

            if ((objFlags & MOTION) && (objFlags & UPDATE)) {
               viewtab[entryNum].stepTimeCount++;
               if (viewtab[entryNum].stepTimeCount >
                   viewtab[entryNum].stepTime) {
                  viewtab[entryNum].stepTimeCount = 1;
                  switch (viewtab[entryNum].motion) {
                     case 0: /* normal.motion */
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        break;
                     case 1: /* wander */
                        oldX = viewtab[entryNum].xPos;
                        oldY = viewtab[entryNum].yPos;
                        switch (viewtab[entryNum].direction) {
                           case 0: break;
                           case 1: normalAdjust(entryNum, 0, -1); break;
                           case 2: normalAdjust(entryNum, 1, -1); break;
                           case 3: normalAdjust(entryNum, 1, 0); break;
                           case 4: normalAdjust(entryNum, 1, 1); break;
                           case 5: normalAdjust(entryNum, 0, 1); break;
                           case 6: normalAdjust(entryNum, -1, 1); break;
                           case 7: normalAdjust(entryNum, -1, 0); break;
                           case 8: normalAdjust(entryNum, -1, -1); break;
                        }
                        if ((viewtab[entryNum].xPos == oldX) &&
                            (viewtab[entryNum].yPos == oldY)) {
                           viewtab[entryNum].direction = (rand() % 8) + 1;
                        }
                        break;
                     case 2: /* follow.ego */
                        followEgo(entryNum);
                        if ((viewtab[entryNum].xPos == viewtab[0].xPos) &&
                            (viewtab[entryNum].yPos == viewtab[0].yPos)) {
                           viewtab[entryNum].motion = 0;
                           viewtab[entryNum].flags &= ~MOTION;
                           flag[viewtab[entryNum].param2] = 1;
                           /* Not sure about this next line */
                           viewtab[entryNum].stepSize = viewtab[entryNum].param1;
                        }
                        break;
                     case 3: /* move.obj */
                        if (flag[viewtab[entryNum].param4]) break;
                        for (steps=0; steps<viewtab[entryNum].stepSize; steps++) {
                           adjustPosition(entryNum, viewtab[entryNum].param1,
                              viewtab[entryNum].param2);
                           if ((viewtab[entryNum].xPos == viewtab[entryNum].param1) &&
                               (viewtab[entryNum].yPos == viewtab[entryNum].param2)) {
                              /* These lines really are guess work */
                              viewtab[entryNum].motion = 0;
                              //viewtab[entryNum].flags &= ~MOTION;
                              viewtab[entryNum].direction = 0;
                              if (entryNum == 0) var[6] = 0;
                              flag[viewtab[entryNum].param4] = 1;
                              viewtab[entryNum].stepSize = viewtab[entryNum].param3;
                              break;
                           }
                        }
                        break;
                  }
               }
            } /* MOTION */

            /* Automatic change of direction if needed */
            calcDirection(entryNum);
   }
}

/***************************************************************************
** showView
**
** Purpose: To display all the cells of VIEW.
***************************************************************************/
void showView(int viewNum)
{
   int loopNum, celNum, maxHeight, totalWidth, totalHeight=5;
   int startX=0, startY=0;
   BITMAP *temp = create_bitmap(800, 600);
   BITMAP *scn = create_bitmap(320, 240);
   char viewString[20], loopString[3];
   boolean stillViewing = TRUE;

   clear_to_color(temp, 15);

   for (loopNum=0; loopNum<loadedViews[viewNum].numberOfLoops; loopNum++) {
      maxHeight = 0;
      totalWidth = 25;
      sprintf(loopString, "%2d", loopNum);
      drawString(temp, loopString, 2, totalHeight, 0, 15);
      for (celNum=0; celNum<loadedViews[viewNum].loops[loopNum].numberOfCels; celNum++) {
         if (maxHeight < loadedViews[viewNum].loops[loopNum].cels[celNum].height)
            maxHeight = loadedViews[viewNum].loops[loopNum].cels[celNum].height;
         //blit(loadedViews[viewNum].loops[loopNum].cels[celNum].bmp, temp,
         //   0, 0, totalWidth, totalHeight,
         //   loadedViews[viewNum].loops[loopNum].cels[celNum].width,
         //   loadedViews[viewNum].loops[loopNum].cels[celNum].height);
         stretch_blit(loadedViews[viewNum].loops[loopNum].cels[celNum].bmp,
            temp, 0, 0,
            loadedViews[viewNum].loops[loopNum].cels[celNum].width,
            loadedViews[viewNum].loops[loopNum].cels[celNum].height,
            totalWidth, totalHeight,
            loadedViews[viewNum].loops[loopNum].cels[celNum].width*2,
            loadedViews[viewNum].loops[loopNum].cels[celNum].height);
         totalWidth += loadedViews[viewNum].loops[loopNum].cels[celNum].width*2;
         totalWidth += 3;
      }
      if (maxHeight < 10) maxHeight = 10;
      totalHeight += maxHeight;
      totalHeight += 3;
   }

   if (strcmp(loadedViews[viewNum].description, "") != 0) {
      int i, counter=0, descLine=0, maxLen=0, strPos;
      char tempString[500]="";
      char *string = loadedViews[viewNum].description;

      totalHeight += 20;

      for (i=0; i<strlen(string); i++) {
         if (counter++ > 30) {
            for (strPos=strlen(tempString)-1; strPos>=0; strPos--)
               if (tempString[strPos] == ' ') break;
            tempString[strPos] = 0;
            drawString(temp, tempString, 27, totalHeight+descLine, 0, 15);
            sprintf(tempString, "%s%c", &tempString[strPos+1], string[i]);
            descLine+=8;
            if (strPos > maxLen) maxLen = strPos;
	         counter = strlen(tempString);
         }
         else {
            if (string[i] == 0x0A) {
   	         sprintf(tempString, "%s\\n", tempString);
   	         counter++;
            }
            else {
   	         if (string[i] == '\"') {
   	            sprintf(tempString, "%s\\\"", tempString);
   	            counter++;
   	         }
   	         else {
   	             sprintf(tempString, "%s%c", tempString, string[i]);
   	         }
            }
         }
      }
      drawString(temp, tempString, 27, totalHeight+descLine, 0, 15);
      rect(temp, 25, totalHeight-2, 25+(maxLen+1)*8+3,
         (totalHeight-2)+(descLine+8)+3, 4);
   }

   sprintf(viewString, "view.%d", viewNum);
   rect(scn, 0, 0, 319, 9, 4);
   rectfill(scn, 1, 1, 318, 8, 4);
   drawString(scn, viewString, 130, 1, 0, 4);
   rect(scn, 310, 9, 319, 230, 4);
   rectfill(scn, 311, 10, 318, 229, 3);
   rect(scn, 310, 230, 319, 239, 4);
   drawChar(scn, 0xB1, 311, 231, 4, 3);
   drawChar(scn, 0x18, 311, 11, 4, 3);
   drawChar(scn, 0x19, 311, 222, 4, 3);
   rect(scn, 0, 230, 310, 239, 4);
   rectfill(scn, 1, 231, 309, 238, 3);
   drawChar(scn, 0x1B, 2, 231, 4, 3);
   drawChar(scn, 0x1A, 302, 231, 4, 3);
   blit(temp, scn, 0, 0, 0, 10, 310, 220);
   stretch_blit(scn, screen, 0, 0, 320, 240, 0, 0, 640, 480);

   while (stillViewing) {
      switch (readkey() >> 8) {
         case KEY_ESC:
            stillViewing = FALSE;
            break;
         case KEY_UP:
            startY-=5;
            break;
         case KEY_DOWN:
            startY+=5;
            break;
         case KEY_LEFT:
            startX-=5;
            break;
         case KEY_RIGHT:
            startX+=5;
            break;
         case KEY_PGUP:
            startY-=220;
            break;
         case KEY_PGDN:
            startY+=220;
            break;
         case KEY_END:
            startX = 489;
            break;
         case KEY_HOME:
            startX = 0;
            break;
      }
      if (startY < 0) startY = 0;
      if (startY >= 380) startY = 379;
      if (startX < 0) startX = 0;
      if (startX >= 490) startX = 489;
      stretch_blit(temp, screen, startX, startY, 310, 220, 0, 20, 620, 440);
   }

   destroy_bitmap(temp);
   destroy_bitmap(scn);
}

/***************************************************************************
** showView2
**
** Purpose: To display AGI VIEWs and allow scrolling through the cells and
** loops. You have to install the allegro keyboard handler to call this
** function.
***************************************************************************/
void showView2(int viewNum)
{
   int loopNum=0, celNum=0;
   BITMAP *temp = create_bitmap(640, 480);
   char viewString[20], loopString[20], celString[20];
   boolean stillViewing = TRUE;

   sprintf(viewString, "View number: %d", viewNum);

   while (stillViewing) {
      clear(temp);
      textout(temp, font, viewString, 10, 10, 15);
      sprintf(loopString, "Loop number: %d", loopNum);
      sprintf(celString, "Cel number: %d", celNum);
      textout(temp, font, loopString, 10, 18, 15);
      textout(temp, font, celString, 10, 26, 15);
      stretch_blit(loadedViews[viewNum].loops[loopNum].cels[celNum].bmp, temp,
         0, 0, loadedViews[viewNum].loops[loopNum].cels[celNum].width,
         loadedViews[viewNum].loops[loopNum].cels[celNum].height, 10, 40,
         loadedViews[viewNum].loops[loopNum].cels[celNum].width*4,
         loadedViews[viewNum].loops[loopNum].cels[celNum].height*3);
      blit(temp, screen, 0, 0, 0, 0, 640, 480);
      switch (readkey() >> 8) {  /* Scan code */
         case KEY_UP:
            loopNum++;
            break;
         case KEY_DOWN:
            loopNum--;
            break;
         case KEY_LEFT:
            celNum--;
            break;
         case KEY_RIGHT:
            celNum++;
            break;
         case KEY_ESC:
            stillViewing = FALSE;
            break;
      }

      if (loopNum < 0) loopNum = loadedViews[viewNum].numberOfLoops-1;
      if (loopNum >= loadedViews[viewNum].numberOfLoops) loopNum = 0;
      if (celNum < 0)
         celNum = loadedViews[viewNum].loops[loopNum].numberOfCels-1;
      if (celNum >= loadedViews[viewNum].loops[loopNum].numberOfCels)
         celNum = 0;
   }

   destroy_bitmap(temp);
}

/***************************************************************************
** showObjectState
**
** This function shows the full on screen object state. It shows all view
** table variables and displays the current cell.
**
** Params:
**
**    objNum              Object number.
**
***************************************************************************/
void showObjectState(int objNum)
{
   char tempStr[256];
   BITMAP *temp = create_bitmap(640, 480);

   while (keypressed()) { /* Wait */ }

   show_mouse(NULL);
   blit(screen, temp, 0, 0, 0, 0, 640, 480);
   rectfill(screen, 10, 10, 630, 470,  0);
   rect(screen, 10, 10, 630, 470, 16);

   stretch_blit(viewtab[objNum].celData->bmp, screen, 0, 0,
      viewtab[objNum].xsize, viewtab[objNum].ysize, 200, 18,
      viewtab[objNum].xsize*4, viewtab[objNum].ysize*4);

   sprintf(tempStr, "objNum: %d", objNum);
   textout(screen, font, tempStr, 18, 18, 8);
   sprintf(tempStr, "xPos: %d", viewtab[objNum].xPos);
   textout(screen, font, tempStr, 18, 28, 8);
   sprintf(tempStr, "yPos: %d", viewtab[objNum].yPos);
   textout(screen, font, tempStr, 18, 38, 8);
   sprintf(tempStr, "xSize: %d", viewtab[objNum].xsize);
   textout(screen, font, tempStr, 18, 48, 8);
   sprintf(tempStr, "ySize: %d", viewtab[objNum].ysize);
   textout(screen, font, tempStr, 18, 58, 8);
   sprintf(tempStr, "currentView: %d", viewtab[objNum].currentView);
   textout(screen, font, tempStr, 18, 68, 8);
   sprintf(tempStr, "numOfLoops: %d", viewtab[objNum].numberOfLoops);
   textout(screen, font, tempStr, 18, 78, 8);
   sprintf(tempStr, "currentLoop: %d", viewtab[objNum].currentLoop);
   textout(screen, font, tempStr, 18, 88, 8);
   sprintf(tempStr, "numberOfCels: %d", viewtab[objNum].numberOfCels);
   textout(screen, font, tempStr, 18, 98, 8);
   sprintf(tempStr, "currentCel: %d", viewtab[objNum].currentCel);
   textout(screen, font, tempStr, 18, 108, 8);
   sprintf(tempStr, "stepSize: %d", viewtab[objNum].stepSize);
   textout(screen, font, tempStr, 18, 118, 8);
   sprintf(tempStr, "stepTime: %d", viewtab[objNum].stepTime);
   textout(screen, font, tempStr, 18, 128, 8);
   sprintf(tempStr, "stepTimeCount: %d", viewtab[objNum].stepTimeCount);
   textout(screen, font, tempStr, 18, 138, 8);
   sprintf(tempStr, "cycleTime: %d", viewtab[objNum].cycleTime);
   textout(screen, font, tempStr, 18, 148, 8);
   sprintf(tempStr, "cycleTimeCount: %d", viewtab[objNum].cycleTimeCount);
   textout(screen, font, tempStr, 18, 158, 8);
   sprintf(tempStr, "direction: %d", viewtab[objNum].direction);
   textout(screen, font, tempStr, 18, 168, 8);
   sprintf(tempStr, "priority: %d", viewtab[objNum].priority);
   textout(screen, font, tempStr, 18, 178, 8);
   switch (viewtab[objNum].motion) {
      case 0: /* Normal motion */
         sprintf(tempStr, "motion: normal motion");
         break;
      case 1: /* Wander */
         sprintf(tempStr, "motion: wander");
         break;
      case 2: /* Follow ego */
         sprintf(tempStr, "motion: follow ego");
         break;
      case 3: /* Move object */
         sprintf(tempStr, "motion: move object");
         break;
      default:
         sprintf(tempStr, "motion: unknown");
         break;
   }
   textout(screen, font, tempStr, 18, 188, 8);
   switch (viewtab[objNum].cycleStatus) {
      case 0: /* Normal cycle */
         sprintf(tempStr, "cycleStatus: normal cycle");
         break;
      case 1: /* End of loop */
         sprintf(tempStr, "cycleStatus: end of loop");
         break;
      case 2: /* Reverse loop */
         sprintf(tempStr, "cycleStatus: reverse loop");
         break;
      case 3: /* Reverse cycle */
         sprintf(tempStr, "cycleStatus: reverse cycle");
         break;
      default:
         sprintf(tempStr, "cycleStatus: unknown");
         break;
   }
   textout(screen ,font, tempStr, 18, 198, 8);

   while (!keypressed()) { /* Wait */ }

   blit(temp, screen, 0, 0, 0, 0, 640, 480);
   destroy_bitmap(temp);
   show_mouse(screen);
}
