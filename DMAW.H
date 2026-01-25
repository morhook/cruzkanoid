#ifndef DMAW_H
#define DMAW_H

#include <dos.h>
#include <stdio.h>

struct WavFilePlay {
  char RetValue;

  FILE *FileToPlay;

  int BufToFill,
      DMAChan8Bit,
      DMAChan16Bit,
      IRQNumber,
      IRQMask,
      MaskSave;

  unsigned char *DMABuffer;
  unsigned int   BytesLeftToPlay;
  unsigned long  BufPhysAddr;

  int LastBufNowPlaying;
  char EofStarted;
  char Done;

  void interrupt (*IRQSave)(void);
};

int start_wav_file(struct WavFilePlay *waveFilePlay, char *filename);
int update_wav_file(struct WavFilePlay *waveFilePlay);
void stop_wav_file(struct WavFilePlay *waveFilePlay);
int play_wav(char *filename);

#endif /* DMAW_H */
