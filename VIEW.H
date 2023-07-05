/**************************************************************************
** VIEW.H
**************************************************************************/

#ifndef _VIEW_H_
#define _VIEW_H_

#include <allegro.h>

#include "general.h"

typedef struct {
   byte width;
   byte height;
   byte transparency;
   BITMAP *bmp;
} Cel;

typedef struct {
   byte numberOfCels;
   Cel *cels;
} Loop;

typedef struct {
   boolean loaded;
   byte numberOfLoops;
   Loop *loops;
   char *description;
} View;

#define DRAWN         0x0001
#define IGNOREBLOCKS  0x0002
#define FIXEDPRIORITY 0x0004
#define IGNOREHORIZON 0x0008
#define UPDATE        0x0010
#define CYCLING       0x0020
#define ANIMATED      0x0040
#define MOTION        0x0080
#define ONWATER       0x0100
#define IGNOREOBJECTS 0x0200
#define ONLAND        0x0800
#define FIXLOOP       0x2000

typedef struct {
   byte stepTime;
   byte stepTimeCount;
   word xPos;
   word yPos;
   byte currentView;
   View *viewData;             /* This pointer points to the loaded view */
   byte currentLoop;
   byte numberOfLoops;
   Loop *loopData;             /* ditto */
   byte currentCel;
   byte numberOfCels;
   Cel *celData;              /* ditto */
   BITMAP *bgPic;             /* Storage for background behind drawn view */
   BITMAP *bgPri;
   word bgX;                  /* Position to place background bmp */
   word bgY;
   word xsize;
   word ysize;
   byte stepSize;
   byte cycleTime;
   byte cycleTimeCount;
   byte direction;
   byte motion;
   byte cycleStatus;
   byte priority;
   word flags;
   byte param1;
   byte param2;
   byte param3;
   byte param4;
} ViewTable;

#define TABLESIZE  20  // 100
extern ViewTable viewtab[];

#define MAXVIEW  256
extern View loadedViews[];

extern BITMAP *spriteScreen;

void loadViewFile(byte viewNum);
void discardView(byte viewNum);
void addViewToTable(byte entryNum, byte viewNum);

#endif   /* _VIEW_H_ */

