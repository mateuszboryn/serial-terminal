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

---

# Windows

## Prerequisites

- **Visual Studio / MSVC Toolset**: Visual Studio 2019 (v142) or Visual Studio 2022 (v143) with C++ desktop development tools (`x64`).
- **Qt 5.15.2 (msvc2019_64)**: Installed at `D:/Qt/5.15.2/msvc2019_64` (or configured via `CMAKE_PREFIX_PATH` / `QT_CMAKE_DIR`).
- **CMake & Ninja**: CMake 3.16+ and Ninja build system.
- **NSIS (Nullsoft Scriptable Install System)**: Required for building Windows installer packages (`makensis.exe` in PATH or standard installation directory `C:\Program Files (x86)\NSIS`).

## Visual Studio Redistributable (VCRedist) for `msvc2019_64`

The application requires the Microsoft Visual C++ 2015–2022 Redistributable (`x64`) runtime.

### Download Links
- **Direct Download (Official Microsoft URL)**: [https://aka.ms/vs/17/release/vc_redist.x64.exe](https://aka.ms/vs/17/release/vc_redist.x64.exe) (or [https://aka.ms/vs/16/release/vc_redist.x64.exe](https://aka.ms/vs/16/release/vc_redist.x64.exe))
- **Documentation & Supported Versions**: [https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)

### Download via PowerShell
To download `vc_redist.x64.exe` using PowerShell:

```powershell
Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vc_redist.x64.exe" -OutFile "vc_redist.x64.exe"
```

### Download via Command Line (curl)
```shell
curl -L -o vc_redist.x64.exe https://aka.ms/vs/17/release/vc_redist.x64.exe
```

> **Note**: During CMake configuration on Windows, CMake locates the Visual Studio redistributable package from the local MSVC directory (`MSVC_REDIST_DIR`) without requiring internet access.

## Build

Open the **x64 Native Tools Command Prompt for VS** (or load `VsDevCmd.bat -arch=x64`) and run:

```shell
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build cmake-build-release --config Release
```

## Package (.exe via NSIS)

To generate the standalone NSIS installer:

```shell
cmake --build cmake-build-release --config Release --target package
```

Alternatively, invoke `cpack` directly from the build directory:

```powershell
cd cmake-build-release
cpack -G NSIS
```

The generated installer will be located in `cmake-build-release/serial-terminal-0.1-win64.exe`.

### What the NSIS Package Includes
1. **Application Executable**: `bin/serial-terminal.exe` and Start Menu shortcut.
2. **Qt5 Libraries & Plugins**: Automatically deployed using `windeployqt`:
   - Core libraries: `Qt5Widgets.dll`, `Qt5SerialPort.dll`, `Qt5Gui.dll`, `Qt5Core.dll`, `Qt5Svg.dll`
   - Platform plugin: `platforms/qwindows.dll` (essential for Qt GUI window creation on Windows)
   - Style & icon plugins: `styles/qwindowsvistastyle.dll`, `iconengines/qsvgicon.dll`, `imageformats/`
   - Rendering libraries: `libEGL.dll`, `libGLESv2.dll`, `d3dcompiler_47.dll`, `opengl32sw.dll`
3. **Visual C++ Redistributable (`vcredist/vc_redist.x64.exe`)**: Packaged within the installer and automatically run in passive mode during installation to ensure all required MSVC runtime libraries are present on the target system.
