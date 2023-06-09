#include <alsa/asoundlib.h>
#include "bmidi.h"

#include "stdbool.h"
#include "syx_pack.h"
#include "rle_pack2.h"

const char *(blocky[]) = {" ", "▀", "▄", "█"};

// TODO: need an event loop, for now use
// watch -n 0.3 amidi -p hw:2,0,0 -S "F0 7D 02 00 00 F7"

static bool bega = false;

void work(uint8_t *data, int len) {
  const int blk_width = 128;
  uint8_t databuffer[1024];
  if (data[1] != 0x7d || data[2] != 2 || data[3] != 0x40) {
    printf("konstig\n");
    return;
  }
  if (data[4] < 0x02) {
    // data[5]
    int size;
    if (data[4]) {
      size = unpack_7to8_rle(databuffer, sizeof databuffer, data+6, len-7);
    } else {
      size = unpack_sysex_to_8bit(databuffer, sizeof databuffer, data+6, len-7);
    }
    if (bega) {
      fwrite(databuffer, size, 1, stdout);
      fflush(stdout);
      exit(1);
    }
    //printf("%d\n", size);
    printf("\n");
    for (int blk = 0; ; blk++) {
      for (int rstride = 0; rstride < 4; rstride++) {
        printf("\n");
        int mask = 3 << (2*rstride);
        for (int j = 0; j < blk_width; j++) {
          if ((blk*blk_width+j) > size) {
            goto enda;
          }
          int idata = (databuffer[blk*blk_width+j] & mask) >> (2*rstride);
          printf("%s", blocky[idata]);
        }
      }
    }
enda:
    printf("\nsize: %d\n", len-7);
  } else {
    printf("DO NOT DO THAT\n");
  }
}


int main(int argc, char **argv)
{
  if (argc < 2) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *input;
  if (argc >= 3) {
    bega = argv[2][0];
  }

  int status = snd_rawmidi_open(&input, NULL, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }

  uint8_t buffer[2048] = {0};
  int bpos = 0;

  int sysex_start = -1;

  while (1) {
    ssize_t res = snd_rawmidi_read(input, buffer+bpos, (sizeof buffer)-bpos);
    if (res < 0) {
      fprintf(stderr, "\nvery error: %ld\n", res);
      return 1;
    }

    for (ssize_t i = 0; i < res; i++) {
      //printf("%02x ", buffer[bpos+i]);
      //if ((i>0 && i%8==0) || i == res-1 || buffer[i] == 0xf7) printf("\n");
    }

    for (ssize_t i = 0; i < res; i++) {
      if (buffer[bpos+i] == 0xf0) {
        sysex_start = bpos+i;
        // printf("begin: %02x\n", sysex_start);
      } else if (buffer[bpos+i] == 0xf7 && sysex_start >= 0) {
        int elen = bpos+i+1;
        // printf("enda: %02x men %02x\n", elen, elen-sysex_start);
        work(buffer+sysex_start, elen-sysex_start);
        sysex_start = -1;
      }
    }

    if (sysex_start >= 0 && bpos+res < 1024) {
      bpos = bpos+res-sysex_start;
      memmove(buffer, buffer+sysex_start,bpos);
      sysex_start = 0;
    } else {
      bpos = 0;  // :(
      sysex_start = -1;
    }
  }

}
