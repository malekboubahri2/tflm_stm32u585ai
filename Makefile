
# Toolchain
CC=arm-none-eabi-gcc
CXX=arm-none-eabi-g++
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

# Build settings
MAKEFLAGS += -j$(shell nproc)
OPENOCD_SCRIPT_PATH ?= $(CURDIR)/openocd

# Target MCU settings
MCU=cortex-m33
MCU_FLAGS=-mcpu=$(MCU) -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
MCU_DEFS=-DSTM32 -DSTM32U585xx -DB_U585I_IOT02A -DSTM32U585AIIxQ -DSTM32U5 \
         -DARM_MATH_CM33 -DARM_MATH_DSP -DCMSIS_NN

# Compiler flags
CFLAGS  = $(MCU_FLAGS) -std=gnu11 -O0 -g -DNDEBUG $(MCU_DEFS) \
          -ffunction-sections -fdata-sections -DTF_LITE_STATIC_MEMORY \
          -Wall -fstack-usage --specs=nano.specs

CXXFLAGS = $(MCU_FLAGS) -std=gnu++17 -O0 -g -DNDEBUG $(MCU_DEFS) \
           -ffunction-sections -fdata-sections -DTF_LITE_STATIC_MEMORY \
           -Wall -fstack-usage --specs=nano.specs \
           -fno-rtti -fno-exceptions -fno-threadsafe-statics -Wnon-virtual-dtor

COMMON_FLAGS = -fno-unwind-tables -fno-asynchronous-unwind-tables \
               -ffunction-sections -fdata-sections -fmessage-length=0

# Assembler and linker flags
ASFLAGS = $(CFLAGS) -x assembler-with-cpp
LDFLAGS = -TSTM32U585xx_FLASH.ld --specs=nosys.specs \
          -Wl,-Map="firmware.map" -Wl,--gc-sections -static -lc -lm

# Dependency tracking
DEPFLAGS = -MMD -MP
CFLAGS  += $(DEPFLAGS)
CXXFLAGS += $(COMMON_FLAGS) $(DEPFLAGS)
ASFLAGS += $(DEPFLAGS)

# Directories
SRCDIR=app/src
OBJDIR=build
INCDIRS=drivers/CMSIS/Device/ST/STM32U5xx/Include \
        drivers/CMSIS/Core/Include \
        app/inc \
        mw/ \
        mw/third_party/cmsis_nn \
        mw/third_party/cmsis_nn/Include \
        mw/third_party/flatbuffers/include \
        mw/third_party/ruy \
        mw/third_party/gemmlowp \
        mw/third_party/kissfft

# Include paths
CFLAGS += $(addprefix -I,$(INCDIRS))
CXXFLAGS += $(addprefix -I,$(INCDIRS))

# CMSIS-NN optimized kernels
CMSIS_NN_KERNELS := add batch_matmul conv depthwise_conv fully_connected \
                    maximum_minimum mul pad pooling softmax svdf transpose \
                    transpose_conv unidirectional_sequence_lstm

# Cache expensive find operations
TFLM_ALL_SOURCES := $(shell find mw/tensorflow -name '*.cc' -o -name '*.cpp' 2>/dev/null)
CMSIS_NN_KERNEL_SOURCES := $(shell find mw/tensorflow/lite/micro/kernels/cmsis_nn -name '*.cc' -o -name '*.cpp' 2>/dev/null)
CMSIS_NN_LIB_SOURCES := $(shell find mw/third_party/cmsis_nn/Source -name '*.c' 2>/dev/null)

# Source files
C_SOURCES := $(wildcard $(SRCDIR)/*.c)
CPP_SOURCES := $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*.cc $(SRCDIR)/*.cxx)
ASOURCES := $(wildcard $(SRCDIR)/*.s)

# Conditional kernel selection
ifeq ($(findstring DCMSIS_NN,$(CXXFLAGS)),DCMSIS_NN)
  # Use CMSIS-NN optimized kernels
  GENERIC_EXCLUDE := $(foreach k,$(CMSIS_NN_KERNELS),mw/tensorflow/lite/micro/kernels/$(k).cc)
  TFLM_SOURCES := $(filter-out $(GENERIC_EXCLUDE) $(CMSIS_NN_KERNEL_SOURCES),$(TFLM_ALL_SOURCES))
  CPP_SOURCES += $(TFLM_SOURCES) $(CMSIS_NN_KERNEL_SOURCES)
  C_SOURCES += $(CMSIS_NN_LIB_SOURCES)
else
  # Use generic kernels
  TFLM_SOURCES := $(filter-out $(CMSIS_NN_KERNEL_SOURCES),$(TFLM_ALL_SOURCES))
  CPP_SOURCES += $(TFLM_SOURCES)
endif

SOURCES := $(C_SOURCES) $(CPP_SOURCES) $(ASOURCES)

# Object files
C_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(C_SOURCES))
CPP_OBJECTS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(CPP_SOURCES))
CPP_OBJECTS := $(patsubst %.cc,$(OBJDIR)/%.o,$(CPP_OBJECTS))
CPP_OBJECTS := $(patsubst %.cxx,$(OBJDIR)/%.o,$(CPP_OBJECTS))
ASM_OBJECTS := $(patsubst %.s,$(OBJDIR)/%.o,$(ASOURCES))
OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)

# Dependencies
-include $(OBJECTS:.o=.d)

# Targets
.PHONY: all clean clean-tf flash

all: firmware.bin

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cc | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cc | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.s | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.s | $(OBJDIR)
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

firmware.elf: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@

$(OBJDIR):
	mkdir -p $@

flash: firmware.bin
	@echo "Flashing..."
	openocd -s $(OPENOCD_SCRIPT_PATH) -f interface/stlink.cfg \
	        -f target/stm32u5x.cfg -c "program firmware.bin reset exit"

clean:
	rm -rf $(OBJDIR) firmware.elf firmware.bin firmware.map

clean-app:
	rm -rf $(OBJDIR)/app firmware.elf firmware.bin firmware.map

clean-tf:
	rm -rf $(OBJDIR)/mw firmware.elf firmware.bin firmware.map
