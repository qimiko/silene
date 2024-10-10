# 🌸 Silene
*Temporary name.*

An Android emulator that exclusively runs[^1] Geometry Dash.
Currently targeting the first ARMv7 version of Geometry Dash, 1.6. Other versions may come later.

Progress may be slow.

[^1]: Runs is subjective.

## Progress

### What works?

- It gets ingame
- On older versions, it can even play a level!

### What doesn't work?

- Everything.

See the [TODO list](./TODO.md) for more information.

## Usage

### Building

Building is done with CMake. You can build the project by using these commands:

1. `cmake -B build`
2. `cmake --build build`

Feel free to adapt those commands to fit your own needs.

On Linux, you'll need to install the OpenGL and GLFW dev packages. For macOS, these dependencies are bundled inside of the repo.

### Running

This is currently a command line program. Only one argument is required, that being the path to the APK file you want to run.

`./silene test/gd19.apk`

You can enable verbose logging with the `--verbose` flag.  
If you like to use GDB, you can also enable debugging with the `--debug` flag.

If you want to use a newer version of Geometry Dash that splits the APK, the `--resources` flag will redirect resource loading accordingly.

## License

This project is currently licensed under the [MIT License](./LICENSE.txt).
