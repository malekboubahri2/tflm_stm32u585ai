# TFLM Sine Wave Regression on STM32 B-U585AI-IOT02A

An end-to-end TinyML project running an int8-quantized sine wave 
regression model on the STM32 B-U585AI-IOT02A using TensorFlow 
Lite for Microcontrollers (TFLM), built with a custom Makefile 
toolchain and CMSIS-NN kernel acceleration.

## Hardware
- Board: STM32 B-U585AI-IOT02A
- MCU: STM32U585AIIxQ (Cortex-M33, FPV5-SP-D16 FPU, DSP extensions)
- Debug output: ITM/SWO via `ITM_SendChar`

## Development Environment

The entire build environment is containerized using VS Code Dev Containers — 
no local toolchain installation required.
```json
// .devcontainer/devcontainer.json
```

To get started:
1. Install [VS Code](https://code.visualstudio.com/) and the 
   [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
2. Clone the repo and open in VS Code
3. When prompted, click **Reopen in Container**
4. The container automatically installs `arm-none-eabi-gcc`, OpenOCD, 
   and all dependencies via `.devcontainer/scripts/setup.sh`
5. Run `make` inside the container terminal

## Debugging

Full embedded debugging is configured out of the box via VS Code and 
the Cortex-Debug extension.

**Setup:**
- Debugger: `arm-none-eabi-gdb` via OpenOCD + ST-Link
- SVD file: `STM32U585.svd` — enables peripheral register inspection 
  (GPIO, DMA, CRYP, etc.) directly in VS Code
- Entry point: auto-breaks at `main` on launch
- Pre-launch task: automatically flashes firmware before debug session starts

**SWO Tracing:**
- CPU frequency: 160 MHz
- SWO frequency: 2 MHz
- ITM channel 0 decoded as console — inference output streams live 
  during debug session

**To start a debug session:**
1. Open the repo in the Dev Container
2. Press `F5` in VS Code
3. Firmware builds, flashes, and halts at `main` automatically
4. Set breakpoints anywhere — including inside `run_inference()` 
   to inspect quantized tensors and intermediate values live
```json
// .vscode/launch.json — cortex-debug config
"svdFile": "drivers/CMSIS/Device/ST/STM32U5xx/Include/STM32U585.svd",
"swoConfig": {
    "enabled": true,
    "cpuFrequency": 160000000,
    "swoFrequency":  2000000,
    "source": "probe",
    "decoders": [{ "port": 0, "type": "console" }]
}
```

## Model
- Task: Sine wave regression — predict sin(x) from x
- Training: TensorFlow/Keras in Python
- Quantization: Post-training int8
- Tensor arena: 2KB statically allocated, 16-byte aligned
- Format: TFLite flatbuffer embedded as C array (`sine_model_data.cc`)

## Inference Pipeline
```c
// Quantize input
int8_t x_quantized = x / input->params.scale + input->params.zero_point;

// Run inference
interpreter->Invoke();

// Dequantize output
y = (y_quantized - output->params.zero_point) * output->params.scale;
```

The model runs two inference modes:
- `run_inference()` — sweeps x over [0, 2π] across a fixed cycle count
- `run_sensor_inference(uint16_t)` — maps a 12-bit ADC value (0–4095) 
  to [0, 2π] and runs inference on live sensor input

## Compiler Flags & Hardware Acceleration
```makefile
-mcpu=cortex-m33
-mfpu=fpv5-sp-d16 -mfloat-abi=hard   # hard FPU ABI
-DARM_MATH_CM33 -DARM_MATH_DSP        # DSP extension macros
-DCMSIS_NN                             # CMSIS-NN kernel acceleration
--specs=nano.specs                     # newlib-nano for minimal footprint
```

CMSIS-NN replaces generic TFLM kernels with Cortex-M33 optimized 
fixed-point implementations, leveraging DSP SIMD instructions for 
int8 arithmetic.

## Project Structure
```
tflite-micro/
├── app/
│   ├── inc/
│   │   ├── constants.h          # kInferencesPerCycle, kXrange
│   │   ├── output_handler.h     # SWO output declaration
│   │   ├── sine_model_data.h    # Model array declaration
│   │   └── system_ai.h          # configure_model / run_inference API
│   └── src/
│       ├── main.cpp             # Entry point, ITM printf redirect
│       ├── system_ai.cc         # TFLM setup & inference runner
│       ├── sine_model_data.cc   # Flatbuffer model as C array
│       ├── output_handler.cc    # SWO logging of x and predicted y
│       ├── constants.cc         # Inference cycle constants
│       └── startup_stm32u585xx.s
├── drivers/CMSIS/               # CMSIS-Core & device headers
├── mw/tensorflow/               # TFLM source tree
├── openocd/                     # OpenOCD config for ST-Link + STM32U5
├── STM32U585xx_FLASH.ld         # Linker script
└── Makefile
```

## Getting Started

### Requirements
- `arm-none-eabi-gcc` toolchain
- OpenOCD with ST-Link support
- SWO viewer (STM32CubeIDE, Ozone, or pyocd)

### Build
```bash
make
```

### Flash
```bash
make flash
```

Uses OpenOCD with `interface/stlink.cfg` and `target/stm32u5x.cfg`.

### View Output

Open your SWO viewer on ITM channel 0. You should see:
```
Start Model!
x: 0.000000   y: 0.001463
x: 0.100000   y: 0.099833
x: 0.200000   y: 0.198669
...
```

## Key Concepts Demonstrated
- Post-training int8 quantization and manual quant/dequant pipeline
- CMSIS-NN kernel acceleration on Cortex-M33
- Hard FPU ABI with FPV5-SP-D16 for dequantization arithmetic
- ITM/SWO printf redirect without UART overhead
- Dual inference modes: cyclic sweep and live ADC sensor input
- Minimal 2KB tensor arena with 16-byte alignment
- Fully reproducible build environment via VS Code Dev Containers