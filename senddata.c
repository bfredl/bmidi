#include <alsa/asoundlib.h>

int main(int argc, char **argv)
{
  if (argc < 3) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *input;
  static snd_rawmidi_t *output;

  int pos_off;
  sscanf(argv[2], "%d", &pos_off);
  printf("offset: 8*%d=%d\n", pos_off, pos_off*8);

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
  int segs = ((size+7)&(~7)) >> 3;
  printf("that's segments %d\n", segs);

  for (int seg = 0; seg < segs; seg += 1) {
    int pos_low = seg & 0x7f;
    int pos_high = (seg >> 7) & 0x7f;

    int buf_pos = 8*seg;

    unsigned char *bytes = (unsigned char*)(buffer+buf_pos);

    uint8_t data[17] = "\xf0hello sysex\xf7";
    data[0] = 0xf0;
    data[1]= 0x67;
    data[2] = 0; // WRITE
    data[3] = pos_low;
    data[4] = pos_high;
    int len = 0;

    int hi = 0;
    for (int i = 0; i < 7; i++) {
      data[5+i] = bytes[i] & 0x7f;
      hi |= (bytes[i]&0x80) ? (1<<i) : 0;
    }
    data[5+7] = bytes[7] & 0x7f;
    data[13] = hi;
    data[14] = (bytes[7] & 0x80) ? 1 : 0;
    data[15] = 42; //not used :P
    data[16] = 0xf7;
    len = 17;

    status = snd_rawmidi_write(output, data, len);
    usleep(10);
  }
  

}
