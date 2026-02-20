# Minimal STM32 project Makefile

# Toolchain
CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
# target is STM32U585AI (Cortex‑M33)
CFLAGS=-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -Wall -O0 -g
# use CubeIDE-generated linker script for U585
# link with newlib (stdc, math) but skip default start files
LDFLAGS=-TSTM32U585xx_FLASH.ld -nostartfiles -specs=nosys.specs

SRCDIR=app/src
# include directories for project headers and CMSIS
INCDIRS=app/inc \
        drivers/CMSIS/Device/ST/STM32U5xx/Include \
        drivers/CMSIS/Include
OBJDIR=build

SOURCES=$(wildcard $(SRCDIR)/*.c)
ASOURCES=$(wildcard $(SRCDIR)/*.s)
OBJECTS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES)) \
        $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASOURCES))

# add project/CMSIS includes plus newlib headers
CFLAGS+=$(addprefix -I,$(INCDIRS)) -I/usr/include/newlib


all: firmware.bin

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.s | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

firmware.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) firmware.elf firmware.bin
