#include "oled.h"
#include "fatfs/ff.h"
#include "GeneralMemoryAllocator.h"
#include "mtu_all_cpus.h"

extern "C" void EXEC_BASE(void);
void chainloader(char *to, char *from, int size);
#define UNCACHED_MIRROR_OFFSET 0x40000000
void mod_main(int *base, int *bytta) {
  const char *path = "IMAGES/chain.bin";

  FILINFO fno;

  int result = f_stat(path, &fno);
  FSIZE_t fileSize = fno.fsize;

  FIL currentFile;
  // Open the file
  result = f_open(&currentFile, path, FA_READ);
  if (result != FR_OK) {
    OLED::popupText("FEL1", true);
    return;
  }

  UINT numBytesRead;
  uint8_t* buffer = (uint8_t*)generalMemoryAllocator.alloc(fileSize, NULL, false, true);
  result = f_read(&currentFile, buffer, fileSize, &numBytesRead);
  if (!buffer) {
    OLED::popupText("FEL2", true);
    return;
  }

  char* funcbuf = (char *)0x20060700;
  char* from = (char *)chainloader;
  for (int i = 0; i < 128; i++) {
    (funcbuf+UNCACHED_MIRROR_OFFSET)[i] = from[i];
  }

  void (*ptr)(char *, char *, int) = (void (*)(char *, char *, int))funcbuf;

#if 0
  // STOP, HAMMER TIME
  volatile int *heap = base;
  int hammerfield;
  for (int i = 0; i < 8192; i ++) {
    hammerfield += heap[i];
  }
  bytta[0] = hammerfield;
  if (true) {
    OLED::popupText("HAMMERTIME 1", true);
    return;
  }
#endif

  disableTimer(TIMER_MIDI_GATE_OUTPUT);
  disableTimer(TIMER_SYSTEM_SLOW);
  disableTimer(TIMER_SYSTEM_FAST);
  disableTimer(TIMER_SYSTEM_SUPERFAST);
  ptr((char *)&EXEC_BASE, (char *)buffer, fileSize);

}

void chainloader(char *to, char *from, int size) {
  for (int i = 0; i < size; i++) {
    (to+UNCACHED_MIRROR_OFFSET)[i] = from[i];
  }

#if 0
  volatile int *heap = (int *)from;
  int hammerfield;
  for (int i = 0; i < 8192; i ++) {
    hammerfield += heap[i];
  }
  from[0] = hammerfield;  // YAGNI
#endif

  void (*ptr)(void) = (void (*)(void))to;
  ptr(); // lessgo
}

