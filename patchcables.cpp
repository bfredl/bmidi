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
#include <new>

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
	MenuItemPatchCableSelection() {}
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
  name = "mod matrix";  // not here!!
                        //
  // TODO: what happens if there is no items?
	soundEditor.currentValue = 0;
  selectedRowOnScreen = 0;
  scrollPos = 0;

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

  if (!soundEditor.currentParamManager) {
    return;
  }

  PatchCableSet *set = soundEditor.currentParamManager->getPatchCableSet();
  if (!set) {
    return;
  }

    //itemNames[0] = "ccc";
    //drawItemsForOled(itemNames, selectedRowOnScreen);

	for (int i = 0; i < OLED_MENU_NUM_OPTIONS_VISIBLE; i++) {
    int thisOption = scrollPos+i;
    if (thisOption > set->numPatchCables) {
      break;
    }

    if (thisOption == soundEditor.currentValue) {
      selectedRowOnScreen = i;
    }

    PatchCable *cable = &set->patchCables[thisOption];
    int src = cable->from;
    int src2 = -1;
    ParamDescriptor desc = cable->destinationParamDescriptor;
    if (!desc.isJustAParam()) {
      src2 = desc.getTopLevelSource();
    }
    int dest = desc.getJustTheParam();

    char* s = textbuf[i];

    const char* src_name = sourceToStringShort(src);  // exactly 4 chars
    const char* src2_name = sourceToStringShort(src2);
    // const char* dest_name = getPatchedParamDisplayNameForOled(dest);

    memcpy(s, src_name, 4);
    //intToString(src, s, 4);
    s[4] = ' ';
    memcpy(s+5, src2_name, 4);
    intToString(dest, s+9, 4);

    // strncpy(s+9, dest_name, (sizeof s) - 10);
    // s[(sizeof s)-1] = 0;

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
	else if (soundEditor.currentValue >= scrollPos + OLED_MENU_NUM_OPTIONS_VISIBLE) scrollPos++;

  // char bufer[20];
  // intToString(scrollPos, bufer, 4);
  // OLED::popupText(bufer);

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
  PatchCable *cable = &set->patchCables[val];
  s = val;

  if (cable->destinationParamDescriptor.isJustAParam()) {
    sourceSelectionMenuRegular.s = val;  // TODO: this is fugly, this value belongs in PatchCableStrengthMenu
    return &patchCableStrengthMenuRegular;
  } else {
    sourceSelectionMenuRange.s = val;
    return &patchCableStrengthMenuRange;
  }
}

MenuItemPatchCableSelection myItem;

extern void mod_main(int*,int*) {
  new (&myItem) MenuItemPatchCableSelection();

  Clip *klip = currentSong->currentClip;
  soundEditor.setup(klip, &myItem, 0);
  openUI(&soundEditor);

  return;

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

