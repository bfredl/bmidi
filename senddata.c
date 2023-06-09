#include <alsa/asoundlib.h>
#include "bmidi.h"

int main(int argc, char **argv)
{
  if (argc < 3) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *input;
  static snd_rawmidi_t *output;

  int pos_off;
  sscanf(argv[2], "%d", &pos_off);
  printf("offset: 16*%d=%d\n", pos_off, pos_off*16);

  int status = snd_rawmidi_open(&input, &output, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }


  FILE *readin = stdin;
  if (argc >= 4) {
    readin = fopen(argv[3], "rb");
    if (!readin) return 10;
  }

  char buffer[4096*4];
  int size = fread(buffer, 1, 4096*4, readin);
  printf("we are gonna transfer up to %d\n", size);
  int segs = ((size+15)&(~15)) >> 4;
  printf("that's segments %d\n", segs);

  for (int seg = 0; seg < segs; seg += 1) {
    int pos_low = seg & 0x7f;
    int pos_high = (seg >> 7) & 0x7f;
    int buf_pos = 16*seg;

    unsigned char *bytes = (unsigned char*)(buffer+buf_pos);

    uint8_t data[25] = "\xf0hello sysex\xf7";
    data[0] = 0xf0;
    data[1]= 0x67;
    data[2] = 0; // WRITE
    data[3] = pos_low;
    data[4] = pos_high;
    int len = 0;

    bytes_to_msg(data, bytes);
    len = 25;

    status = snd_rawmidi_write(output, data, len);
    usleep(10);
  }
  

}
