/**************************************************************************
** AGIFILES.H
**************************************************************************/

#ifndef _AGIFILES_H_
#define _AGIFILES_H_

#define  LOGIC    0
#define  PICTURE  1
#define  VIEW     2
#define  SOUND    3

#define  EMPTY  0xFFFFF   /* Empty DIR entry */

typedef struct {          /* DIR entry structure */
   char *fileName;
   long filePos;
} AGIFilePosType;

typedef struct {          /* AGI data file structure */
   unsigned int size;
   unsigned char *data;
} AGIFile;

extern AGIFilePosType logdir[256], picdir[256], viewdir[256], snddir[256];
extern int numLogics, numPictures, numViews, numSounds;

void initfiles();
void loadAGIDirs();
void loadAGIFile(int resType, AGIFilePosType location, AGIFile *AGIData);

#endif  /* _AGIFILES_H_ */
