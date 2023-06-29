#include "hid/display/oled.h"
#include "io/midi/midi_engine.h"

void pack(uint8_t status, uint8_t byte);

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
    midiEngine.sendUsbMidiRaw(packed, 0);

  }

}

extern void mod_main(int*,int*) {
  OLED::popupText("halloj", true);
  uint8_t mySysex[] = {0xf0, 0x7e, 0x11, 0x22, 0xf7};
  send_sysex(mySysex, sizeof mySysex);
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

