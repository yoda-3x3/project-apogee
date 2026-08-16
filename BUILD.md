# Building Project Apogee

## Prerequisites

- Qt 6.9.3 (mingw_64 kit), installed at `C:\Users\reach\Dance\Qt\6.9.3\mingw_64`,
  with its bundled MinGW 13.1.0 at `C:\Users\reach\Dance\Qt\Tools\mingw1310_64`.
- CMake, on PATH or at `C:\Program Files\CMake\bin`.

No vcpkg and no extra Qt modules (Charts/WebEngine/Location) are required —
this project only uses Qt Core/Widgets/Gui/Network/Sql/Svg/SvgWidgets, all of
which ship in the base Qt Widgets install.

## Build

From PowerShell, in the repo root:

```powershell
./build.ps1              # core + data + tests, headless (no GUI)
./build.ps1 -BuildGui     # also builds the apogee_studio.exe GUI
./build.ps1 -Clean        # wipe build/ and reconfigure from scratch
./build.ps1 -SkipTests    # configure + build only, skip running tests
```

`build.ps1` pins the Qt-matched MinGW compiler to `PATH` for its own process
only — it does not touch your permanent PATH. This machine has a second,
unrelated MinGW installation (used by a different project); always build via
this script rather than a bare `cmake --build` from an arbitrary terminal, to
avoid an ABI mismatch.

Outputs (Qt's project setup places all executables directly in `build\`,
not per-subdirectory):
- `build\apogee_tests.exe` — Catch2 unit tests for `core`/`data`.
- `build\apogee_studio.exe` — the GUI (only with `-BuildGui`).
