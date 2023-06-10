
// overwrites the entire flag byte!
static inline void bytes_to_msg(unsigned char msg[25], unsigned char bytes[16]) {
  int hi1 = 0, hi2 = 0;
    for (int i = 0; i < 7; i++) {
      msg[5+i] = bytes[i] & 0x7f;
      msg[5+i+8] = bytes[i+8] & 0x7f;
      hi1 |= (bytes[i]&0x80) ? (1<<i) : 0;
      hi2 |= (bytes[i+8]&0x80) ? (1<<i) : 0;
    }
    msg[5+7] = bytes[7] & 0x7f;
    msg[5+7+8] = bytes[7+8] & 0x7f;
    int f = 0;
    f |= (1<<0)* ((bytes[7] & 0x80) ? 1 : 0);
    f |= (1<<1)* ((bytes[15] & 0x80) ? 1 : 0);
    msg[5+16] = hi1;
    msg[5+17] = hi2;
    msg[5+18] = f;
    msg[5+19] = 0xf7;
}

static inline uint32_t __ac_X31_hash_string(const unsigned char *s, int len)
{
  if (!len) return 0;
  uint32_t h = (uint32_t)*s;
  for (int i = 1 ; i < len; ++i) {
    h = (h << 5) - h + (uint32_t)s[i];
  }
  return h;
}

