#include "oled.h"
unsigned char data[8] = {0x01, 0x00, 0xa0, 0xe1, 0x1e, 0xff, 0x2f, 0xe1};

extern void mod_main(int* base,int* bytes) {

  unsigned char *loadpos = ((unsigned char *)base)+1024;

  for (int i=0; i < 8; i++) {
    loadpos[i] = data[i];
  }

  auto ptr = (int (*)(int,int))loadpos;

  OLED::popupText(&"0123456789"[ptr(3,5)], true);

}
