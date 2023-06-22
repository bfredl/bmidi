#include "oled.h"
#include <RootUI.h>
#include <new>
#include "extern.h"
#include "soundeditor.h"
#include <soundinstrument.h>
#include "song.h"
#include "InstrumentClip.h"
#include "NoteRow.h"
#include "ModelStack.h"
#include "kit.h"

class Dx7UI final : public UI {
public:
  Dx7UI() {};
  int padAction(int x, int y, int velocity);
  int buttonAction(int x, int y, bool on, bool inCardRoutine);

#if HAVE_OLED
  void renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]);
#endif

  bool did_button = false;
};

int Dx7UI::padAction(int x, int y, int velocity) {
  if (x >= displayWidth) return ACTION_RESULT_DEALT_WITH;

  if (sdRoutineLock && !allowSomeUserActionsEvenWhenInCardRoutine) {
    return ACTION_RESULT_REMIND_ME_OUTSIDE_CARD_ROUTINE; // Allow some of the time when in card routine.
  }

  return ACTION_RESULT_DEALT_WITH;
}

#if HAVE_OLED
void Dx7UI::renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]) {
  const char *text = did_button ? "buton" : "Halloj";
  OLED::drawString(text, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
}
#endif

int Dx7UI::buttonAction(int x, int y, bool on, bool inCardRoutine) {
  if (x == clipViewButtonX && y == clipViewButtonY) {
    did_button = true;
    renderUIsForOled();
    return ACTION_RESULT_DEALT_WITH;
  } else if (x == backButtonX && y == backButtonY) {
    close();
  }
  return ACTION_RESULT_DEALT_WITH;
}

static Dx7UI dx7ui;

extern void mod_main(int*,int*) {
  new (&dx7ui) Dx7UI;
  // This (with RootUI parent class) doesn't work
  // changeRootUI(&dx7ui);

  openUI(&dx7ui);
}
