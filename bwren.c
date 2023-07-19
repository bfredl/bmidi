#include <alsa/asoundlib.h>

#include "bmidi.h"

int main(int argc, char **argv)
{
  if (argc < 3) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *output;

  int status = snd_rawmidi_open(NULL, &output, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }

  char *command = argv[2];

  uint8_t pigga_buffer[1024] = {0xf0, 0x7d, 0x04, 0x00};
  int len = strlen(command);
  if (len > (sizeof pigga_buffer) - 5) {
    return 1;
  }

  memcpy(pigga_buffer+4, command, len);
  pigga_buffer[5+len] = 0xf7;

  status = snd_rawmidi_write(output, pigga_buffer, len+6);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
  }

}

