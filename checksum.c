#include <stdint.h>
#include "bmidi.h"

void static inline mcpy(char *dest, char* src, unsigned int len) {
  for (int i = 0; i < len; i++) dest[i] = src[i];
}

int checksum(char *mem, char* buf) {
  //int param[2];
  int *param = (int *)buf;
  unsigned int hask = __ac_X31_hash_string(mem+param[0], param[1]);
  return (int)hask;
}
