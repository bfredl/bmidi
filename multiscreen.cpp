#include "oled.h"
#include <RootUI.h>
#include <new>

class MultiScreen final : public UI {
public:
  MultiScreen() {};
  // int padAction(int x, int y, int velocity);
  int buttonAction(int x, int y, bool on, bool inCardRoutine);

#if HAVE_OLED
  void renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]);
#endif

  bool did_button = false;
};

#if HAVE_OLED
void MultiScreen::renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]) {
  const char *text = did_button ? "buton" : "Halloj";
  OLED::drawString(text, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
}
#endif

int MultiScreen::buttonAction(int x, int y, bool on, bool inCardRoutine) {
  if (x == clipViewButtonX && y == clipViewButtonY) {
    did_button = true;
    renderUIsForOled();
    return ACTION_RESULT_DEALT_WITH;
  } else if (x == backButtonX && y == backButtonY) {
    close();
  }
  return ACTION_RESULT_DEALT_WITH;
}

static MultiScreen multiscreen;

extern void mod_main(int*,int*) {
  new (&multiscreen) MultiScreen;
  // This (with RootUI parent class) doesn't work
  // changeRootUI(&multiscreen);

  openUI(&multiscreen);
}
