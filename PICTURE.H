/***************************************************************************
** picture.h
***************************************************************************/

#ifndef _PICTURE_H_
#define _PICTURE_H_

#define  AGI_GRAPHICS  0
#define  AGI_TEXT      1

typedef struct {
   int loaded;
   unsigned int size;
   char *data;
} PictureFile;

extern PictureFile loadedPictures[];

extern boolean okToShowPic;
extern int screenMode;
extern int min_print_line, user_input_line, status_line_num;
extern boolean statusLineDisplayed, inputLineDisplayed;
extern BITMAP *picture, *priority, *control, *agi_screen, *working_screen;

#endif  /* _PICTURE_H_ */
