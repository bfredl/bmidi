int mod_main();

extern "C" int mod_start() {
  // TODO: ctors
  return mod_main();
}
