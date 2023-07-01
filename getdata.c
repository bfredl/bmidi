#include <alsa/asoundlib.h>
#include "bmidi.h"

#include "syx_pack.h"

void work(uint8_t *data, int len) {
  if (data[0] == 0xf0) {
    uint8_t bollbuffer[32];
    int xpos = data[3];
    int ypos = data[4];
    int blk_width = data[5];
    printf("\nXMISSION %d %d w %d OUTER-SIZ %d\n", xpos, ypos, blk_width, len);
    if (blk_width > 32) return;
    unpack_sysex_to_8bit(bollbuffer, 32, data+6, len-7);

    for (int rstride = 0; rstride < 4; rstride++) {
      int mask = 3 << (2*rstride);
      for (int j = 0; j < blk_width; j++) {
        int idata = (bollbuffer[j] & mask) >> (2*rstride);
        printf("%d ", idata);
      }
      printf("\n");
    }

  } else {
    printf("\nFEEEL %d %d OUTER-SIZ %d\n", data[0], data[1], len);
  }
}

int main(int argc, char **argv)
{
  if (argc < 1) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *input;

  int status = snd_rawmidi_open(&input, NULL, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }

  uint8_t buffer[1024] = {0};
  int bpos = 0;

  while (1) {
    ssize_t res = snd_rawmidi_read(input, buffer+bpos, (sizeof buffer)-bpos);
    if (res < 0) {
      fprintf(stderr, "\nvery error: %ld\n", res);
      return 1;
    }

    for (ssize_t i = 0; i < res; i++) {
      // printf("%02x ", buffer[bpos+i]);
      // if ((i>0 && i%8==0) || i == res-1 || buffer[i] == 0xf7) printf("\n");
    }

    for (ssize_t i = 0; i < res; i++) {
      if (buffer[bpos+i] == 0xf7) {
        int elen = bpos+i+1;
        work(buffer, elen);
        memmove(buffer, buffer+elen,res-i-1);
        bpos = 0;
        res = res-i-1;
        i = 0; continue;  // haha control flow go brr
      }
    }

    bpos += res;

    if (bpos > 512) {
      bpos = 0;  // :(
    }
  }

}
