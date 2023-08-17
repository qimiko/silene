# 🌸 Silene
*Temporary name.*

An Android emulator that exclusively runs[^1] Geometry Dash.
Currently targeting the first ARMv7 version of Geometry Dash, 1.6. Other versions may come later.

Progress may be slow.

[^1]: Runs is subjective.

## Progress

### What works?

- Nothing.

### What doesn't work?

- Everything.

See the [TODO list](./TODO.md) for more information.

## Usage

### Building

Building is done with CMake. You can build the project by using these commands:

1. `cmake -B build`
2. `cmake --build build`

Feel free to adapt those commands to fit your own needs.

Unfortunately, [Boost](https://www.boost.org/) must be installed on your system to successfully build.

### Running

This is currently a command line program. Only one argument is taken, that being the path to the .so file you want to run.

`./silene test/minimal-library/build/libtest-library.so`

> [!NOTE]  
> This will probably spam your console/explode. Redirecting the log output is very recommended.

## License

This project is currently licensed under the [MIT License](./LICENSE.txt).
