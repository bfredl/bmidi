
void cache_clear(void *from, void *to) {
  __builtin___clear_cache(from, to);
}
