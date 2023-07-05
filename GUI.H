/***************************************************************************
** gui.h
***************************************************************************/

#ifndef _GUI_H_
#define _GUI_H_

#include "general.h"

#define  ASCII_KEY_EVENT  0
#define  SCAN_KEY_EVENT   1
#define  MENU_EVENT       2

/* An event as tested by the controller() test command is either a key or
** a menu item. */
typedef struct {
   byte type;     /* either key or menu item */
   byte eventID;  /* either scancode or menu item ID */
   boolean activated;
} eventType;

extern DIALOG the_dialog[];
extern eventType events[256];

#endif  /* _GUI_H_ */
