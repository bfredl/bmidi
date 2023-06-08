#include <alsa/asoundlib.h>


int main(int argc, char **argv)
{
  if (argc < 2) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *input;
  static snd_rawmidi_t *output;

  int status = snd_rawmidi_open(&input, &output, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }

  //char data[] = {0xf0, 'a', 'b', 'a', 'c', 'u', 's', 0xf7};
  char data[] = "\xf0hello sysex\xf7";

  status = snd_rawmidi_write(output, data, sizeof(data));
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
  }
}
