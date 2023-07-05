/***************************************************************************
** object.h
***************************************************************************/

#ifndef _OBJECT_H_
#define _OBJECT_H_

typedef struct {
   int roomNum;
   char *name;
} objectType;

extern objectType *objects;
extern int numObjects;

void loadObjectFile();
void discardObjects();

#endif /* _OBJECT_H_ */
