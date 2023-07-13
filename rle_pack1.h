#include "syx_pack.h"

const int MAX_DENSE_SIZE = (63+2);
const int MAX_REP_SIZE = (31+127);
static int pack_8to7_rle(uint8_t *dst, int dst_size, uint8_t *src, int src_len) {
  int d = 0;
  int s = 0;

  int i = 0;  // start of outer (dense) run
  while (s < src_len) {
    // no matter what happens, we are gonna need at least 2 more bytes..
    if (d > dst_size-2) return -1;
    int k = s; // start of inner (repeated) run
    uint8_t val = src[s++];
    while (s < src_len && (s-k) < MAX_REP_SIZE) {
      if (src[s] != val) break;
      s++;
    }
    int dense_size = k-i;
    int rep_size = s-k;
    if (rep_size < 2 || (rep_size < 3 && dense_size > 0)) {
      if (dense_size + rep_size >= MAX_DENSE_SIZE) {
        dst[d++] = MAX_DENSE_SIZE-2;
        d += pack_8bit_to_sysex(dst+d, dst_size-d, src + i, MAX_DENSE_SIZE);
        i += MAX_DENSE_SIZE;
        s = i;
        // printf("dense: %d\n", MAX_DENSE_SIZE);
        continue;
      } else {
        // if there is more data, reconsider after next run
        if (s < src_len) continue;
      }
    }

    if (dense_size > 0) {
      if (dense_size == 1) {
        // printf("repeat: %d\n", dense_size);
        dst[d++] = 64 + (1<<1) + ((src[i]&0x80)?1:0);
        dst[d++] = src[i]&0x7f;
      } else {
        // printf("dense: %d\n", dense_size);
        dst[d++] = dense_size-2;
        d += pack_8bit_to_sysex(dst+d, dst_size-d, src + i, dense_size);
      }
    }

    // printf("repeat: %d\n", rep_size);
    if (d > dst_size-2-(rep_size >= 31)) return -2;
    int first = 64 + ((val&0x80)?1:0);
    if (rep_size < 31) {
      dst[d++] = first + (rep_size<<1);
    } else {
      dst[d++] = first + (31<<1);
      dst[d++] = (rep_size-31);
    }
    dst[d++] = val&0x7f;
    i = s;
  }
  return d;
}

static int unpack_7to8_rle(uint8_t *dst, int dst_size, uint8_t *src, int src_len) {
  int d = 0;
  int s = 0;

  while (s+1 < src_len) {
    uint8_t first = src[s++];
    if (first < 64) {
      int out_size = first+2;
      printf("dense: %d\n", out_size);
      int in_size = out_size + (out_size+6)/7;
      if (in_size > src_len-s) {
        printf("s: %d, d: %d, first: %d\n", s, d, first);
        return -1;
      }
      if (out_size > dst_size-d) return -11;
      int unpacked = unpack_sysex_to_8bit(dst+d, out_size, src+s, in_size);
      if (unpacked != out_size) return -2;
      d += out_size;
      s += in_size;
    } else {
      // first = 64 + (runlen<<1) + highbit
      first = first-64;
      int high = (first&1);
      int runlen = first >> 1;
      if (runlen == 31) {
        runlen = 31 + src[s++];
        if (s == src_len) return -3;
      }
      int byte = src[s++] + 128*high;
      // printf("repeat: %d (%d)\n", runlen, byte);
      if (runlen > dst_size-d) return -12;
      memset(dst+d, byte, runlen);
      d += runlen;
    }
  }
  return d;
}
