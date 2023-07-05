/***************************************************************************
** sound.h
***************************************************************************/

#ifndef _SOUND_H_
#define _SOUND_H

typedef struct {
   int loaded;
   MIDI *data;
} SoundFile;

extern SoundFile loadedSounds[];
extern boolean checkForEnd;
extern int soundEndFlag;

#endif /* _SOUND_H_ */

