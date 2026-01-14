# ArcadeHub Mini
Use any bluetooth gamepad on your Nintendo Switch or Nintendo Switch 2 with a Raspberry Pi Pico 2 W.

[List of supported controllers](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/)

This project is possible thanks to [Bluepad32](https://github.com/ricardoquesada/bluepad32) and [TinyUSB](https://github.com/hathach/tinyusb).
This project is a fork of [PicoSwitch-WirelessGamepadAdapter](https://github.com/juan518munoz/PicoSwitch-WirelessGamepadAdapter)

Multiple gamepads support (4 max)

## Updates from PicoSwitch-WirelessGamepadAdapter
- Outputs to Switch and Switch 2 as a Pro Controller
   - Please note that by connecting your controller this way it will disconnect from the adapter and sync with the Switch natively so the adapter is no longer being used and will automatically pair wirelessly without using the adapter again
   - Tested working with Wii U Pro Controller
   - PS5 controller does not pair back wirelessly
   - Testing more controllers
  - Corrected Wii U triggers so that they register properly
  - Corrected Wii Gamepad L/R and triggers so that they register properly
  - Added buttonn command of pressing +/- or Start/Select equivalents to disconnect the controller from the adapter (doesnt work on Switch because the switch will just switch to its own bluetooth instead of communicating via the adapter)



## Installing
1. Download latest `.uf2` file
2. Plug Pico on PC while holding the bootsel button.
3. A folder will appear, drag and drop the `.uf2` file inside it.

## Building
1. Install Make, CMake (at least version 3.13), and GCC cross compiler
   ```bash
   sudo apt-get install make cmake gdb-arm-none-eabi gcc-arm-none-eabi build-essential
   ```
2. (Optional) Install [Pico SDK](https://github.com/raspberrypi/pico-sdk) and set `PICO_SDK_PATH` environment variable to the SDK path. Not using the SDK will download it automatically for each build.
3. Update submodules
   ```bash
   make update
   ```
4. Build
   ```bash
   make build
   ```
5. Flash!
   ```bash
   make flash
   ```
   This `make` command will only work on OSes where the mounted pico drive is located in `/media/${USER}/RPI-RP2`. If this is not the case, you can manually copy the `.uf2` file located inside the `build` directory to the pico drive.

#### Other `make` commands:
- `clean` - Clean build directory.
- `flash_nuke` - Flash the pico with `flash_nuke.uf2` which will erase the flash memory. This is useful when the pico is stuck in a boot loop.
- `all` - `build` and `flash`.
- `format` - Format the code using `clang-format`. This requires `clang-format` to be installed.
- `debug` - Start _minicom_ to debug the pico. This requires `minicom` to be installed and uart debugging.

## Development roadmap
- [ ] Implement Mode Switching to PS3
- [ ] Implement Mode Switching to Wii U
- [ ] Implement Mode Switching to Wii
- [ ] Implement Gamecube Controller output
- [ ] Implement PS2 Controller output
- [ ] Implement OG Xbox Controller output 

## Acknowledgements
- [ricardoquesada](https://github.com/ricardoquesada) - maker of [Bluepad32](https://github.com/ricardoquesada/bluepad32)
- [hathach](https://github.com/hathach) creator of [TinyUSB](https://github.com/hathach/tinyusb)
- [splork](https://github.com/aveao/splork) and [retro-pico-switch](https://github.com/DavidPagels/retro-pico-switch) - for the hid descriptors and TinyUsb usage examples.
- [juan518munoz](https://github.com/juan518munoz) for the base of the project adapter

