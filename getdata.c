#include <alsa/asoundlib.h>
#include "bmidi.h"

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

  while (1) {
    ssize_t res = snd_rawmidi_read(input, buffer, sizeof buffer);
    if (res < 0) {
      fprintf(stderr, "\nvery error: %ld\n", res);
    }
    for (ssize_t i = 0; i < res; i++) {
      printf("%02x ", buffer[i]);
      if ((i>0 && i%8==0) || i == res-1) printf("\n");
    }
  }

}
