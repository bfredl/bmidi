#include <alsa/asoundlib.h>


int main(int argc, char **argv)
{
  if (argc < 4) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *input;
  static snd_rawmidi_t *output;

  int status = snd_rawmidi_open(&input, &output, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }

  int cmd, pos, x = 0;
  sscanf(argv[2], "%d", &cmd);
  sscanf(argv[3], "%d", &pos);
  if (argc >= 5) sscanf(argv[4], "%d", &x);

  cmd = cmd & 0x7f;
  int pos_low = pos & 0x7f;
  int pos_high = (pos >> 7) & 0x7f;
  x = x & 0x7f;
  printf("cmd: %d, the split: %d %d, x %d\n", cmd, pos_low, pos_high, x);


  uint8_t bytes[8] = {0};
  int has_bytes = 0;
  if (argc >= 12) {
    has_bytes = 1;
    for (int i = 0; i < 8; i++) {
      int byt;
      sscanf(argv[4+i], "%d", &byt);
      bytes[i]=byt;
    }
  }

  //char data[] = {0xf0, 'a', 'b', 'a', 'c', 'u', 's', 0xf7};
  uint8_t data[17] = "\xf0hello sysex\xf7";
  data[0] = 0xf0;
  data[1]= 0x67;
  data[2] = cmd;
  data[3] = pos_low;
  data[4] = pos_high;
  int len = 0;

  if (has_bytes) {
    printf("byten: %d %d\n", bytes[0], bytes[7]);
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
  } else {
    data[5] = x;
    data[6] = 0xf7;
    len = 7;
  }

  status = snd_rawmidi_write(output, data, len);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
  }
}
