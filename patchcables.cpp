#include "processing/sound/sound.h"
#include "model/song/song.h"
#include "model/clip/instrument_clip.h"
#include "model/output.h"
#include "modulation/patch/patch_cable_set.h"
#include "hid/display/oled.h"
#include "gui/menu_item/menu_item.h"
#include "gui/ui/sound_editor.h"
#include "gui/menu_item/menu_item_patch_cable_strength.h"
#include "gui/menu_item/menu_item_source_selection.h"

char const* sourceToStringShort(uint8_t source) {

	switch (source) {
	case PATCH_SOURCE_LFO_GLOBAL:
		return "lfo1";

	case PATCH_SOURCE_LFO_LOCAL:
		return "lfo2";

	case PATCH_SOURCE_ENVELOPE_0:
		return "env1";

	case PATCH_SOURCE_ENVELOPE_1:
		return "env2";

	case PATCH_SOURCE_VELOCITY:
		return "velo";

	case PATCH_SOURCE_NOTE:
		return "note";

	case PATCH_SOURCE_COMPRESSOR:
		return "comp";

	case PATCH_SOURCE_RANDOM:
		return "rand";

	case PATCH_SOURCE_AFTERTOUCH:
		return "pres";

	case PATCH_SOURCE_X:
		return "mpeX";

	case PATCH_SOURCE_Y:
		return "mpeY";

	default:
		return "----";
	}
}
class MenuItemPatchCableSelection : public MenuItem {
public:
	MenuItemPatchCableSelection();
	void beginSession(MenuItem* navigatedBackwardFrom = NULL);
	void selectEncoderAction(int offset) final;
	void readValueAgain() final;
	MenuItem* selectButtonPress();

#if HAVE_OLED
	void drawPixelsForOled();
	static int selectedRowOnScreen;
	int scrollPos; // Each instance needs to store this separately
#else
	void drawValue();
#endif

	uint8_t s;
};

int MenuItemPatchCableSelection::selectedRowOnScreen;

static char textbuf[OLED_MENU_NUM_OPTIONS_VISIBLE][24];


void MenuItemPatchCableSelection::beginSession(MenuItem* navigatedBackwardFrom) {
  // TODO: what happens if there is no items?
	soundEditor.currentValue = 0;

	if (navigatedBackwardFrom) {
    // TODO: klippa till maxkabel
			soundEditor.currentValue = s;
		// Scroll pos will be retained from before.
	}

#if !HAVE_OLED
	drawValue();
#endif
}

void MenuItemPatchCableSelection::readValueAgain() {
#if HAVE_OLED
	renderUIsForOled();
#else
	drawValue();
#endif
}


void MenuItemPatchCableSelection::drawPixelsForOled() {
	char const* itemNames[OLED_MENU_NUM_OPTIONS_VISIBLE];
	for (int i = 0; i < OLED_MENU_NUM_OPTIONS_VISIBLE; i++) {
		itemNames[i] = NULL;
	}

  PatchCableSet *set = soundEditor.currentParamManager->getPatchCableSet();

	for (int i = 0; i < OLED_MENU_NUM_OPTIONS_VISIBLE; i++) {
    int thisOption = scrollPos+i;
    if (thisOption > set->numPatchCables) {
      break;
    }

    PatchCable *cable = &set->patchCables[0];
    int src = cable->from;
    int src2 = -1;
    ParamDescriptor desc = cable->destinationParamDescriptor;
    if (!desc.isJustAParam()) {
      src2 = desc.getTopLevelSource();
    }
    int dest = desc.getJustTheParam();

    const char* src_name = sourceToStringShort(src);  // exactly 4 chars
    const char* src2_name = sourceToStringShort(src2);
    const char* dest_name = getPatchedParamDisplayNameForOled(dest);

    char* s = textbuf[i];
    memcpy(s, src_name, 4);
    s[4] = ' ';
    memcpy(s+5, src2_name, 4);

    strncpy(s+9, dest_name, (sizeof s) - 9);
    s[(sizeof s)-1] = 0;

    itemNames[i] = s;
  }

  drawItemsForOled(itemNames, selectedRowOnScreen);
}

void MenuItemPatchCableSelection::selectEncoderAction(int offset) {
	int newValue = soundEditor.currentValue;
  newValue += offset;

  PatchCableSet *set = soundEditor.currentParamManager->getPatchCableSet();

#if HAVE_OLED
		if (newValue >= set->numPatchCables || newValue < 0) return;
#else
		if (newValue >= set->numPatchCables) newValue -= set->numPatchCables;
		else if (newValue < 0) newValue += set->numPatchCables;
#endif

	soundEditor.currentValue = newValue;

#if HAVE_OLED
	if (soundEditor.currentValue < scrollPos) scrollPos = soundEditor.currentValue;
	else if (offset >= 0 && selectedRowOnScreen == OLED_MENU_NUM_OPTIONS_VISIBLE - 1) scrollPos++;

	renderUIsForOled();
#else
	drawValue();
#endif
}


MenuItem* MenuItemPatchCableSelection::selectButtonPress() {
  PatchCableSet *set = soundEditor.currentParamManager->getPatchCableSet();
  int val = soundEditor.currentValue;

  if (val >= set->numPatchCables) {
    return MenuItem::selectButtonPress();
  }
  PatchCable *cable = &set->patchCables[0];

  if (cable->destinationParamDescriptor.isJustAParam()) {
    sourceSelectionMenuRegular.s = val;  // TODO: this is fugly, this value belongs in PatchCableStrengthMenu
    return &patchCableStrengthMenuRegular;
  } else {
    sourceSelectionMenuRange.s = val;
    return &patchCableStrengthMenuRange;
  }
}


extern void mod_main(int*,int*) {
  Clip *klip = currentSong->currentClip;
  if (klip->type != CLIP_TYPE_INSTRUMENT) return;
  InstrumentClip *clip = (InstrumentClip *)klip;
  Sound *sound;
  if (clip->output->type == INSTRUMENT_TYPE_SYNTH) {
    sound = (Sound*)clip->output;
  } else {
    return;
  }

  PatchCableSet *set = clip->paramManager.getPatchCableSet();
  char buffer[16] = "num: ";
  intToString(set->numPatchCables, buffer+5, 3);
  OLED::popupText(buffer, 1);
  // return;

  PatchCable *cable = &set->patchCables[0];

  int src = cable->from;
  ParamDescriptor desc = cable->destinationParamDescriptor;

  if (!desc.isJustAParam()) {
    int src2 = desc.getTopLevelSource();
  }


  int dest = desc.getJustTheParam();

}

