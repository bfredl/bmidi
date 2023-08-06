
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

  if (size & 3) {
    printf("weird size :P\n");
    return 1;
  }

  int segs = ((size+511)&(~511)) >> 9;
  int total_bytes = segs*512;

  // TODO: this should use the embedded size (code_end - code_start)
  uint32_t crc = get_crc((unsigned char*)buffer, size);
  printf("transfer size: %d bytes (%d segments)\n", size, segs);
  printf("crc: %x\n", crc);

  uint8_t data[1024];
  for (int seg = 0; seg < segs; seg += 1) {
    int pos_low = seg & 0x7f;
    int pos_high = (seg >> 7) & 0x7f;
    int buf_pos = 512*seg;

    unsigned char *bytes = (unsigned char*)(buffer+buf_pos);

    data[0] = 0xf0;
    data[1] = 0x7d;
    data[2] = 3; // debug
    data[3] = 1; // begaa
    data[4] = pos_low;
    data[5] = pos_high;

		const int packed_size = 586; // ceil(512+512/7)
    pack_8bit_to_7bit(data+6, packed_size, bytes, 512);
    data[6+packed_size] = 0xf7;
    int len = packed_size+7;

    status = snd_rawmidi_write(output, data, len);
    if (status < 0) {
      fprintf(stderr, "no write! %s", snd_strerror(status));
      return 11;
    }
    usleep(10);
  }

  int words = size/4; // 32-bit words
  int len_low = words & 0x7f;
  int len_high = (words >> 7) & 0x7f;
  int len_higher = (words >> 14) & 0x7f;

  data[0] = 0xf0;
  data[1] = 0x7d;
  data[2] = 3; // debug
  data[3] = 2; // run FUL
  data[4] = len_low;
  data[5] = len_high;
  data[6] = len_higher;

  // TODO: Litle-endian mandated
  pack_8bit_to_7bit(data+7, 5, (uint8_t*)&crc, 4);
  data[7+5] = 0xf7;
  status = snd_rawmidi_write(output, data, 13);
  if (status < 0) {
    fprintf(stderr, "no write! %s", snd_strerror(status));
    return 11;
  }

}


