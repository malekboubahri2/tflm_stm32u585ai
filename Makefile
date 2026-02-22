# Minimal STM32 project Makefile

# toolchain binaries
CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy


OPENOCD_SCRIPT_PATH ?= $(CURDIR)/openocd  # openocd scripts

# compiler flags (debug, arm-m33, nano specs, CMSIS macros)
CFLAGS  = -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG \
          -DSTM32 -DSTM32U585xx -DB_U585I_IOT02A -DSTM32U585AIIxQ -DSTM32U5 \
          -O0 -ffunction-sections -fdata-sections \
          -Wall -fstack-usage \
          --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb

# assembler flags
ASFLAGS = $(CFLAGS) -x assembler-with-cpp

# enable dependency output
DEPFLAGS = -MMD -MP
CFLAGS  += $(DEPFLAGS)
ASFLAGS += $(DEPFLAGS)

# linker flags
LDFLAGS = -TSTM32U585xx_FLASH.ld --specs=nosys.specs \
          -Wl,-Map="firmware.map" -Wl,--gc-sections -static \
          -lc -lm


SRCDIR=app/src
# include directories for project headers and CMSIS
INCDIRS=drivers/CMSIS/Device/ST/STM32U5xx/Include \
        drivers/CMSIS/Include \
        app/inc
OBJDIR=build

# source lists
SOURCES=$(wildcard $(SRCDIR)/*.c)
ASOURCES=$(wildcard $(SRCDIR)/*.s)

OBJECTS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES)) \
        $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASOURCES))

# include paths
CFLAGS += $(addprefix -I,$(INCDIRS))

-include $(OBJECTS:.o=.d)


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

flash: firmware.bin
	@echo "Flashing..."
	# use workspace copy of scripts; target file lives under openocd/target
	openocd -s $(OPENOCD_SCRIPT_PATH) -f interface/stlink.cfg \
		-f target/stm32u5x.cfg \
		-c "program firmware.bin reset exit"

clean:
	rm -rf $(OBJDIR) firmware.elf firmware.bin
