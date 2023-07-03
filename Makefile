DELUGE_PATH=./DelugeFirmware
FW_INCLUDE = -I$(DELUGE_PATH)/src/drivers/RZA1 -I$(DELUGE_PATH)/src/drivers/All_CPUs/usb/userdef -I$(DELUGE_PATH)/src/drivers/All_CPUs/usb -I$(DELUGE_PATH)/src/drivers/All_CPUs/dmac -I$(DELUGE_PATH)/src/drivers/All_CPUs/uart_all_cpus/ -I$(DELUGE_PATH)/src/drivers/All_CPUs/rspi_all_cpus -I$(DELUGE_PATH)/src/drivers/All_CPUs/oled_low_level_all_cpus -I$(DELUGE_PATH)/src/drivers/RZA1/mtu -I$(DELUGE_PATH)/src/drivers/RZA1/oled -I$(DELUGE_PATH)/src/drivers/RZA1/spibsc -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/src -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/src/sd/sdio -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/src/sd -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/userdef -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/src/sd/access -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/inc -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi -I$(DELUGE_PATH)/src/drivers/RZA1/rspi -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/src/sd/inc -I$(DELUGE_PATH)/src/drivers/RZA1/sdhi/src/sd/inc/access -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_basic -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_basic/src -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_basic/src/hw/inc -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_basic/src/driver -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_basic/src/hw -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_basic/src/driver/inc -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_hmidi -I$(DELUGE_PATH)/src/drivers/RZA1/usb/r_usb_hmidi/src/inc -I$(DELUGE_PATH)/src/drivers/RZA1/usb/userdef -I$(DELUGE_PATH)/src/drivers/RZA1/intc -I$(DELUGE_PATH)/src/drivers/RZA1/system -I$(DELUGE_PATH)/src/drivers/RZA1/system/iobitmasks -I$(DELUGE_PATH)/src/drivers/RZA1/system/iodefines -I$(DELUGE_PATH)/src/drivers/RZA1/bsc -I$(DELUGE_PATH)/src/drivers/RZA1/dma -I$(DELUGE_PATH)/src/drivers/RZA1/gpio -I$(DELUGE_PATH)/src/drivers/RZA1/ssi -I$(DELUGE_PATH)/src/drivers/RZA1/uart -I$(DELUGE_PATH)/src/drivers/All_CPUs -I$(DELUGE_PATH)/src/drivers/All_CPUs/dmac -I$(DELUGE_PATH)/src/drivers/All_CPUs/ssi_all_cpus -I$(DELUGE_PATH)/src/NE10/modules -I$(DELUGE_PATH)/src/NE10/inc -I$(DELUGE_PATH)/src -I$(DELUGE_PATH)/src/fatfs -I$(DELUGE_PATH)/src/deluge

# TODO: pic should not be used. we need to set the load adress explicitly
BASEFLAGS = -mcpu=cortex-a9 -marm -mthumb-interwork -mlittle-endian -mfloat-abi=hard -mfpu=neon -Og  -fmessage-length=0 -fsigned-char
CFLAGS = $(BASEFLAGS) $(FW_INCLUDE) -DHAVE_RTT=0 -DHAVE_OLED=1 -fno-rtti -fno-exceptions -fabi-version=0
CXXFLAGS = $(CFLAGS)

# NOTE: the -s flag in the build rule for DelugeFirmware-release-oled.elf must be removed!
# this does not change the content of the final DelugeFirmware-release-oled.bin fine, but keeps the symbol
# table so we can adress this
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
	./senddata hw:1,0,0 exec $<
