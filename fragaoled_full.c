#include <alsa/asoundlib.h>

#include "bmidi.h"

int main(int argc, char **argv)
{
  if (argc < 2) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *output;

  int status = snd_rawmidi_open(NULL, &output, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }

  int cmd;
  // sscanf(argv[2], "%d", &cmd);

  for (int i = 0; i < 6; i++) {
      //char data[] = {0xf0, 'a', 'b', 'a', 'c', 'u', 's', 0xf7};
      uint8_t data[25] = "";
      data[0] = 0xf0;
      data[1]= 0x7d;
      data[2] = 0x02;
      data[3] = 0x00;
      data[4] = 0x00;
      data[5] = i; // ROW
      data[6] = 0x7f; // COLUMN
      data[7] = 0xf7;
      int len = 8;

      status = snd_rawmidi_write(output, data, len);
      if (status < 0) {
        fprintf(stderr, "no write! %s", snd_strerror(status));
      }
      printf("%d,%d\n", i, 127);
      usleep(1000);
  }

}
