#include "fatfs/ff.h"
#include "memory/general_memory_allocator.h"
#include "RZA1/mtu/mtu.h"
#include "io/debug/module.h"
#include "hid/display/oled.h"
#include <fmt/core.h>

#include "util/pack.h"

class MyMod : public LoadableModule {
  void unload() {
    OLED::popupText("good byes", true);
  }

  void activate();
	void sysex(MIDIDevice* device, uint8_t* data, int32_t len);
} mymod;

void chainload_buf(uint8_t *buffer, int buf_size);
void chainloader(uint32_t user_code_start, uint32_t user_code_end, uint32_t user_code_exec, char *buf, int buf_size);
#define UNCACHED_MIRROR_OFFSET 0x40000000

#define OFF_USER_CODE_START         ( 0x20)
#define OFF_USER_CODE_END           ( 0x24)
#define OFF_USER_CODE_EXECUTE       ( 0x28)
#define OFF_USER_SIGNATURE          ( 0x2c)

uint8_t *load_buf;
size_t load_bufsize;
size_t load_codesize;

void MyMod::sysex(MIDIDevice* device, uint8_t* data, int32_t len) {
	if (len < 7) {
		return;
	}


	// first 4 bytes are already used, next is command
	switch (data[4]) {
	case 0: {
    numericDriver.displayPopup(HAVE_OLED ? "hello" : "x");
    int pos = 512*(data[5]+0x80*data[6]);
    const int size = 512;
    const int packed_size = 586; // ceil(512+512/7)
    if (len < packed_size+8) {
      return;
    }

    if (pos == 0) {
      uint8_t tmpbuf[0x40] __attribute__ ((aligned (CACHE_LINE_SIZE)));
      unpack_7bit_to_8bit(tmpbuf, 0x40, data+7, len-8);
      uint32_t user_code_start = *(uint32_t *)(tmpbuf + OFF_USER_CODE_START);
      uint32_t user_code_end = *(uint32_t *)(tmpbuf + OFF_USER_CODE_END);
      load_codesize = (int32_t)(user_code_end - user_code_start);
      if (load_bufsize < load_codesize) {
        if (load_buf != nullptr) {
          GeneralMemoryAllocator::get().dealloc(load_buf);
        }
        load_bufsize = load_codesize + (511-((load_codesize-1)&511));
        load_buf = (uint8_t*)GeneralMemoryAllocator::get().alloc(load_bufsize, NULL, false, true);
        if (load_buf == nullptr) {
          // fail :(
          return;
        }
      }
    }

    if (load_buf == nullptr || pos + 512 > load_bufsize) {
      return;
    }

    unpack_7bit_to_8bit(load_buf+pos, size, data+7, packed_size);
    break;
  }

	case 1: {
    numericDriver.displayPopup(HAVE_OLED ? "godby" : "y");
		int size = 512*(data[4]+0x80*data[5]);
    if (false && (load_buf != nullptr || size != load_codesize)) {
			numericDriver.displayPopup(HAVE_OLED ? "wrong size?" : "SIZ FAIL");
      return;
    }
		uint32_t checksum;
		unpack_7bit_to_8bit((uint8_t *)&checksum, 4, data+6, 5);
		uint32_t actual = get_crc(load_buf, size);
    // TODO: also verify size is the same as the metadata size
		if (true || checksum == actual) {
      chainload_buf(load_buf, load_bufsize);
		} else {
			numericDriver.displayPopup(HAVE_OLED ? "checksum fail2" : "CRC FAIL2");
		}
  }
  }

}

void chainload() {
  const char *path = "IMAGES/chainseg.bin";

  FILINFO fno;

  int result = f_stat(path, &fno);
  FSIZE_t fileSize = fno.fsize;

  FIL currentFile;
  // Open the file
  result = f_open(&currentFile, path, FA_READ);
  if (result != FR_OK) {
    OLED::popupText("FEL1", true);
    return;
  }

  UINT numBytesRead;
  uint8_t* buffer = (uint8_t*)GeneralMemoryAllocator::get().alloc(fileSize, NULL, false, true);
  if (!buffer) {
    OLED::popupText("FEL2", true);
    return;
  }

  result = f_read(&currentFile, buffer, fileSize, &numBytesRead);
  // TODO: kolla

  chainload_buf(buffer, fileSize);
}

void chainload_buf(uint8_t *buffer, int buf_size) {

  uint32_t user_code_start = *(uint32_t *)(buffer + OFF_USER_CODE_START);
  uint32_t user_code_end = *(uint32_t *)(buffer + OFF_USER_CODE_END);
  uint32_t user_code_exec = *(uint32_t *)(buffer + OFF_USER_CODE_EXECUTE);


  char buf[200];
  auto res = fmt::format_to_n(buf, 200, "start {:x} end {:x} exec {:x}", user_code_start, user_code_end, user_code_exec);
  // auto res = fmt::format_to_n(buf, 200, "buf {:x} siz {:x}", numBytesRead, user_code_end - user_code_start);
  buf[res.size] = 0;
  OLED::popupText(buf, true);
  // return;

  char* funcbuf = (char *)0x20060700;
  char* from = (char *)chainloader;
  for (int i = 0; i < 128; i++) {
    (funcbuf+UNCACHED_MIRROR_OFFSET)[i] = from[i];
  }

  auto ptr = (void (*)(uint32_t, uint32_t, uint32_t, char *, int))funcbuf;

  disableTimer(TIMER_MIDI_GATE_OUTPUT);
  disableTimer(TIMER_SYSTEM_SLOW);
  disableTimer(TIMER_SYSTEM_FAST);
  disableTimer(TIMER_SYSTEM_SUPERFAST);

  ptr(user_code_start, user_code_end, user_code_exec, (char *)buffer, buf_size);

}

void MyMod::activate() {
  chainload();
}

void chainloader(uint32_t user_code_start, uint32_t user_code_end, uint32_t user_code_exec, char *buf, int buf_size) {

  int32_t code_size = (int32_t)(user_code_end - user_code_start);
  int32_t loop_num = (((uint32_t)code_size + 3) / (sizeof(uint32_t)));

  uint32_t *psrc = (uint32_t *)buf;
  uint32_t *pdst = (uint32_t *)user_code_start;
  uint32_t *pdst2 = (uint32_t *)(user_code_start + UNCACHED_MIRROR_OFFSET);

  for(int32_t i=0; i<loop_num; i++) {
    if (i*4 > buf_size) break;
    pdst[i] = psrc[i];
    pdst2[i] = psrc[i];
  }

  void (*ptr)(void) = (void (*)(void))user_code_exec;
  ptr(); // lessgo
}


extern void mod_main() {
  loadable_module = &mymod;
}
