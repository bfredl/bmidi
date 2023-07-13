#include "stdio.h"
#include "stdlib.h"
#include "syx_pack.h"

#define SIZ 2048
int main(int argc, char **argv)
{
  uint8_t in_buf[SIZ], pack_buf[SIZ], check_buf[SIZ];
  if (argc < 2) return 1;

  int in_size = 0;
  FILE *f = fopen(argv[1], "rb");
  if (!f) return 2;
  in_size = fread(in_buf, 1, SIZ, f);
  printf("original size: %d\n", in_size);
  if (!in_size) return 3;

  int packed_size = pack_8to7_rle(pack_buf, SIZ, in_buf, in_size);
  printf("packed size: %d\n", packed_size);

  if (packed_size <= 0) return 5;

  int unpacked_size = unpack_7to8_rle(check_buf, SIZ, pack_buf, packed_size);
  if (unpacked_size != in_size) {
    printf("unpack size fail: %d\n", unpacked_size);
    return 7;
  }

  if (memcmp(in_buf, check_buf, in_size)) {
    printf("fail!\n");
  } else {
    printf("ok!\n");
  }

}
