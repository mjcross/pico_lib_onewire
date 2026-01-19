# OneWire library for the RPi Pico and Pico 2 'C' SDK
A version of my OneWire driver from the RPi Pico examples repository packaged for easy inclusion as a CMake subdirectory.

For a simple demonstration of how to use this libary see [pico_ds18b20_example](https://github.com/mjcross/pico_ds18b20_example).

The library does not currently support 'overdrive' (high speed) mode.

## about OneWire
[*OneWire / 1-wire*](https://en.wikipedia.org/wiki/1-Wire) is a serial bus designed by Dallas semiconductor for a master to communicate with a *micro-LAN* of digital sensors or other devices over a single bi-directional data line.

Devices are simply wired in parallel to the bus which must be pulled up to the positive supply with a single 4.7k resistor. Each device has a unique 64bit id that enables the controller to addresss it individually.

Do not power the devices at more than 3.3V as it will damage the Pico GPIO pin used to read the bus.

## importing the library
The library is designed to be included in a project as a [CMake subdirectory](https://cmake.org/cmake/help/latest/guide/tutorial/Getting%20Started%20with%20CMake.html#exercise-4-subdirectories).

First fork this repository on GitHub or clone a local copy then change to your project directory and do:
```
$ git submodule add <path-to-library-repository or URL>
```
This is preferable to just copy/pasting the source folder because it ensures any customisations you make to the library are shared across all your projects.

Note that anyone cloning your repositiory should now use `git clone --recursive` to make sure they get the full source tree.

## including the library in your build
Once you've imported the library you can include it in your CMake build by including the following lines in your top level CMakeLists.txt:
```
add_subdirectory(<library-folder-name>)
target_link_libraries(<project-name> pico_lib_onewire)
```
You should now be able to `#include "onewire.h"` in your source code and start using the library functions.

## using the library
The library presents a straightforward API: simply call `onewire_init()` to create an instance of the library, then `onewire_bus_scan()` to see what devices are connected and talk to them with `onewire_reset()`, `onewire_send()` and `onewire_read()`.

A typical command sequence is:
- master asserts RESET
- master sends a ROM Command, tyically
    - `OW_SKIP_ROM` to address all devices, or 
    - `OW_MATCH_ROM` followed by the device ID to address a single device
- master sends a device-specific FUNCTION command (see datasheet for device)
- naster reads or writes some data to/from the device

Note that every transaction starts with the master asserting RESET.

### API documentation

----
`onewire_t onewire_init(uint pio_num, uint gpio)`

Initialise the library and return an instance handle (must be called before other functions)

**Inputs:** 
- PIO device number *(0 to NUM_PIOS - 1)*
- GPIO pin number

**Outputs:** 
- returns an instance of the library on success, or NULL if the given PIO had insuffient resources

The current version of the driver uses 17 PIO instructions and one state machine. You can install multiple instances of the library but each must use a different GPIO.

----
`int onewire_bus_scan (onewire_t ow, onewire_id_t *id_list, int maxdevs, uint8_t search_command)`

Detect connected devices and their unique 64bit ids (rom codes)

**Inputs:**
- library instance
- pointer to user-allocated space for the ID list, or NULL if not required
- maximum number of devices to detect, or 0 for no limit
- type of search: `OW_SEARCH_ROM` for all devices or `OW_ALARM_SEARCH` for only devices in an alarm state

**Outputs:**
- returns the number of devices found, or -1 if an error occurred
- optionally stores a list of 64bit ids (rom codes)

The search algorithm takes approximately 14ms per connected device. The returned device IDs can be decoded using the `.family`, `.serial` and `.crc` members of the provided `onewire_id_t` type, or accessed as a `.raw` 64bit value; and verfied with `onewire_check_crc()` (see below).

----
`bool onewire_check_crc(onewire_id_t id)`

Check the CRC of a device ID.

**Inputs:**
- pointer to a device ID