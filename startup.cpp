#include <stdint.h>
#include <stddef.h>

void mod_main();

extern void (*__mod_preinit_array_start []) (void);
extern void (*__mod_preinit_array_end []) (void);
extern void (*__mod_init_array_start []) (void);
extern void (*__mod_init_array_end []) (void);

extern "C" void mod_start() {
  size_t count = __mod_preinit_array_end - __mod_preinit_array_start;
  for (size_t i = 0; i < count; i++) {
    __mod_preinit_array_start[i]();
  }

  count = __mod_init_array_end - __mod_init_array_start;
  for (size_t i = 0; i < count; i++) {
    __mod_init_array_start[i]();
  }

  mod_main();
}
