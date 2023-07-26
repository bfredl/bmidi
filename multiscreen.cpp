#include "hid/display/oled.h"
//#include <RootUI.h>
#include <new>
#include "extern.h"
#include "gui/ui/ui.h"
#include "gui/ui/sound_editor.h"
#include "processing/sound/sound_instrument.h"
#include "model/song/song.h"
#include "model/clip/instrument_clip.h"
#include "model/note/note_row.h"
#include "model/model_stack.h"
#include "model/drum/kit.h"
#include "model/instrument/melodic_instrument.h"
#include "io/debug/module.h"

class MultiScreen final : public UI {
public:
  MultiScreen() {};
  ActionResult padAction(int x, int y, int velocity) override;
  ActionResult buttonAction(hid::Button b, bool on, bool inCardRoutine) override;

	bool renderMainPads(uint32_t whichRows, uint8_t image[][kDisplayWidth + kSideBarWidth][3],
	                    uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth], bool drawUndefinedArea = false);
	bool renderSidebar(uint32_t whichRows, uint8_t image[][kDisplayWidth + kSideBarWidth][3],
	                   uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth]);

#if HAVE_OLED
  void renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]) override;
#endif

  bool did_button = false;
};

ActionResult MultiScreen::padAction(int x, int y, int velocity) {
  if (x >= kDisplayWidth) return ActionResult::DEALT_WITH;

  if (sdRoutineLock && !allowSomeUserActionsEvenWhenInCardRoutine) {
    return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE; // Allow some of the time when in card routine.
  }

  ActionResult soundEditorResult = soundEditor.potentialShortcutPadAction(x, y, velocity);
  if (soundEditorResult != ActionResult::NOT_DEALT_WITH) {
    return soundEditorResult;
  }

  char modelStackMemory[MODEL_STACK_MAX_SIZE];
  ModelStack* modelStack = setupModelStackWithSong(modelStackMemory, currentSong);

  if (y == 0) {
    Output *out = currentSong->firstOutput;
    while (out) {
      if (out->type == InstrumentType::KIT) {
        break;
      }
      out = out->next;
    }

    if (!out) {
      OLED::popupText("no out", true);
      return ActionResult::DEALT_WITH;
    }

    auto kit = (Kit *)out;
    InstrumentClip *clip = (InstrumentClip *)out->activeClip;

    if (!clip) {
      OLED::popupText("no clip", true);
      return ActionResult::DEALT_WITH;
    }
    ModelStackWithTimelineCounter* modelStackWithTimelineCounter = modelStack->addTimelineCounter(clip);

    int noteRowIndex;
    if (x > clip->noteRows.getNumElements()) {
      OLED::popupText("too much row", true);
      return ActionResult::DEALT_WITH;
    }
    NoteRow* noteRow = clip->noteRows.getElement(x);
    int noteRowId;
    if (noteRow) noteRowId = clip->getNoteRowId(noteRow, noteRowIndex);
    ModelStackWithNoteRow* modelStackWithNoteRow = modelStackWithTimelineCounter->addNoteRow(noteRowId, noteRow);
    if (!noteRow) {
      OLED::popupText("no row", true);
      return ActionResult::DEALT_WITH;
    }

    Drum* drum = noteRow->drum;
    if (!drum) {
      OLED::popupText("no dram", true);
      return ActionResult::DEALT_WITH;
    }

    if (velocity) {
      kit->beginAuditioningforDrum(modelStackWithNoteRow, drum, 100, zeroMPEValues);
    } else {
      kit->endAuditioningForDrum(modelStackWithNoteRow, drum);
    }
    return ActionResult::DEALT_WITH;
  }

  auto instrument = (MelodicInstrument*)currentSong->currentClip->output;
  if (!instrument || instrument->type == InstrumentType::KIT) {
    return ActionResult::DEALT_WITH;
  }

  int noteCode = 60+x;
  if (velocity) {
    // Ensure the note the user is trying to sound isn't already sounding
    NoteRow* noteRow = ((InstrumentClip*)instrument->activeClip)->getNoteRowForYNote(noteCode);
    if (noteRow) {
      if (noteRow->soundingStatus == STATUS_SEQUENCED_NOTE) return ActionResult::DEALT_WITH;
    }

    int velocityToSound = 16*y;
    instrument->beginAuditioningForNote(modelStack, noteCode, velocityToSound, zeroMPEValues);
  } else {
    instrument->endAuditioningForNote(modelStack, noteCode);
  }

  return ActionResult::DEALT_WITH;
}

#if HAVE_OLED
void MultiScreen::renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]) {
  const char *text = did_button ? "buton" : "MULTIPLE SCREEN";
  OLED::drawString(text, 0, 5, OLED::oledMainImage[0], OLED_MAIN_WIDTH_PIXELS, kTextSpacingX, kTextSizeYUpdated);
}
#endif

ActionResult MultiScreen::buttonAction(hid::Button b, bool on, bool inCardRoutine) {
  using namespace hid::button;
  if (b == CLIP_VIEW) {
    did_button = true;
    renderUIsForOled();
    uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
    return ActionResult::DEALT_WITH;
  } else if (b == BACK) {
    close();
  }
  return ActionResult::DEALT_WITH;
}

// change the coleur
void color(uint8_t *colour, int r, int g, int b) {
  colour[0] = r;
  colour[1] = g;
  colour[2] = b;
}

bool MultiScreen::renderMainPads(uint32_t whichRows, uint8_t image[][kDisplayWidth + kSideBarWidth][3],
	                    uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth], bool drawUndefinedArea) {
	if (!image) {
		return true;
	}

  return true;
}
	bool MultiScreen::renderSidebar(uint32_t whichRows, uint8_t image[][kDisplayWidth + kSideBarWidth][3],
	                   uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth]) {

	if (!image) {
		return true;
	}
	for (int i = 0; i < kDisplayHeight; i++) {
		if (!(whichRows & (1 << i))) continue;
    uint8_t* thisColour = image[i][kDisplayWidth];
    color(image[i][kDisplayWidth], 15*i, 15*i, 14*i);
  }

  return true;
}

static MultiScreen multiscreen;

class MyMod : public LoadableModule {
  void unload();

  void activate();
} mymod;

void MyMod::unload() {
  if (isUIOpen(&multiscreen)) {
    multiscreen.close();
  }
}

void MyMod::activate() {
  openUI(&multiscreen);
}

extern void mod_main() {
  new (&mymod) MyMod;
  new (&multiscreen) MultiScreen;
  // This (with RootUI parent class) doesn't work
  // changeRootUI(&multiscreen);

  loadable_module = &mymod;

  openUI(&multiscreen);
}
