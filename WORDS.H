/***************************************************************************
** words.h
***************************************************************************/

#ifndef _WORDS_H_
#define _WORDS_H_

typedef struct {
   char *wordText;
   int synonymNum;
} wordType;

extern wordType *words;
extern int numWords;
extern int numSynonyms;

#endif /* _WORDS_H_ */
