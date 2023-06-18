#include "oled.h"

// mov r0, r1; bx lr
unsigned char data[8] = {0x01, 0x00, 0xa0, 0xe1, 0x1e, 0xff, 0x2f, 0xe1};

#define UNCACHED_MIRROR_OFFSET 0x40000000
extern "C" void v7_dma_flush_range(unsigned char *start, unsigned char *end);

extern void mod_main(int* base,int* bytes) {

  unsigned char *loadpos = ((unsigned char *)base)+1024;
  unsigned char *writepos = loadpos + UNCACHED_MIRROR_OFFSET;

  for (int i=0; i < 8; i++) {
    writepos[i] = data[i];
  }

  //v7_dma_flush_range(loadpos,loadpos+32);
  auto ptr = (int (*)(int,int))loadpos;

  // STOP, HAMMER TIME
#if 0
  volatile int *heap = base;
  int hammerfield;
  for (int i = 0; i < 8192; i ++) {
    hammerfield += heap[i];
  }
  bytes[0] = hammerfield;
#endif

  OLED::popupText(&"0123456789"[ptr(3,5)], true);

}
