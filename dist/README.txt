MINI CAR
========

Overview
--------
MINI CAR is a kiosk-style video player for an art installation. Four video
files are pre-loaded; one plays at a time. A physical potentiometer wired to
a USB-serial device drives the playback speed in real time, with inertia
smoothing. Two additional switches (connected through the same serial line)
pick which of the four clips is showing — a 2x2 matrix of "night on/off"
and "country on/off".

A built-in Sim Mode lets you operate the app without any hardware — useful
for testing, demos, or setting up a new machine.

========================
INSTALL
========================

1. Microsoft Visual C++ 2015-2022 Runtime (required, ~15 MB)
   Double-click vc_redist.x64.exe and accept. If it says "Already installed"
   that is fine — carry on. Without this, the .exe will fail with
   "VCRUNTIME140.dll was not found" or similar.

2. K-Lite Codec Pack Mega (recommended, ~70 MB)
   Download from https://codecguide.com/download_k-lite_codec_pack_mega.htm.
   Pick the Mega version (not Basic or Standard) — it bundles LAV Filters,
   which is what cleanly handles H.264 rate changes without stutter.

   Why: Windows' default Media Foundation handles most H.264 files fine, but
   some files and some rates cause the decoder to stall during speed changes.
   K-Lite Mega + LAV Filters is the most reliable fallback for an
   installation that runs for weeks without attention.

   If you are committed to zero third-party installs, the app also works on
   a machine with only Windows' built-in Media Foundation — test first.

3. Nothing else. No Python, no drivers (Windows provides USB CDC serial
   drivers out of the box), no .NET, no registry edits.

========================
LAUNCH
========================

Double-click "Start MINI CAR.bat".

That .bat does two things:
  1. prepends app\lib\ to PATH so the 65 runtime DLLs are discoverable
  2. cd's into app\ and launches mini-car-of.exe

You can also launch app\mini-car-of.exe directly if app\lib\ is on PATH.

Auto-start on boot: place a shortcut to "Start MINI CAR.bat" in
  %APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup
The app opens fullscreen, hides the cursor, and runs until Esc is pressed.

========================
CONTROLS
========================

Keyboard (any time):
  G           show / hide the GUI overlay (also shows the mouse cursor)
  F           toggle windowed / fullscreen
  Esc         save current settings and quit

Keyboard (only when Sim Mode is on — see GUI):
  Up / Down          speed byte -/+ 8 per press
  Left / Right       country off / on
  Page Up / Page Dn  night off / on
  0                  snap speed to minimum
  9                  snap speed to maximum

GUI panel (press G to show):
  Speed Scale        multiplier on the max playback rate. 1.0 maps the
                     potentiometer's max to 1.8x real-time. Raise for a
                     faster top end, lower for a gentler one.
  Inertia            smoothing on speed changes, 0.0 instant - 0.99 sludgy.
                     Higher = more glide between rates.
  USB Address        serial port. On Windows usually "COM3" etc. (check
                     Device Manager > Ports). The HUD shows connection
                     status live: green when opened, red on failure.
  Video Mode         0 = original (no scaling), 1 = fit (letterbox),
                     2 = fill (crop to fill screen).
  Invert             flip the potentiometer direction. Tick this if the
                     knob runs the video backwards.
  Sim Mode           use keyboard + the "Sim Byte" slider instead of the
                     real serial potentiometer. See Controls above for
                     the key bindings that become active.
  Sim Byte           fake potentiometer value (0-248). Only has an effect
                     when Sim Mode is on.

HUD lines (shown while the GUI is visible):
  Potentiometer:     the last speed byte seen from serial OR sim
  FPS                render frame rate
  Video URL          which file is currently on screen
  Video Size         decoded video dimensions
  Switches           A = night, B = country (1/0 flags)
  Speed / target     current playback rate and the inertia target
  serial OK/ERROR    green when the USB port opened, red otherwise
  SIM MODE           yellow banner shown while Sim Mode is on

========================
VIDEO FILES
========================

