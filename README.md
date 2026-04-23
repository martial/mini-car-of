# mini-car-of

An openFrameworks kiosk app for a video installation. Four H.264 clips are
pre-loaded; one plays at a time. A physical potentiometer wired to a
USB-serial device drives playback speed in real time (with inertia smoothing);
two toggle switches on the same serial line pick which of the four clips is
on screen — a 2×2 matrix of **night** × **country**.

A built-in **Sim Mode** drives the whole thing from the keyboard + GUI so the
app is usable without the physical rig for testing or demos.

> End-user install / troubleshooting lives in [`dist/README.txt`](dist/README.txt).
> This file is for developers landing on the repo.

## Build

Requires:
- **Windows 10+ x64**
- **Visual Studio 2022 Build Tools** (or Community) with the *Desktop development with C++* workload
- **openFrameworks 0.12.1 VS 64-bit release** extracted to e.g. `C:\of_vs\`
  (from <https://github.com/openframeworks/openFrameworks/releases/tag/0.12.1>)

Clone into the oF apps folder and build:

```bat
cd C:\of_vs\apps\myApps
git clone https://github.com/martial/mini-car-of.git
cd mini-car-of
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" ^
  mini-car-of.sln /p:Configuration=Release /p:Platform=x64
```

Output: `bin\mini-car-of.exe` with all runtime DLLs dropped alongside.

Full build / packaging steps (VC redist, DLL layout, zip): see
[`dist/README.txt` → BUILD FROM SOURCE](dist/README.txt).

## Run

After a successful build:

```bat
cd bin
mini-car-of.exe
```

For distribution on a clean machine, ship the `dist/MINI CAR/` folder
produced by the packaging steps — users double-click `Start MINI CAR.bat`.

## Controls

- `G` — show / hide the GUI overlay
- `F` — toggle fullscreen
- `Esc` — save settings and quit

Full keyboard reference (including Sim Mode bindings) is in `dist/README.txt`.

## Serial protocol

One byte at a time, 9600 baud, 8N1:

| Byte | Meaning |
|------|---------|
| `3`  | country = off |
| `4`  | country = on |
| `5`  | night = off |
| `6`  | night = on |
| `≥7` | speed byte; `byte − 7` mapped through `computeSpeed()` |

## Project layout

| Path | Purpose |
|------|---------|
| `src/main.cpp` | window setup + `runApp` bootstrap |
| `src/ofApp.h` | all app logic (header-only; no `ofApp.cpp`) |
| `addons.make` | lists `ofxGui` |
| `config.make` | per-project compiler / linker overrides |
| `bin/data/config.json` | runtime settings (persisted every 5 s) |
| `bin/data/*.mp4` | the four video assets |
| `dist/README.txt` | end-user install + run guide |
| `dist/Start MINI CAR.bat` | launcher script for the packaged zip |
| `mini-car-of.vcxproj` | Visual Studio project (v143 toolset) |
| `Makefile` | oF's makefile-based build (Linux / macOS / MSYS2) |

## Credits

Built on [openFrameworks](https://openframeworks.cc/) 0.12.1 with `ofxGui`,
compiled via Visual Studio 2022 Build Tools (v143). Video playback via
`ofVideoPlayer` backed by Media Foundation; serial via `ofSerial`.
