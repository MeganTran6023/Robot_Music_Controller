# Real - Time Music Robot Controller

## Wiring Diagram

<img width="3000" height="2188" alt="image" src="https://github.com/user-attachments/assets/b9a07dbd-b28d-4b6a-9256-39032759d7b1" />

# Blink Project — RTOS Playground

A beginner's guide to this FreeRTOS project running on a Raspberry Pi Pico W. It uses two concurrent tasks — one that reads a joystick and buzzer, and one that animates a face on an I2C LCD — to demonstrate task priorities, delays, and (as a follow-up exercise) mutexes.

## What this project does

`main.c` creates two FreeRTOS tasks and starts the scheduler:

1. **`vInteractionTask`** (priority 2, the higher-priority task) — plays a short buzzer tone, then listens for 2.5 seconds for joystick movement or a button press. If input is detected, it plays a second, higher-pitched "success" buzzer. Either way, it waits 4 seconds and repeats. All of this is logged to the serial monitor with `printf`.
2. **`vBlinkTask`** (priority 1, the lower-priority task) — drives a 16x2 character LCD over I2C (through a PCF8574 backpack) to draw a simple blinking face using custom characters.

`shared_counter` and `xMutex` are declared but not yet used in the task loops — they're there for a follow-up exercise in synchronizing access to shared data with `xSemaphoreTake()` / `xSemaphoreGive()` (see `notes.txt` for the reasoning).

## Files in this folder

| File | Purpose |
|---|---|
| `main.c` | The application: pin/LCD definitions, LCD helper functions, and the two FreeRTOS tasks. |
| `FreeRTOSConfig.h` | FreeRTOS kernel configuration (tick rate, heap size, priorities, which API functions are compiled in, etc.). Required by every FreeRTOS project. |
| `CMakeLists.txt` | Build configuration — tells CMake where to find the Pico SDK and FreeRTOS kernel, which board to target (`pico_w`), and which libraries to link (`hardware_i2c`, `hardware_adc`, etc.). |
| `notes.txt` | Q&A notes on the RTOS concepts used here (delay vs. busy-wait, mutexes, task starvation). Worth reading alongside the code. |

The `copy` variants of these files (`main copy.c`, `CMakeLists copy 2.txt`, etc.) are earlier drafts kept for reference — they aren't part of the active build.

## Hardware you'll need

- Raspberry Pi Pico W
- 16x2 character LCD with a PCF8574 I2C backpack, wired to GPIO 4 (SDA) and GPIO 5 (SCL)
- A buzzer on GPIO 15
- A 2-axis joystick with a button, wired to GPIO 26 (X), GPIO 27 (Y), and GPIO 22 (button)

## One-time setup

Before building, you need three things installed on your machine:

- **CMake**
- **Arm GNU Toolchain** (`arm-none-eabi-gcc`) — this project was set up with version 12.3 rel1
- **MinGW** (provides `mingw32-make`, since these steps use the `"MinGW Makefiles"` CMake generator on Windows)
- The **Pico SDK** and **FreeRTOS Kernel** source trees, downloaded/cloned somewhere on disk

### Environment variables

`CMakeLists.txt` reads `PICO_SDK_PATH` and `FREERTOS_KERNEL_PATH` from your environment (it even prints their values at configure time so you can double check them). Set them to wherever you placed the SDK and kernel:

```bash
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
export FREERTOS_KERNEL_PATH="$HOME/pico/FreeRTOS-Kernel"
```

On Windows, these paths will look more like:

```
PICO_SDK_PATH:        C:/Users/megantran/Downloads/test_rpi_pico/freertos-pico/pico-sdk
FREERTOS_KERNEL_PATH:  C:/Users/megantran/Downloads/test_rpi_pico/freertos-pico/FreeRTOS-Kernel
```

You'll also need the Arm GNU Toolchain's `bin` folder available, e.g.:

```
C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.3 rel1\bin
```

When CMake runs, it prints these back out so you can confirm they're pointing at the right place:

```
-- --- DEBUG ENVIRONMENT VARIABLES ---
-- PICO_SDK_PATH: 'C:/Users/megantran/Downloads/test_rpi_pico/freertos-pico/pico-sdk'
-- FREERTOS_KERNEL_PATH: 'C:/Users/megantran/Downloads/test_rpi_pico/freertos-pico/FreeRTOS-Kernel'
```

If either path prints blank, the environment variable isn't set in the shell you're building from — set it there before continuing.

## Building (Windows)

From inside this `blink` folder:

1. **Create and enter a build folder, then configure with CMake.** Since the Arm toolchain isn't necessarily on your `PATH`, point CMake at the compiler executables directly:

   ```
   mkdir build
   cd build
   cmake -G "MinGW Makefiles" -DPICO_BOARD=pico_w -DCMAKE_C_COMPILER="C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.3 rel1/bin/arm-none-eabi-gcc.exe" -DCMAKE_CXX_COMPILER="C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.3 rel1/bin/arm-none-eabi-g++.exe" -DCMAKE_ASM_COMPILER="C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.3 rel1/bin/arm-none-eabi-gcc.exe" ..
   ```

   If your toolchain's `bin` folder is already on your system `PATH`, you can skip the three `-DCMAKE_*_COMPILER` flags and just run:

   ```
   cmake -G "MinGW Makefiles" ..
   ```

2. **Build:**

   ```
   mingw32-make
   ```

   (or plain `make`, depending on how your MinGW install named it — check which one exists in your MinGW `bin` folder if the command isn't found).

3. **Flash the Pico.** Hold the **BOOTSEL** button on the Pico W while plugging it into USB (or while pressing reset), so it mounts as a USB drive. Then drag the generated `main.uf2` file from the `build` folder onto that drive. The Pico will automatically reboot and start running the program.
