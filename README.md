# OneWire library for the RPi Pico and Pico 2 'C' SDK
A version of my OneWire driver from the RPi Pico examples repository packaged for easy inclusion as a CMake subdirectory.

For a simple demonstration of how to use this libary see [pico_ds18b20_example](https://github.com/mjcross/pico_ds18b20_example).

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
target_link_libraries(<project-name> <library-name>)
```
You should now be able to `#include` the library header in your source code without prefixing it with the folder name and start using the library functions.
