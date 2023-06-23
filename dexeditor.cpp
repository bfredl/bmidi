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

class Dx7UI final : public UI {
public:
  Dx7UI() {};
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

  // TODO: this is huge, use alloc
  Cartridge cart;
  bool cartValid = false;
  int cartPos = 0;

  bool loadPending = false;
  char progName[11];
};

void Dx7UI::openFile() {
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

void Dx7UI::doLoad(int delta) {
  if (!patch) return;
  if (!cartValid) {
    openFile();
    if (!cartValid) return;
  }
  state = kStateLoading;

  cartPos += delta;
  if (cartPos < 0) cartPos = 0;
  if (cartPos >= 32) cartPos = 31;

  cart.unpackProgram(patch->currentPatch, cartPos);
  Cartridge::normalizePgmName(progName, (const char *)&patch->currentPatch[145]);
  renderUIsForOled();
}

void Dx7UI::focusRegained() {
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

int Dx7UI::padAction(int x, int y, int on) {
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

int Dx7UI::potentialShortcutPadAction(int x, int y, int on) {
  if (!on || x >= displayWidth || !Buttons::isShiftButtonPressed()) {
    return ACTION_RESULT_NOT_DEALT_WITH;
  }

  state = kStateNone;

  if (y > 1) {
    int op = 8-y-1; // op 0-5
    int ip = 0;
    if (x < 8) {
      ip = x;
    } else if (x < 13) {
      ip = (x-8)+16;
    } else if (x < 16) {
      ip = (x-13)+13;
    }
    param = 21*op+ip;
    state = kStateEditing;
  } else  if (y==0 && x < 10) {
    param = 6*21+x;
    state = kStateEditing;
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

void Dx7UI::selectEncoderAction(int8_t offset) {
  if (patch && state == kStateEditing) {

    int newval = patch->currentPatch[param]+offset;
    if (newval > 127) newval = 127;
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
  "sc1", "sc2", "sc3", "sc4", "sc5", "rate scale", "ampmod", "velocity sens",
  "level", "mode", "coarse", "fine", "detune"
};

const char *desc_global[] {
  "pitch rate1", "pitch rate2", "pitch rate3", "pitch rate4",
  "pitch level1", "pitch level2", "pitch level3", "pitch level4",
  "algoritm", "feedback",
};

// change the coleur
void color(uint8_t *colour, int r, int g, int b) {
  colour[0] = r;
  colour[1] = g;
  colour[2] = b;
}

bool Dx7UI::renderSidebar(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
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

bool Dx7UI::renderMainPads(uint32_t whichRows, uint8_t image[][displayWidth + sideBarWidth][3],
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
    } else if (i == 1) {
    }

  }
	return true;
}


void Dx7UI::renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]) {
  if (patch && state == kStateEditing) {
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

    int val = patch->currentPatch[param];
    if (param == 6*21+8) {
      val += 1; // algorithms start at one
    }
    intToString(val, buffer, 3);
    OLED::drawString(buffer, 0, 5+TEXT_SIZE_Y_UPDATED+2, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, TEXT_SPACING_X, TEXT_SIZE_Y_UPDATED);
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
#endif

int Dx7UI::buttonAction(int x, int y, bool on, bool inCardRoutine) {
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
    state = kStateNone;
    renderUIsForOled();
  } else if (x == backButtonX && y == backButtonY) {
    close();
  }
  return ACTION_RESULT_DEALT_WITH;
}

static Dx7UI dx7ui;

extern void mod_main(int*,int*) {
  new (&dx7ui) Dx7UI;
  dexedUI = &dx7ui;
}

// BLUFF: delete theese as soon as we have loading in firmware

char normparm(char value, char max, int id) {
    if ( value <= max && value >= 0 )
        return value;

    // if this is beyond the max, we expect a 0-255 range, normalize this
    // to the expected return value; and this value as a random data.

    value = abs(value);

    char v = ((float)value)/255 * max;

    return v;
}
void Cartridge::unpackProgram(uint8_t *unpackPgm, int idx) {
    // TODO put this in uint8_t :D
    char *bulk = (char *)voiceData + 6 + (idx * 128);

    for (int op = 0; op < 6; op++) {
        // eg rate and level, brk pt, depth, scaling

        for(int i=0; i<11; i++) {
            uint8_t currparm = bulk[op * 17 + i] & 0x7F; // mask BIT7 (don't care per sysex spec)
            unpackPgm[op * 21 + i] = normparm(currparm, 99, i);
        }

        memcpy(unpackPgm + op * 21, bulk + op * 17, 11);
        char leftrightcurves = bulk[op * 17 + 11]&0xF; // bits 4-7 don't care per sysex spec
        unpackPgm[op * 21 + 11] = leftrightcurves & 3;
        unpackPgm[op * 21 + 12] = (leftrightcurves >> 2) & 3;
        char detune_rs = bulk[op * 17 + 12]&0x7F;
        unpackPgm[op * 21 + 13] = detune_rs & 7;
        char kvs_ams = bulk[op * 17 + 13]&0x1F; // bits 5-7 don't care per sysex spec
        unpackPgm[op * 21 + 14] = kvs_ams & 3;
        unpackPgm[op * 21 + 15] = (kvs_ams >> 2) & 7;
        unpackPgm[op * 21 + 16] = bulk[op * 17 + 14]&0x7F;  // output level
        char fcoarse_mode = bulk[op * 17 + 15]&0x3F; //bits 6-7 don't care per sysex spec
        unpackPgm[op * 21 + 17] = fcoarse_mode & 1;
        unpackPgm[op * 21 + 18] = (fcoarse_mode >> 1)&0x1F;
        unpackPgm[op * 21 + 19] = bulk[op * 17 + 16]&0x7F;  // fine freq
        unpackPgm[op * 21 + 20] = (detune_rs >> 3) &0x7F;
    }

    for (int i=0; i<8; i++)  {
        uint8_t currparm = bulk[102 + i] & 0x7F; // mask BIT7 (don't care per sysex spec)
        unpackPgm[126+i] = normparm(currparm, 99, 126+i);
    }
    unpackPgm[134] = normparm(bulk[110]&0x1F, 31, 134); // bits 5-7 are don't care per sysex spec

    char oks_fb = bulk[111]&0xF;//bits 4-7 are don't care per spec
    unpackPgm[135] = oks_fb & 7;
    unpackPgm[136] = oks_fb >> 3;
    unpackPgm[137] = bulk[112] & 0x7F; // lfs
    unpackPgm[138] = bulk[113] & 0x7F; // lfd
    unpackPgm[139] = bulk[114] & 0x7F; // lpmd
    unpackPgm[140] = bulk[115] & 0x7F; // lamd
    char lpms_lfw_lks = bulk[116] & 0x7F;
    unpackPgm[141] = lpms_lfw_lks & 1;
    unpackPgm[142] = (lpms_lfw_lks >> 1) & 7;
    unpackPgm[143] = lpms_lfw_lks >> 4;
    unpackPgm[144] = bulk[117] & 0x7F;
    for (int name_idx = 0; name_idx < 10; name_idx++) {
        unpackPgm[145 + name_idx] = bulk[118 + name_idx] & 0x7F;
    } //name_idx
//    memcpy(unpackPgm + 144, bulk + 117, 11);  // transpose, name
}

uint8_t sysexChecksum(const uint8_t *sysex, int size) {
    int sum = 0;
    int i;

    for (i = 0; i < size; sum -= sysex[i++]);
    return sum & 0x7F;
}
