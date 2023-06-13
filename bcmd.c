
#include <alsa/asoundlib.h>

#include "bmidi.h"

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

  int cmd, pos, x = 0, y = 0;
  char *cname = argv[2];
  char *addrform = argv[3];

  if (!strcmp(cname, "exec")) {
    cmd = 1; //
  } else if (!strcmp(cname, "execnr")) {
    cmd = 2; // int output
  } else if (!strcmp(cname, "peek")) {
    cmd = 17;
  } else if (!strcmp(cname, "ppos")) {
    cmd = 18;
  }

  if (!addrform[0]) {
    return 17;
  }

  int addr = 0;
  sscanf(&addrform[1], "%d", &addr);

  int  addr_x = 0;
  if (addrform[0] == 'B') { 
    // block
    pos = addr;
  } else if (addrform[0] == 'c') {
    pos = addr >> 4;
    addr_x = addr & 15;
    // char
  } else {
    return 10;
  }

  int is_exec = 0;
  if (cmd == 17 || cmd == 18) {
    x = addr_x;
  } else if (cmd >= 1 && cmd < 16) {
    is_exec = 1;
    y = addr_x;
    if (y & 3) {
      return 3;  // u wat
    }
  }

  int has_bytes = 0;
  uint8_t bytes[16] = {0};
  if (argc >= 5) {
    if (!is_exec) return 27;  // TODO: inte såå
    has_bytes = 1;

    char *byte_form = argv[4];
    if (!strcmp(byte_form, "int")) {
      int iargc = argc-5;
      if (iargc < 1 || iargc > 4) return 55;
      for (int i = 0; i < iargc; i++) {
        int datta;
        sscanf(argv[5+i], "%d", &datta);
        memcpy(bytes+4*i, &datta, 4);
        // printf("bytta %d: %d\n", i, datta);
      }
    } else {
      return 75;
    }
  }

  int pos_low = pos & 0x7f;
  int pos_high = (pos >> 7) & 0x7f;

  printf("cmd: %d, the split: %d %d, x %d, y %d\n", cmd, pos_low, pos_high, x, y);
  uint8_t data[25] = "\xf0hello sysex\xf7";
  int len;
  data[0] = 0xf0;
  data[1]= 0x67;
  data[2] = cmd;
  data[3] = pos_low;
  data[4] = pos_high;
  data[5] = x;
  if (is_exec) {
    if (has_bytes) {
      bytes_to_msg(data, bytes);
      data[23] |= addr_x;  // twiddly!
      len = 25;
    } else {
      data[6] = y;
      data[7] = 0xf7;
      len = 8;
    }
  } else {
    data[6] = 0xf7;
    len = 7;
  }

  status = snd_rawmidi_write(output, data, len);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
  }
}
