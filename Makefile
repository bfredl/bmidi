CFLAGS = -mcpu=cortex-a9 -marm -mthumb-interwork -mlittle-endian -mfloat-abi=hard -mfpu=neon -fdiagnostics-parseable-fixits -Og

.SUFFIXES: .bin
CC = arm-none-eabi-gcc

all: checksum.o

.o.bin:
	arm-none-eabi-objcopy -O binary -j .text $< $@
