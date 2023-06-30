#include "hid/display/oled.h"
#include "io/midi/midi_engine.h"
#include "syx_pack.h"
#include "util/functions.h"

void pack(uint8_t status, uint8_t byte);

// TODO: this should just be a bigger per-device buffer
int32_t bulk_buffer[400];
volatile int write_pos = 0;
volatile int read_pos = 0;
bool buffering = false;

bool (*midi_send_callback)(void) = NULL;

bool midi_do_send(void) {
  if (!buffering) return false;
  if (!(read_pos < write_pos)) {
    buffering = false;
    write_pos = 0;
    read_pos = 0;
    return false;
  }
  while (read_pos < write_pos) {
    bool will_wrap = (connectedUSBMIDIDevices[0][0].numMessagesQueued >= 16);
    midiEngine.sendUsbMidiRaw(bulk_buffer[read_pos], 0);
    read_pos++;
    if (will_wrap) return true; // call me later
  }
  return true;
}

void send_sysex(uint8_t *data, int len) {
  if (len < 4 || data[0] != 0xf0 || data[len-1] != 0xf7) {
    return;
  }

  int pos = 0;
  while (pos < len) {
    int status, byte0 = 0, byte1 = 0, byte2 =0;
    byte0 = data[pos];
    if (pos == 0 || len-pos > 3) {
      status = 0x4;
      byte1 = data[pos+1];
      byte2 = data[pos+2];
      pos += 3;
    } else {
      status = 0x4 + (len-pos);
      if ((len-pos) > 1) byte1 = data[pos+1];
      if ((len-pos) > 2) byte2 = data[pos+2];
      pos = len;
    }
    uint32_t packed = ((uint32_t)byte2 << 24) | ((uint32_t)byte1 << 16) | ((uint32_t)byte0 << 8) | status;
    if (!buffering) {
      if (connectedUSBMIDIDevices[0][0].numMessagesQueued >= 15) {
        buffering = true;
        OLED::popupText("bufer", true);
      }
      midiEngine.sendUsbMidiRaw(packed, 0);
    } else {
      bulk_buffer[write_pos] = packed;
      write_pos++;

    }
  }

}

const int data_size = OLED_MAIN_WIDTH_PIXELS;
const int buf_size = (data_size*8)/7 + 2;
const int packed_rows = OLED_MAIN_HEIGHT_PIXELS >> 3;

void pack_led() {
  uint8_t buffer[buf_size+10];
  buffer[0] = 0xf0; // status: sysex
  buffer[1] = 0x7e; // :zany_face:
  buffer[2] = 0x42; // "led image row" indicator
  for (int i = 0; i < 1; i++) {
    buffer[3] = i;  // which packed row
    buffer[4] = data_size&0x7F;       // data_size low 7 bits
    buffer[5] = (data_size>>7)&0x7f;  // data_size high 7 bits (max 2**14-1 bytes)
    int len = pack_8bit_to_sysex(buffer+6, (sizeof buffer)-6-1,
                                 OLED::oledMainImage[i], data_size);
    if (!len) return;
    buffer[6+len] = 0xf7; // end of transmission
    send_sysex(buffer, len+7);


    char ebuf[10];
    intToString(len+7, ebuf);
    // OLED::popupText(ebuf);
  }
}


void big_sysex() {
  uint8_t buffer[128];
  buffer[0] = 0xf0; // status: sysex
  buffer[1] = 0x7e; // :zany_face:
  buffer[2] = 0x69; // TESTING
  for (int i = 3; i < 127; i++) {
    buffer[i] = i;
  }
  buffer[127] = 0xf7;
  send_sysex(buffer, 128);
}

extern void mod_main(int*,int*) {
		int potentialNumDevices = midiEngine.getPotentialNumConnectedUSBMIDIDevices(0);
    char ebuf[10];
    intToString(potentialNumDevices, ebuf);
  OLED::popupText(ebuf, true);
  //return;

  //OLED::popupText("halloj", true);
  uint8_t mySysex[] = {0xf0, 0x7e, 0x11, 0x22, 0xf7};
  //send_sysex(mySysex, sizeof mySysex);
  midi_send_callback = midi_do_send;

  // TODO: ring buffer for real
  buffering = false;
  write_pos = 0;
  read_pos = 0;

  big_sysex();
  // pack_led();
}

#define USB_NUM_USBIP (1u) // Set by Rohan
// HACKY: fullMessage should have the "cable id" cleared, and use `port` instead
void MidiEngine::sendUsbMidiRaw(uint32_t fullMessage, int port) {
	for (int ip = 0; ip < USB_NUM_USBIP; ip++) {
		int potentialNumDevices = getPotentialNumConnectedUSBMIDIDevices(ip);

		for (int d = 0; d < potentialNumDevices; d++) {
			ConnectedUSBMIDIDevice* connectedDevice = &connectedUSBMIDIDevices[ip][d];
			int maxPort = connectedDevice->maxPortConnected;
      if (port <= maxPort) {
					uint32_t channeled_message = fullMessage | (port << 4);
					connectedDevice->bufferMessage(channeled_message);
			}
		}
	}
}

