#include <stdint.h>
#include <string.h>
#include "stdio.h"

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
