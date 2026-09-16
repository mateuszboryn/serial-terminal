# Linux

## Prerequisites

Install required Qt5 development libraries and build tools:

```shell
sudo apt update
sudo apt install qtbase5-dev libqt5serialport5-dev build-essential cmake ninja-build
```

Grant user access to serial ports:

```shell
# grant user access to the dialout group
sudo usermod -a -G dialout $USER
# then reboot machine or log out and back in.
```

## Build

Configure and build the project:

```shell
cmake -B cmake-build-release-system -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release-system
```

## Package (.deb)

To build the `.deb` package:

```shell
cmake --build cmake-build-release-system --target package
```

Alternatively, run `cpack` directly from the build folder:

```shell
cd cmake-build-release-system && cpack
```

The generated `.deb` package will be located in `cmake-build-release-system/serial-terminal-0.1-Linux.deb`.

## Inspect Package

To list files to be installed by the package (without unpacking):

```shell
dpkg-deb -c cmake-build-release-system/serial-terminal-0.1-Linux.deb
```

To list package dependencies:

```shell
dpkg-deb -I cmake-build-release-system/serial-terminal-0.1-Linux.deb
```

Or query only the dependency field directly:

```shell
dpkg-deb -f cmake-build-release-system/serial-terminal-0.1-Linux.deb Depends
```

```shell
ldd cmake-build-release-system/serial-terminal
```