Where
  app\data\MINI CAR 01.mp4 .. MINI CAR 04.mp4

Which clip plays:
  index 0: country=off, night=off      MINI CAR 01
  index 1: country=off, night=on       MINI CAR 02
  index 2: country=on,  night=off      MINI CAR 03
  index 3: country=on,  night=on       MINI CAR 04

Supported codec (most reliable):
  Container:  .mp4 (QuickTime / MP4)
  Video:      H.264, yuv420p, progressive
  Profile:    High or Main
  Audio:      optional; AAC if present
  Resolution: 1920x1080 or below
  Frame rate: 30 or 60 fps
  Bitrate:    up to ~20 Mbps tested OK

Also works:
  .mov with H.264
  .mp4 with other codecs K-Lite can decode (HEVC, ProRes, etc.)

Avoid for this app (variable-rate friction):
  .avi with MJPEG — the MJPEG DirectShow filter on this Windows build
    rejects some rate changes; we tested and got frozen frames.
  Files larger than 1080p60 at 20+ Mbps — decode cost spikes during rate
    changes on modest hardware.

To change a video:
  1. Drop the new file in app\data\.
  2. Open app\data\config.json in any text editor.
  3. In the "videoFiles" array, replace the filename at the right index
     (see the night/country matrix above).
  4. Save. Relaunch the app (or press Esc then Start MINI CAR.bat again).

