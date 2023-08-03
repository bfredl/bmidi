
#include <alsa/asoundlib.h>
#include "pack.h"

int main(int argc, char **argv)
{
  init_crc_table();

  if (argc < 3) return 1;
  char *port_name = argv[1];
  static snd_rawmidi_t *output;
  int status = snd_rawmidi_open(NULL, &output, port_name, 0);

  if (status < 0) {
    fprintf(stderr, "no port! %s", snd_strerror(status));
    return 2;
  }

  FILE *readin = fopen(argv[2], "rb");
  static char buffer[4*1024*1024] = {0};
  int size = fread(buffer, 1, (sizeof buffer), readin);

  int segs = ((size+511)&(~511)) >> 9;
  int total_bytes = segs*512;

  uint32_t crc = get_crc((unsigned char*)buffer, total_bytes);
  printf("\ntransfer size: %d bytes (%d segments)\n", size, segs);
  printf("crc: %d\n", crc);

  uint8_t data[1024];
  for (int seg = 0; seg < segs; seg += 1) {
    int pos_low = seg & 0x7f;
    int pos_high = (seg >> 7) & 0x7f;
    int buf_pos = 512*seg;

    unsigned char *bytes = (unsigned char*)(buffer+buf_pos);

    data[0] = 0xf0;
    data[1] = 0x7d;
    data[2] = 3; // debug
    data[3] = 3; // begaa
    data[4] = 0; // send
    data[5] = pos_low;
    data[6] = pos_high;

		const int packed_size = 586; // ceil(512+512/7)
    pack_8bit_to_7bit(data+7, packed_size, bytes, 512);
    data[7+packed_size] = 0xf7;
    int len = packed_size+8;

    status = snd_rawmidi_write(output, data, len);
    if (status < 0) {
      fprintf(stderr, "no write! %s", snd_strerror(status));
      return 11;
    }
    usleep(300);
  }

  int len_low = segs & 0x7f;
  int len_high = (segs >> 7) & 0x7f;

  data[0] = 0xf0;
  data[1] = 0x7d;
  data[2] = 3; // debug
  data[3] = 3; // bega
  data[4] = 1; // run
  data[5] = len_low;
  data[6] = len_high;

  // TODO: Litle-endian mandated
  pack_8bit_to_7bit(data+7, 5, (uint8_t*)&crc, 4);
  data[7+5] = 0xf7;
  status = snd_rawmidi_write(output, data, 13);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
    return 11;
  }

}


