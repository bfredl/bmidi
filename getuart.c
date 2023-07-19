#include <alsa/asoundlib.h>
#include "bmidi.h"

#include "syx_pack.h"

const char *(blocky[]) = {" ", "▀", "▄", "█"};

void work(uint8_t *data, int len) {
  uint8_t bollbuffer[32];
  if (data[1] != 0x7d || data[2] != 3 || data[3] != 0x40 || len < 6) {
    printf("konstig\n");
    return;
  }
  int lenny = len - 6;
  fwrite(data+5, 1, lenny, stdout);
  fflush(stdout);
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

    if (sysex_start && bpos+res < 512) {
      bpos = bpos+res-sysex_start;
      memmove(buffer, buffer+sysex_start,bpos);
      sysex_start = 0;
    } else {
      bpos = 0;  // :(
      sysex_start = -1;
    }
  }

}
