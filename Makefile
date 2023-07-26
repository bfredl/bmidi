DELUGE_PATH=./DelugeFirmware
FW_INCLUDE = -I$(DELUGE_PATH)/src -I$(DELUGE_PATH)/src/deluge -I$(DELUGE_PATH)/src/fatfs -I$(DELUGE_PATH)/src/NE10/inc -includepreinclude_cxx.h

# TODO: pic should not be used. we need to set the load adress explicitly
BASEFLAGS = -mcpu=cortex-a9 -marm -mthumb-interwork -mlittle-endian -mfloat-abi=hard -mfpu=neon -Og  -fmessage-length=0 -fsigned-char
CFLAGS = $(BASEFLAGS) $(FW_INCLUDE) -DHAVE_RTT=0 -DHAVE_OLED=1 -fno-rtti -fno-exceptions -fabi-version=0
CXXFLAGS = $(CFLAGS)

FW_ELF = DelugeFirmware/dbt-build-debug-oled/Deluge-debug-oled.elf
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++

all: chainload.bin symbolic.bin multiscreen.bin

# myyyyyy precious
.PRECIOUS: %.elf %.o %.bin

# this is a bit of a hacky whacky. currently the load adress is so low (8000), so ld is forced to generate
# indirect jumps with fixed address. This works but is inefficient. make a linker script which makes the
# load position explicit which is both correct (no nasty surprises) and gives more efficient code.
%.elf: %.o $(FW_ELF) module.ld startup.o
	arm-none-eabi-c++ $(BASEFLAGS) -nostartfiles -T module.ld -Wl,-R,$(FW_ELF) startup.o $< -o $@

%.bin: %.elf
	arm-none-eabi-objcopy -O binary $< $@

clean:
	rm *.elf *.bin *.o

%.exec : %.bin
	./loadmod hw:1,0,0 $<
