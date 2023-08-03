#include "hid/display/oled.h"
#include "io/debug/module.h"

class MyMod : public LoadableModule {
  void unload() {
    OLED::popupText("good byes", true);
  }

  void activate();
} mymod;


void MyMod::activate() {
  OLED::popupText("Aactivation", true);
}

extern void mod_main() {
  loadable_module = &mymod;
}
