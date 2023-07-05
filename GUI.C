/***************************************************************************
** gui.c
**
** This module holds the code to implement MEKAs interface. At this stage
** it consists of an edit box, a bitmap, a status line, and a menu. It is
** uncertain whether I will go with a pop-up edit box for text input like
** the early SCI games, or stick to the way that Sierra did it for the
** AGI games.
**
** (c) 1997 Lance Ewing - Initial code (27 Aug 97)
***************************************************************************/

#include <allegro.h>

#include "gui.h"

extern int counter;

char inputString[41];
char status_line[81] = " Score:25 of 185    KQ2      Sound:off";



