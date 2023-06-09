#include <alsa/asoundlib.h>
#include "bmidi.h"

int main(int argc, char **argv)
{
  if (argc < 3) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *output;

  int exec = 0;

  int pos_off;
  if (!strcmp(argv[2], "exec")) {
    exec = 1;
    pos_off = 0;
  } else {
    sscanf(argv[2], "%d", &pos_off);
    printf("offset: 16*%d=%d\n", pos_off, pos_off*16);
  }

  int status = snd_rawmidi_open(NULL, &output, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }


  FILE *readin = stdin;
  if (argc >= 4) {
    readin = fopen(argv[3], "rb");
    if (!readin) return 10;
  }

  char buffer[4096*4] = {0};
  int size = fread(buffer, 1, 4096*4, readin);
  int segs = ((size+15)&(~15)) >> 4;
  printf("\ntransfer size: %d bytes (%d segments)\n", size, segs);

  uint32_t hash = __ac_X31_hash_string((unsigned char*)buffer, size);
  printf("hash: %d\n", hash);

  uint8_t data[25] = "\xf0hello sysex\xf7";
  int len = 0;

  data[0] = 0xf0;
  data[1] = 0x67;
  data[2] = 25;  // turn off module timer
  data[3] = 0;
  data[4] = 0;
  data[5] = 0;
  data[6] = 0xf7;
  len = 7;
  status = snd_rawmidi_write(output, data, len);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
  }

  for (int seg = 0; seg < segs; seg += 1) {
    int pos_low = seg & 0x7f;
    int pos_high = (seg >> 7) & 0x7f;
    int buf_pos = 16*seg;

    unsigned char *bytes = (unsigned char*)(buffer+buf_pos);

    data[0] = 0xf0;
    data[1]= 0x67;
    data[2] = 0; // WRITE
    data[3] = pos_low;
    data[4] = pos_high;

    bytes_to_msg(data, bytes);
    len = 25;

    status = snd_rawmidi_write(output, data, len);
    usleep(10);
  }
  
  data[0] = 0xf0;
  data[1]= 0x67;
  data[2] = 24; // HASH
  data[3] = 0; // unused, we use bytes
  data[4] = 0; //
  uint32_t ibytes[4] = {0};
  
  ibytes[0] = pos_off*16;  // start
  ibytes[1] = size;

  bytes_to_msg(data, (unsigned char *)ibytes);
  len = 25;

  status = snd_rawmidi_write(output, data, len);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
    return 11;
  }

  if (!exec) return 0;

  printf("hash ok? ret=yes, ctrl-c=no\n");
  getc(stdin);

  usleep(100);
  data[2] = 1;
  data[3] = 0;
  data[4] = 0;
  data[5] = 0;
  data[6] = 0xf7;
  len = 7;
  status = snd_rawmidi_write(output, data, len);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
  }
}
