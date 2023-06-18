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

class MultiScreen final : public UI {
public:
  MultiScreen() {};
  int padAction(int x, int y, int velocity);
  int buttonAction(int x, int y, bool on, bool inCardRoutine);

#if HAVE_OLED
  void renderOLED(uint8_t image[][OLED_MAIN_WIDTH_PIXELS]);
#endif

  bool did_button = false;
};

int MultiScreen::padAction(int x, int y, int velocity) {

  if (x >= displayWidth) return ACTION_RESULT_DEALT_WITH;

  if (sdRoutineLock && !allowSomeUserActionsEvenWhenInCardRoutine) {
    return ACTION_RESULT_REMIND_ME_OUTSIDE_CARD_ROUTINE; // Allow some of the time when in card routine.
  }

  int soundEditorResult = soundEditor.potentialShortcutPadAction(x, y, velocity);
  if (soundEditorResult != ACTION_RESULT_NOT_DEALT_WITH) {
    return soundEditorResult;
  }

  char modelStackMemory[MODEL_STACK_MAX_SIZE];
  ModelStack* modelStack = setupModelStackWithSong(modelStackMemory, currentSong);

  if (y == 0) {
    Output *out = currentSong->firstOutput;
    while (out) {
      if (out->type != INSTRUMENT_TYPE_KIT) {
        out = out->next;
      }
    }
    if (!out) {
      return ACTION_RESULT_DEALT_WITH;
    }
    InstrumentClip *clip = (InstrumentClip *)out->activeClip;
    if (!clip) {
      return ACTION_RESULT_DEALT_WITH;
    }
		ModelStackWithTimelineCounter* modelStackWithTimelineCounter = modelStack->addTimelineCounter(clip);

    int noteRowIndex;
    ModelStackWithNoteRow* modelStackWithNoteRow = clip->getNoteRowForYNote(x, modelStackWithTimelineCounter);

    auto kit = (Kit *)out;

    // .........
  }

  auto instrument = (MelodicInstrument*)currentSong->currentClip->output;
  if (!instrument || instrument->type == INSTRUMENT_TYPE_KIT) {
    return ACTION_RESULT_DEALT_WITH;
  }

  int noteCode = 60+x;
  if (velocity) {
    // Ensure the note the user is trying to sound isn't already sounding
    NoteRow* noteRow = ((InstrumentClip*)instrument->activeClip)->getNoteRowForYNote(noteCode);
    if (noteRow) {
      if (noteRow->soundingStatus == STATUS_SEQUENCED_NOTE) return ACTION_RESULT_DEALT_WITH;
    }

    int velocityToSound = 40+10*y;
    instrument->beginAuditioningForNote(modelStack, noteCode, velocityToSound, zeroMPEValues);
  } else {
    instrument->endAuditioningForNote(modelStack, noteCode);
  }

  return ACTION_RESULT_DEALT_WITH;
}

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