To re-encode an existing clip to the recommended profile:
  ffmpeg -i INPUT.mov -c:v libx264 -profile:v main -pix_fmt yuv420p ^
         -crf 20 -movflags +faststart OUTPUT.mp4
  (ffmpeg is free at https://ffmpeg.org. The app itself does not need it.)

========================
config.json REFERENCE
========================

Path: app\data\config.json

All fields:
  countryMode   boolean   last country toggle state (app saves it every 5 s)
  nightMode     boolean   last night toggle state (same)
  invert        boolean   potentiometer direction flip
  smoothScale   0.0-0.99  inertia amount (0 = instant, 0.99 = very slow)
  speedScale    0.0-3.0   max playback rate multiplier
  usbAddress    string    e.g. "COM3" on Windows, "/dev/tty.usbmodemXXXX"
                          on macOS. If wrong, HUD shows red; change in GUI
                          or edit this file.
  videoFiles    array[4]  filenames in app\data\, indexed by night/country.
  soundFiles    array[4]  optional .wav filenames to play alongside each
                          video (left empty since the originals are silent)
  videoMode     string    "original" | "fitScreen" | "fillScreen"

The app rewrites this file every 5 seconds and on Esc. Manual edits stick
only if you quit the app first.

========================
SERIAL PROTOCOL
========================

The app reads one byte at a time at 9600 baud, 8N1:
  3       country = off
  4       country = on
  5       night = off
  6       night = on
  >=7     speed byte; (byte - 7) is mapped through the "Invert" rule to a
          target speed in [0.0 .. 1.8 * speedScale]

To emulate this from an Arduino/firmware, just write single bytes on the
serial line. No framing, no checksum, no baud negotiation.

========================
TROUBLESHOOTING
========================

"VCRUNTIME140.dll was not found" when launching
  -> Install step 1 above (vc_redist.x64.exe).

Black screen on launch
  -> Press G to open the GUI. If "Video URL" is blank, the filename in
     config.json does not match anything in app\data\. Fix and relaunch.
  -> If "Video Size" is 0x0 the file loaded but decoding failed. Install
     K-Lite Mega (step 2 above) or re-encode the clip with the ffmpeg
     recipe under VIDEO FILES.

Frozen image but audio plays (or no motion)
  -> The codec's rate control rejected the current speed. Lower
     "Speed Scale" and "Inertia" in the GUI. If the problem persists on
     one specific file, re-encode that clip.

Red "serial ERROR" on the HUD
  -> The USB device is not present or the address is wrong.
  -> Plug in the device, then type the correct COM port into "USB Address"
     in the GUI. The HUD should flip to green the next frame.
  -> Or tick Sim Mode to bypass serial entirely for testing.

Speed is inverted (knob fully right = no motion)
  -> Toggle the "Invert" checkbox in the GUI.

App does not launch, no error
  -> Open a Command Prompt, cd into the extracted folder, run
     "Start MINI CAR.bat" from there. You will see any startup error in
     the console window.

========================
FILES / LAYOUT
========================

MINI CAR\
  Start MINI CAR.bat        launcher (sets PATH, runs mini-car-of.exe)
  README.txt                this file
  vc_redist.x64.exe         Microsoft VC++ runtime installer
  app\
    mini-car-of.exe         the application
    data\
      config.json           all persistent settings
      MINI CAR 01..04.mp4   the four video clips
    lib\                    65 runtime DLLs (Cairo, FreeImage, GLEW, GLFW,
                            freetype, libcurl, openal, openssl, etc.)

========================
BUILD FROM SOURCE
========================

Requirements
  - Windows 10 or 11, x64.
  - Visual Studio 2022 Build Tools (or Community) with the
    "Desktop development with C++" workload installed. That brings the
    v143 toolset, MSBuild, and Windows 11 SDK. Free download:
      https://visualstudio.microsoft.com/downloads/
    Silent CLI:
      winget install Microsoft.VisualStudio.2022.BuildTools ^
        --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools ^
        --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --includeRecommended"
    (Elevated PowerShell.)
  - openFrameworks 0.12.1 VS 64-bit release:
      https://github.com/openframeworks/openFrameworks/releases/tag/0.12.1
    Grab of_v0.12.1_vs_64_release.zip and extract, e.g. to C:\of_vs\.

Get the project
  Clone the repo into your oF install's apps\myApps folder:
    cd C:\of_vs\apps\myApps
    git clone https://github.com/martial/mini-car-of.git

Compile (Release x64)
  "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" ^
    mini-car-of.sln /p:Configuration=Release /p:Platform=x64
  (Or open mini-car-of.sln in Visual Studio and Build > Build Solution.)

  Output: bin\mini-car-of.exe. All required DLLs get dropped in bin\ by the
  post-build step.

Test
  cd bin
  mini-car-of.exe
  (Run directly; bin\ contains the exe and all DLLs. The packaged
  Start MINI CAR.bat layout is only needed once you split DLLs into a
  lib\ subfolder, which is done in dist\.)

Make a distribution package
  After a successful build:
  1. Create dist\MINI CAR\app\, move bin\mini-car-of.exe + bin\data there.
  2. Move all bin\*.dll files into dist\MINI CAR\app\lib\.
  3. Copy vc_redist.x64.exe (https://aka.ms/vs/17/release/vc_redist.x64.exe)
     and a copy of this README.txt into dist\MINI CAR\.
  4. Drop in Start MINI CAR.bat (see this package for reference).
  5. Compress-Archive the folder into MINI CAR.zip for sharing.

Project layout (source)
  src\main.cpp            window setup, runApp bootstrap
  src\ofApp.h             all app logic (header-only; no ofApp.cpp)
  addons.make             ofxGui
  config.make             per-project compiler / linker overrides
  bin\data\config.json    runtime settings
  bin\data\*.mp4          video assets

Adding an addon
  1. Drop the addon under C:\of_vs\addons\ (git clone it or copy the source).
  2. Append its name to addons.make, one per line.
  3. Regenerate the vcxproj: run C:\of_vs\projectGenerator\projectGenerator.exe,
     set the project folder to this repo, pick "vs" as target, click Update.
     That rewrites mini-car-of.vcxproj with the new include + source paths.
  4. Rebuild.

========================
CREDITS / BUILD
========================

Built against openFrameworks 0.12.1 (VS 64-bit release) with Visual Studio
2022 Build Tools (v143 toolset). Video playback via ofVideoPlayer backed
by Media Foundation; GUI via ofxGui; serial via ofSerial.
