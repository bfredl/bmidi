
#include <alsa/asoundlib.h>
#include "bmidi.h"

int main(int argc, char **argv)
{
  if (argc < 3) return 1;
  char buffer[4096*4] = {0};
  int size = fread(buffer, 1, 4096*4, stdin);
  printf("lesa: %d\n", size);
  int off, siz;
  sscanf(argv[1], "%d", &off);
  sscanf(argv[2], "%d", &siz);
  printf("off: %d, siz %d\n", off, siz);
  if (siz > size) return 12;

  unsigned int hash = __ac_X31_hash_string(buffer+off, siz);
  printf("hash in dec: %u vs %d\n", hash, hash);
  printf("hash in hex: %08x\n", hash);
}
