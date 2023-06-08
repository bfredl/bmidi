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

  //char data[] = {0xf0, 'a', 'b', 'a', 'c', 'u', 's', 0xf7};
  uint8_t data[17] = "\xf0hello sysex\xf7";
  data[0] = 0xf0;
  data[1]= 0x67;
  data[2] = cmd;
  data[3] = pos_low;
  data[4] = pos_high;
  data[5] = x;
  data[6] = 0xf7;

  status = snd_rawmidi_write(output, data, 7);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
  }
}
