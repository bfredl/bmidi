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
#include "KeyboardScreen.h"
#include "Buttons.h"
#include "dexed/engine.h"

class Dx7UI final : public UI {
public:
  Dx7UI() {};
  int padAction(int x, int y, int velocity);
  int buttonAction(int x, int y, bool on, bool inCardRoutine);
	void selectEncoderAction(int8_t offset);


	bool renderSidebar(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
	                   uint8_t occupancyMask[][displayWidth + sideBarWidth]);
#if HAVE_OLED
  void renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]);
#endif

  int potentialShortcutPadAction(int x, int y, int on);

  bool did_button = false;
  bool editing = false;
  int param = 0;
};

int Dx7UI::padAction(int x, int y, int on) {
  if (x == displayWidth && on && y > 1) {
    int op = 8-y-1; // op 0-5
    char* val = &Dexed::dummy_controller.opSwitch[op];
    *val = (*val == '0' ? '1' : '0');
    uiNeedsRendering(this, 0x00000000, 0xFFFFFFFF);
  }

  if (x >= displayWidth) return ACTION_RESULT_DEALT_WITH;

		int result = potentialShortcutPadAction(x, y, on);
		if (result != ACTION_RESULT_NOT_DEALT_WITH) return result;

	if (getRootUI() == &keyboardScreen) {
		if (x < displayWidth) {
			keyboardScreen.padAction(x, y, on);
			return ACTION_RESULT_DEALT_WITH;
		}
	}

  if (sdRoutineLock && !allowSomeUserActionsEvenWhenInCardRoutine) {
    return ACTION_RESULT_REMIND_ME_OUTSIDE_CARD_ROUTINE; // Allow some of the time when in card routine.
  }

  return ACTION_RESULT_NOT_DEALT_WITH;
}

int Dx7UI::potentialShortcutPadAction(int x, int y, int on) {
  if (!on || x >= displayWidth || !Buttons::isShiftButtonPressed()) {
    return ACTION_RESULT_NOT_DEALT_WITH;
  }

  if (y > 1) {
    int op = 8-y-1; // op 0-5
    int ip = 0;
    if (x < 8) {
      ip = x;
    } else if (x < 15) {
      ip = (x-8)+16;
    }
    param = 21*op+ip;
    editing = true;
  } else  if (y==0 && x < 10) {
    param = 6*21+x;
    editing = true;
  }

  renderUIsForOled();
  return ACTION_RESULT_DEALT_WITH;
}

void Dx7UI::selectEncoderAction(int8_t offset) {
  if (!editing) return;

  Dx7Patch &p = Dexed::globalPatch;
  int newval = p.currentPatch[param]+offset;
  if (newval > 127) newval = 127;
  if (newval < 0) newval = 0;
  p.currentPatch[param] = newval;
  p.update(p.currentPatch);
  renderUIsForOled();

}

#if HAVE_OLED

const char *desc[] = {
  "env rate1", "env rate2", "env rate3", "env rate4",
  "env level1", "env level2", "env level3", "env level4",
  "sc1", "sc2", "sc3", "sc4", "sc5", "rate_scale", "ampmod", "velocity sens",
  "level", "mode", "coarse", "fine", "detune"
};

const char *desc_global[] {
  "pitch rate1", "pitch rate2", "pitch rate3", "pitch rate4",
  "pitch level1", "pitch level2", "pitch level3", "pitch level4",
  "algoritm", "feedback",
};


bool Dx7UI::renderSidebar(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
                                       uint8_t occupancyMask[][displayWidth + sideBarWidth]) {
	if (!image) return true;

	for (int i = 0; i < displayHeight; i++) {
		if (whichRows & (1 << i)) {
      uint8_t* thisColour = image[i][displayWidth];
      int op = 8-i-1; // op 0-5

      bool muted = false;
      if (op < 6) {
        char* val = &Dexed::dummy_controller.opSwitch[op];
        muted = (*val == '0');
      }

      if (muted) {
        thisColour[0] = 255;
      } else {
        thisColour[0] = 0;
      }
      thisColour[1] = 0;
      thisColour[2] = 0;
		}
	}

	return true;
}


void Dx7UI::renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]) {
  if (editing) {
    char buffer[12];
    intToString(param, buffer, 3);
    OLED::drawString(buffer, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);

    if (param < 6*21) {
      int op = param/21;
      int idx = param%21;

      buffer[0] = 'o';
      buffer[1] = 'p';
      buffer[2] = '1' + op;
      buffer[3] = ' ';
      buffer[4] = 0;

    OLED::drawString(buffer, 4*TEXT_SPACING_X, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);

    OLED::drawString(desc[idx], 8*TEXT_SPACING_X, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    } else if (param < 6*21+10) {
    OLED::drawString(desc_global[param-6*21], 4*TEXT_SPACING_X, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    }

  Dx7Patch &p = Dexed::globalPatch;
  int val = p.currentPatch[param];
    intToString(val, buffer, 3);
    OLED::drawString(buffer, 0, 5+TEXT_SIZE_Y_UPDATED+2, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
  } else {
    const char *text = did_button ? "buton" : "Halloj";
    OLED::drawString(text, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
  }
}
#endif

int Dx7UI::buttonAction(int x, int y, bool on, bool inCardRoutine) {
  if (x == clipViewButtonX && y == clipViewButtonY) {
    did_button = true;
    editing = false;
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
