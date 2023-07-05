/* PLAY.C */

/* Original program designed by Jens Christian Restemeier to convert */
/* AGI sounds to MIDI. */

/* Program slighly adjusted by Lance Ewing to play the MIDI file after */
/* it has been converted. It uses the Allegro library to do this. */

/* Note: This program was written before we worked out the details on the */
/* fourth voice and the volume control. */


/* ORIGINAL COMMENT */

/* A very simple program, that converts an AGI-song into a MIDI-song. */
/* Feel free to use it for anything. */

/* To compile (using DJGPP for MS/DOS) type: */
/*   gcc -lm -O3 snd2midi.c -o snd2midi */
/* if you've got problems on unix, type: */
/*   gcc -lm -O3 -DUNIX snd2midi.c -o snd2midi */

/* The default instrument is "piano" for all the channels, what gives */
/* good results on most games. But I found, that some songs are interesting */
/* with other instruments. If you want to experiment, modify the "instr" */
/* array. */

/* Timing is not perfect, yet. It plays correct, when I use the */
/* Gravis-Midiplayer, but the songs are too fast when I use playmidi on */
/* Linux. */

/* (A Wavetable-card is handy, here :-) */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <allegro.h>

unsigned char *snddata;

/* channel / intrument setup: */

/* most songs are good with this: */
unsigned char instr[] = {0, 0, 0};

/* cool for sq2:
unsigned char instr[] = {50, 51, 19}; */


void write_byte(FILE *f, long data)
{
   fwrite(&data,1,1,f);
}

void write_word(FILE *f, long data)
{
   write_byte(f,(data>>8)&255);
   write_byte(f,data&255);
}

void write_long(FILE *f, long data)
{
   write_byte(f,(data>>24)&255);
   write_byte(f,(data>>16)&255);
   write_byte(f,(data>>8)&255);
   write_byte(f,data&255);
}

void write_delta(FILE *f,long delta)
{
   long i;
   i=delta>>21; if (i>0) write_byte(f,(i&127)|128);
   i=delta>>14; if (i>0) write_byte(f,(i&127)|128);
   i=delta>>7;  if (i>0) write_byte(f,(i&127)|128);
   write_byte(f,delta&127);
}

void write_midi(FILE *f)
{
   long lp, ep;
   int n;
   double ll;

   ll=log10(pow(2.0,1.0/12.0));

   /* Header */
   fwrite("MThd",4,1,f);
   write_long(f,6);
   write_word(f,1);    /* mode */
   write_word(f,3);    /* number of tracks */
   write_word(f,192);  /* ticks / quarter */

   for (n=0; n<3; n++) {
      unsigned short start, end, pos;
        
      fwrite("MTrk",4,1,f);
      lp=ftell(f);
      write_long(f,0);        /* chunklength */
      write_delta(f,0);       /* set instrument */
      write_byte(f,0xc0+n);
      write_byte(f,instr[n]);
      start=snddata[n*2+0] | (snddata[n*2+1]<<8);
      end=((snddata[n*2+2] | (snddata[n*2+3]<<8)))-5;

      for (pos=start; pos<end; pos+=5) {
         unsigned short freq, dur;
         dur=(snddata[pos+0] | (snddata[pos+1]<<8))*6;
         freq=((snddata[pos+2] & 0x3F) << 4) + (snddata[pos+3] & 0x0F);
         if (snddata[pos+2]>0) {
            double fr;
            int note;
            /* I don't know, what frequency equals midi note 0 ... */
            /* This moves the song 4 octaves down: */
            fr=(log10(111860.0 / (double)freq) / ll) - 48;
            note=floor(fr+0.5);
            if (note<0) note=0;
            if (note>127) note=127;
            /* note on */
            write_delta(f,0);
            write_byte(f,144+n);
            write_byte(f,note);
            write_byte(f,100);
            /* note off */
            write_delta(f,dur);
            write_byte(f,128+n);
            write_byte(f,note);
            write_byte(f,0);
         } else {
            /* note on */
            write_delta(f,0);
            write_byte(f,144+n);
            write_byte(f,0);
            write_byte(f,0);
            /* note off */
            write_delta(f,dur);
            write_byte(f,128+n);
            write_byte(f,0);
            write_byte(f,0);
         }
      }
      write_delta(f,0);
      write_byte(f,0xff);
      write_byte(f,0x2f);
      write_byte(f,0x0);
      ep=ftell(f);
      fseek(f,lp,SEEK_SET);
      write_long(f,(ep-lp)-4);
      fseek(f,ep,SEEK_SET);
   }
}

/* MS/DOS requires "b"-mode, but I heard, that some unix-systems don't */
/* like it. */
#ifndef UNIX
#define O_RB "rb"
#define O_WB "wb"
#else
#define O_RB "r"
#define O_WB "w"
#endif

void main(int argc, char *argv[])
{
   FILE *fsnd, *fmid;
   MIDI *the_music;
   long size;

   if (argc<2) {
      printf("play <sndfile>\n");
   }
   else {
      if ((fsnd=fopen(argv[argc-1],"rb"))!=NULL) {
         /* Read SOUND file into snddata buffer */
         fseek(fsnd,0,SEEK_END);
         size=ftell(fsnd);
         fseek(fsnd,0,SEEK_SET);
         snddata=malloc(size);
         fread(snddata,1,size,fsnd);
         fclose(fsnd);

         /* Open MIDI file and convert SND to MIDI */
         if ((fmid=fopen("TEMPFILE.MID","wb"))!=NULL) {
            write_midi(fmid);
            fclose(fmid);
            //printf("ok !\n");
         }
         else
            printf("Error opening output !\n");

         free(snddata);

      }
      else
         printf("Error opening input !\n");

      allegro_init();
      install_keyboard();
      install_timer();

      if (install_sound(DIGI_NONE, MIDI_AUTODETECT, NULL) != 0) {
         printf("Error initialising sound card.\n%s\n", allegro_error);
         exit(0);
      }

      the_music = load_midi("TEMPFILE.MID");

      play_midi(the_music, TRUE);
      readkey();
      destroy_midi(the_music);
      remove("TEMPFILE.MID");

      allegro_exit();
   }
}
