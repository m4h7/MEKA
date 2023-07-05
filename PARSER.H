/***************************************************************************
** parser.h
***************************************************************************/

#ifndef _PARSER_H_
#define _PARSER_H_

#define  NO_EVENT         0
#define  ASCII_KEY_EVENT  1
#define  SCAN_KEY_EVENT   2
#define  MENU_EVENT       3

/* An event as tested by the controller() test command is either a key or
** a menu item. */
typedef struct {
   byte type;     /* either key or menu item */
   byte eventID;  /* either scancode or menu item ID */
   byte asciiValue;
   byte scanCodeValue;
   boolean activated;
} eventType;

extern DIALOG the_dialog[];
extern eventType events[256];

extern int numInputWords, inputWords[];
extern char wordText[10][80];
extern boolean haveKey;
extern byte keyState[256], asciiState[256];
extern byte directions[9];
extern int lastKey;

#endif  /* _PARSER_H_ */
