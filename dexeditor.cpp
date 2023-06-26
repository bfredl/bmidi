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
#include "GeneralMemoryAllocator.h"
#include "uitimermanager.h"
#include "dexed/engine.h"
#include "dexed/PluginData.h"

class Dx7UImod final : public UI {
public:
  Dx7UImod() {};
	void focusRegained();
  int padAction(int x, int y, int velocity);
  int buttonAction(int x, int y, bool on, bool inCardRoutine);
	void selectEncoderAction(int8_t offset);


	bool renderMainPads(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
	                    uint8_t occupancyMask[][displayWidth + sideBarWidth], bool drawUndefinedArea = false);
	bool renderSidebar(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
	                   uint8_t occupancyMask[][displayWidth + sideBarWidth]);
#if HAVE_OLED
  void renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]);
  void renderEnvelope(uint8_t image[][OLED_MAIN_WIDTH_PIXELS], int op, int param);
  void renderScaling(uint8_t image[][OLED_MAIN_WIDTH_PIXELS], int op, int param);
  void renderTuning(uint8_t image[][OLED_MAIN_WIDTH_PIXELS], int op, int param);
#endif

  void openFile();
  void doLoad(int delta);

  int potentialShortcutPadAction(int x, int y, int on);

  bool did_button = false;
  enum State {
    kStateNone,
    kStateEditing,
    kStateLoading,
  };
  State state = kStateNone;
  int param = 0;
  Dx7Patch *patch;
  bool isScale;

  // TODO: this is huge, use alloc
  Cartridge cart;
  bool cartValid = false;
  int cartPos = 0;
  int uplim = 127;

  bool loadPending = false;
  char progName[11];
};

void Dx7UImod::openFile() {
  const char *path = "dx7.syx";
  const int xx = 4104;

  FILINFO fno;
  int result = f_stat(path, &fno);
  FSIZE_t fileSize = fno.fsize;
  if (fileSize < xx) {
    OLED::popupText("too small!", true);
  }

  FIL currentFile;
  // Open the file
  result = f_open(&currentFile, path, FA_READ);
  if (result != FR_OK) {
    OLED::popupText("read error 1", true);
    return;
  }

  int status;
  UINT numBytesRead;
  int readsize = min((int)fileSize, 8192);
  uint8_t* buffer = (uint8_t*)generalMemoryAllocator.alloc(readsize, NULL, false, true);
  if (!buffer) {
    OLED::popupText("out of memory", true);
    return;
  }
  result = f_read(&currentFile, buffer, fileSize, &numBytesRead);
  if (numBytesRead < xx) {
    OLED::popupText("read error 2", true);
    goto free;
  }

  status = cart.load(buffer, numBytesRead);
  if (status) {
    OLED::popupText("load error", true);
    goto free;
  }

  cartValid = true;
  cartPos = 0;

free:
  generalMemoryAllocator.dealloc(buffer);
}

void Dx7UImod::doLoad(int delta) {
  if (!patch) return;
  if (!cartValid) {
    openFile();
    if (!cartValid) return;
  }
  state = kStateLoading;
  uiTimerManager.unsetTimer(TIMER_SHORTCUT_BLINK);

  cartPos += delta;
  if (cartPos < 0) cartPos = 0;
  if (cartPos >= 32) cartPos = 31;

  cart.unpackProgram(patch->currentPatch, cartPos);
  Cartridge::normalizePgmName(progName, (const char *)&patch->currentPatch[145]);
  renderUIsForOled();
}

void Dx7UImod::focusRegained() {
    uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);

    if (dexedEditedSource) {
      if (dexedEditedSource->dx7Patch == NULL) {
        void* memory = generalMemoryAllocator.alloc(sizeof(Dx7Patch), NULL, false, true);
        dexedEditedSource->dx7Patch = new(memory) Dx7Patch;
        memcpy(dexedEditedSource->dx7Patch->currentPatch, Dexed::globalPatch.currentPatch, sizeof Dexed::globalPatch.currentPatch);
      }
      patch = dexedEditedSource->dx7Patch;
    } else {
      patch = NULL;
    }
}

