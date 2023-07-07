#include <alsa/asoundlib.h>
#include "bmidi.h"

#include "stdbool.h"
#include "syx_pack.h"

const char *(blocky[]) = {" ", "▀", "▄", "█"};

// TODO: need an event loop, for now use
// watch -n 0.3 amidi -p hw:2,0,0 -S "F0 7D 02 00 00 F7"

static bool bega = false;

void work(uint8_t *data, int len) {
  const int blk_width = 128;
  uint8_t bollbuffer[1024];
  if (data[1] != 0x7e && data[2] != 2 && data[3] != 0x40) {
    printf("konstig\n");
    return;
  }
  if (data[4] == 0x00) {
    int ypos = data[5];
    // printf("\nXMISSION %d %d w %d OUTER-SIZ %d\n", -7, ypos, blk_width, len);
    int kniff = unpack_sysex_to_8bit(bollbuffer, blk_width, data+6, len-7);
    // printf("galong %d %d %d\n", ypos, data[6], kniff);

    for (int rstride = 0; rstride < 4; rstride++) {
      printf("\033[%d;%dH", (4*ypos+rstride)+1, 1);
      int mask = 3 << (2*rstride);
      for (int j = 0; j < blk_width; j++) {
        int idata = (bollbuffer[j] & mask) >> (2*rstride);
        printf("%s", blocky[idata]);
      }
    }
  } else if (data[4] == 0x01) {
    // data[5]
    int kniff = unpack_sysex_to_8bit(bollbuffer, sizeof bollbuffer, data+6, len-7);
    if (bega) {
      fwrite(bollbuffer, kniff, 1, stdout);
      fflush(stdout);
      exit(1);
    }
    //printf("%d\n", kniff);
    for (int blk = 0; ; blk++) {
      for (int rstride = 0; rstride < 4; rstride++) {
        printf("\n");
        int mask = 3 << (2*rstride);
        for (int j = 0; j < blk_width; j++) {
          if ((blk*blk_width+j) > kniff) {
            return;
          }
          int idata = (bollbuffer[blk*blk_width+j] & mask) >> (2*rstride);
          printf("%s", blocky[idata]);
        }
      }
    }
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
