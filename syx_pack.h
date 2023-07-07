#include <stdint.h>
#include <string.h>

// referece: last page of any Sequential synth manual, lol
//
// I made it so that any lenght is handled propery.
// you probably want to encode the full message length as a prefix just to be
// Sure™ anyway.

static int pack_8bit_to_sysex(uint8_t *dst, int dst_size, uint8_t *src, int src_len) {
  int packets = (src_len+6)/7;
  int missing = (7*packets-src_len);  // allow incomplete packets
  int out_len = 8*packets-missing;
  if (out_len > dst_size) return 0;

  for (int i = 0; i < packets; i++) {
    int ipos = 7*i;
    int opos = 8*i;
    memset(dst+opos, 0, 8);
    for (int j = 0; j < 7; j++) {
      // incomplete packet
      if (!(ipos+j < src_len)) break;
      dst[opos+1+j] = src[ipos+j] & 0x7f;
      if (src[ipos+j] & 0x80) {
        dst[opos] |= (1<<j);
      }
    }
  }
  return out_len;
}

static int unpack_sysex_to_8bit(uint8_t *dst, int dst_size, uint8_t *src, int src_len) {
  int packets = (src_len+7)/8;
  int missing = (8*packets-src_len);
  if (missing == 7) { // this would be weird
    packets--;
    missing = 0;
  }
  int out_len = 7*packets-missing;
  if (out_len > dst_size) return 0;
  for (int i = 0; i < packets; i++) {
    int ipos = 8*i;
    int opos = 7*i;
    memset(dst+opos, 0, 7);
    for (int j = 0; j < 7; j++) {
      if (!(j+1+ipos < src_len)) break;
      dst[opos+j] = src[ipos+1+j] & 0x7f;
      if (src[ipos] & (1<<j)) {
        dst[opos+j] |= 0x80;
      }
    }
  }
  return out_len;
}

static int pack_8to7_rle(uint8_t *dst, int dst_size, uint8_t *src, int src_len) {
  int d = 0;
  int s = 0;

}

static int unpack_7to8_rle(uint8_t *dst, int dst_size, uint8_t *src, int src_len) {
  int d = 0;
  int s = 0;

  while (s+1 < src_len) {
    uint8_t first = src[s++];
    if (first < 64) {
      int out_size = first+2;
      int in_size = out_size + (out_size+6)/7;
      if (in_size > src_len-s) return -1;
      if (out_size > dst_size-d) return -11;
      int unpacked = unpack_sysex_to_8bit(dst+d, out_size, src+s, in_size);
      if (unpacked != out_size) return -2;
      d += out_size;
      s += in_size;
    } else {
      first = first-64;
      int high = (first&1);
      int runlen = first >> 1;
      if (runlen == 31) {
        runlen = 32 + src[s++];
        if (s == src_len) return -3;
      }
      int byte = src[s++] + 128*high;
      if (runlen > dst_size-d) return -12;
      memset(dst+d, byte, runlen);
      dst += d;
    }
  }
  return d;
}