int Dx7UImod::padAction(int x, int y, int on) {
  if (x == displayWidth && on && y > 1) {
    int op = 8-y-1; // op 0-5
    char* val = &Dexed::dummy_controller.opSwitch[op];
    *val = (*val == '0' ? '1' : '0');
    uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
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

int Dx7UImod::potentialShortcutPadAction(int x, int y, int on) {
  if (!on || x >= displayWidth || !Buttons::isShiftButtonPressed()) {
    return ACTION_RESULT_NOT_DEALT_WITH;
  }

  state = kStateNone;
  uplim = 127;

  if (y > 1) {
    int op = 8-y-1; // op 0-5
    int ip = 0;
    if (isScale && x < 5) {
      ip = 8+x;
    } else if (x < 8) {
      ip = x;
    } else if (x < 13) {
      ip = (x-8)+16;
    } else if (x < 16) {
      ip = (x-13)+13;
    }
    param = 21*op+ip;
    state = kStateEditing;
    if (ip == 17) {
      uplim = 1;
    } else if (ip == 11 || ip == 12) {
      uplim = 3;
    }
  } else  if (y==0 && x < 10) {
    param = 6*21+x;
    state = kStateEditing;
  } else if (y == 1) {
    if (x == 0) isScale = !isScale;
  }

  if (state == kStateEditing) {
    loadPending = false;
    soundEditor.setupShortcutBlink(x, y, 1);
    soundEditor.blinkShortcut();
  } else {
    uiTimerManager.unsetTimer(TIMER_SHORTCUT_BLINK);
  }

  renderUIsForOled();
  uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
  return ACTION_RESULT_DEALT_WITH;
}

void Dx7UImod::selectEncoderAction(int8_t offset) {
  if (patch && state == kStateEditing) {

    int newval = patch->currentPatch[param]+offset;
    if (newval > uplim) newval = uplim;
    if (newval < 0) newval = 0;
    patch->currentPatch[param] = newval;
    renderUIsForOled();
  } else if (state == kStateLoading) {
    doLoad(offset);
  }

}

#if HAVE_OLED

const char *desc[] = {
  "env rate1", "env rate2", "env rate3", "env rate4",
  "env level1", "env level2", "env level3", "env level4",
  "breakpoint", "left depth", "right depth", "left curve", "right curve",
  "rate scale", "ampmod", "velocity sens",
  "level", "mode", "coarse", "fine", "detune"
};

const char *desc_global[] {
  "pitch rate1", "pitch rate2", "pitch rate3", "pitch rate4",
  "pitch level1", "pitch level2", "pitch level3", "pitch level4",
  "algoritm", "feedback",
};

const char *curves[] {"lin-", "exp-", "exp+", "lin+", "?????"};

// change the coleur
void color(uint8_t *colour, int r, int g, int b) {
  colour[0] = r;
  colour[1] = g;
  colour[2] = b;
}

bool Dx7UImod::renderSidebar(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
                                       uint8_t occupancyMask[][displayWidth + sideBarWidth]) {
	if (!image) return true;

	for (int i = 0; i < displayHeight; i++) {
		if (!(whichRows & (1 << i))) continue;
    uint8_t* thisColour = image[i][displayWidth];
    int op = 8-i-1; // op 0-5

    bool muted = false;
    if (op < 6) {
      char* val = &Dexed::dummy_controller.opSwitch[op];
      if (*val == '0') {
        color(thisColour, 255, 0, 0);
      } else {
        color(thisColour, 0, 255, 0);
      }
    } else {
        color(thisColour, 0, 0, 0);
    }
	}

	return true;
}

bool Dx7UImod::renderMainPads(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
                                    uint8_t occupancyMask[][displayWidth + sideBarWidth], bool drawUndefinedArea) {
	if (!image) return true;

  int editedOp = -1;
  if (state == kStateEditing && param < 6*21) {
    editedOp = param/21;
  }

	for (int i = 0; i < displayHeight; i++) {
		if (!(whichRows & (1 << i))) continue;
    memset(image[i], 0, 3*16);
    int op = 8-i-1; // op 0-5
    if (op < 6) {
      char* val = &Dexed::dummy_controller.opSwitch[op];
      for (int x = 0; x < 4; x++) {
        color(image[i][x], 0, 120, 0);
        color(image[i][x+4], 0, 40, 110);
      }
      if (isScale) {
        for (int x = 0; x < 5; x++) {
          color(image[i][x], 100, 150, 0);
        }
      }
      color(image[i][8], 70, 70, 70);
      for (int x = 0; x < 3; x++) {
        color(image[i][x+10], 150, 0, 150);
      }
      color(image[i][15], 150, 125, 0);
      if (*val == '0') {
        for (int x = 0; x < 16; x++) {
          for (int c = 0; c < 16; c++) {
            image[i][x][c] = (image[i][x][c] >> 1) + (image[i][x][c] >> 2);
          }
        }
      } else if (op == editedOp) {
        for (int x = 0; x < 16; x++) {
          for (int c = 0; c < 3; c++) {
            image[i][x][c] = image[i][x][c] + (image[i][x][c] >> 2);
            if (image[i][x][c] < 30) image[i][x][c] = 30;
          }
        }
      }
    } else if (i == 0) {
      for (int x = 0; x < 4; x++) {
        color(image[i][x], 40, 240, 40);
        color(image[i][x+4], 40, 140, 240);
      }
      color(image[i][8], 255, 0, 0);
      color(image[i][9], 100, 100, 100);
    } else if (i == 1) {
      if(isScale) {
        color(image[i][0], 0, 120, 0);
      } else {
        color(image[i][0], 100, 150, 0);
      }
    }

  }
	return true;
}

void Dx7UImod::renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]) {
  if (patch && state == kStateEditing) {
    char buffer[12];
    intToString(param, buffer, 3);
    OLED::drawString(buffer, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);

    int op = param/21;
    int idx = param%21;
    if (param < 6*21) {

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

    int val = patch->currentPatch[param];
    if (param == 6*21+8) {
      val += 1; // algorithms start at one
    }
    int ybel = 5+TEXT_SIZE_Y_UPDATED+2;
    intToString(val, buffer, 3);
    OLED::drawString(buffer, 0, ybel, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);

    const char *extra = "";
    if (param < 6*21 && (idx == 11 || idx == 12)) {
      int kurva = min(val,4);
      extra = curves[kurva];
    } else if (param < 6*21 && (idx == 17)) {;
      extra = val ? "fixed" : "keyboard";
    }
    OLED::drawString(extra, 4*TEXT_SPACING_X, ybel, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);

    if (param < (6*21+8) && idx < 8) {
      renderEnvelope(image, op, idx); // op== 6 for pitch envelope
    } else if (param < 6*21 && idx < 13) {
      renderScaling(image, op, idx);
    } else if (param < 6*21 && idx < 16) {
      // :P
    } else if (param < 6*21 && idx < 21) {
      renderTuning(image, op, idx);
    }

  } else if (state == kStateLoading) {
    char buffer[12];
    intToString(cartPos+1, buffer, 3);
    OLED::drawString(buffer, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    progName[10] = 0; // just checking
    OLED::drawString(progName, 0, 5+TEXT_SIZE_Y_UPDATED+2, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
  } else {
    const char *text = did_button ? "buton" : "Halloj";
    OLED::drawString(text, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
  }
}

void Dx7UImod::renderEnvelope(uint8_t image[][OLED_MAIN_WIDTH_PIXELS], int op, int param) {
  char buffer[12];
  int ybel = 5+2*(TEXT_SIZE_Y_UPDATED+2)+2;
  int ybel2 = ybel+(TEXT_SIZE_Y_UPDATED+2);
  for (int i = 0; i < 4; i++) {
    int val = patch->currentPatch[op*21+i];
    intToString(val, buffer, 3);
    int xpos = 10+i*4*TEXT_SPACING_X;
    OLED::drawString(buffer, xpos, ybel, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    if (i == param) {
      OLED::invertArea(xpos-1, TEXT_SPACING_X*3+1, ybel-1, ybel + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
    }
    val = patch->currentPatch[op*21+4+i];
    intToString(val, buffer, 3);
    OLED::drawString(buffer, xpos, ybel2, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    if (i+4 == param) {
      OLED::invertArea(xpos-1, TEXT_SPACING_X*3+1, ybel2-1, ybel2 + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
    }
  }
}

void Dx7UImod::renderScaling(uint8_t image[][OLED_MAIN_WIDTH_PIXELS], int op, int param) {
  char buffer[12];
  int ybel = 5+2*(TEXT_SIZE_Y_UPDATED+2)+2;
  int ybel2 = ybel+(TEXT_SIZE_Y_UPDATED+2);
  int ybelmid = (ybel+ybel2)>>1;

  for (int i = 0; i < 2; i++) {
    int val = patch->currentPatch[op*21+9+i];
    intToString(val, buffer, 3);
    int xpos = 14+i*11*TEXT_SPACING_X;
    OLED::drawString(buffer, xpos, ybel, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    if (9+i == param) {
      OLED::invertArea(xpos-1, TEXT_SPACING_X*3+1, ybel-1, ybel + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
    }

    val = patch->currentPatch[op*21+11+i];
    int kurva = min(val,4);
    OLED::drawString(curves[kurva], xpos, ybel2, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    if (11+i == param) {
      OLED::invertArea(xpos-1, TEXT_SPACING_X*4+1, ybel2-1, ybel2 + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
    }
  }

  int val = patch->currentPatch[op*21+8];
  intToString(val, buffer, 3);
  int xpos = 14+6*TEXT_SPACING_X;
  OLED::drawString(buffer, xpos, ybelmid, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
  if (8 == param) {
    OLED::invertArea(xpos-1, TEXT_SPACING_X*3+1, ybelmid-1, ybelmid + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
  }

  int noteCode = val + 17;
	if (getRootUI() == &keyboardScreen) {
    InstrumentClip *clip = (InstrumentClip*)currentSong->currentClip;
    int notedisp = noteCode - clip->yScrollKeyboardScreen;
    int x = notedisp;
    int y = 0;
    while (x>10 && y < 7) {
      x -= 5;  // fUBBIGT
      y += 1;
    }
    if (x >= 0 && x < 16) {
      // TODO: different color, do not override the main shortcut blink
      soundEditor.setupShortcutBlink(x, y, 2);
    }
  }
}

void Dx7UImod::renderTuning(uint8_t image[][OLED_MAIN_WIDTH_PIXELS], int op, int param) {
  char buffer[12];
  int ybel = 5+2*(TEXT_SIZE_Y_UPDATED+2)+2;
  int ybel2 = ybel+(TEXT_SIZE_Y_UPDATED+2);
  for (int i = 0; i < 3; i++) {
    int val = patch->currentPatch[op*21+i+18];
    intToString(val, buffer, 3);
    int xpos = 10+(6+i*4)*TEXT_SPACING_X;
    OLED::drawString(buffer, xpos, ybel, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
    if (i+18 == param) {
      OLED::invertArea(xpos-1, TEXT_SPACING_X*3+1, ybel-1, ybel + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
    }
  }

  int mode = patch->currentPatch[op*21+17];
  const char *text = mode ? "fixed" : "pitch";
  int xpos = 10;
  OLED::drawString(text, xpos, ybel, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
  if (17 == param) {
    OLED::invertArea(xpos-1, TEXT_SPACING_X*5+1, ybel-1, ybel + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
  }

  OLED::drawString("level", xpos, ybel2, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);

  int val = patch->currentPatch[op*21+16];
  intToString(val, buffer, 3);
  int xpos2 = 10+6*TEXT_SPACING_X;
  OLED::drawString(buffer, xpos2, ybel2, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
  if (16 == param) {
    OLED::invertArea(xpos2-1, TEXT_SPACING_X*3+1, ybel2-1, ybel2 + TEXT_SIZE_Y_UPDATED, OLED::oledMainImage);
  }

}

#endif

int Dx7UImod::buttonAction(int x, int y, bool on, bool inCardRoutine) {
  if (on && loadPending) {
    if (x == loadButtonX && y == loadButtonY) {
      loadPending = false;
      doLoad(0);
      return ACTION_RESULT_DEALT_WITH;
    }
  }

  if (x == loadButtonX && y == loadButtonY && on) {
    if (state != kStateLoading) {
      OLED::popupText("overwrite? press again", false);
      loadPending = true;
    }
  } else if (x == clipViewButtonX && y == clipViewButtonY) {
    did_button = true;
    uiTimerManager.unsetTimer(TIMER_SHORTCUT_BLINK);
    state = kStateNone;
    renderUIsForOled();
  } else if (x == backButtonX && y == backButtonY) {
    close();
  }
  return ACTION_RESULT_DEALT_WITH;
}

static Dx7UImod dx7ui;

extern void mod_main(int*,int*) {
  new (&dx7ui) Dx7UImod;
  dexedUI = &dx7ui;
}

