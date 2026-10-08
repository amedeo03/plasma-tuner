# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project goals

`plasma-tuner` is a plugin for the kde plasma 6 desktop environment: it's used to tune electric musical instruments connected as audio sources via audio interfaces. The plugin consists in one thing primarily: an applet icon that runs on system startup that, when clicked on, brings up a widget displaying the following elements:
* input selection: which microfone to listen on for tuning;
* instrument selection: which instrument you are currently tuning. For now the only available instruments are electric bass and guitar;
* tuning selection: which tuning do you want to tune to. This includes the most popular tunings, such as E standard, Drop D, D standard, Drop C, C standard;
* the current note being played: and how accurate is its in pitch using a classic "tachimeter" style indicator, providing both the note and its octave;

## Project state

Implemented: the applet builds, its unit tests pass, and it has been verified end to end in `plasmoidviewer`. The plugin ID is `io.github.amedeo03.plasmatuner`. It must stay consistent across `src/CMakeLists.txt` (target and `TRANSLATION_DOMAIN`), `src/metadata.json`, the QML import in `src/qml/FullRepresentation.qml`, and `Messages.sh`.

Design decisions agreed with the user:
- The tuner is **chromatic**: the needle measures against the nearest semitone. The selected tuning only lists target notes (a string is highlighted when the detected note matches it); it never changes the measurement.
- Instruments: guitar with 6/7/8 strings and bass with 4/5/6 strings. Drop tunings are named after the dropped string (Drop D on a 6-string, Drop A on a 7-string).
- The microphone is open only while the tuner is on screen (popup expanded, or the full representation shown inline).
- The reference pitch (A4) can be adjusted from 380 to 500 Hz in 0.1 Hz steps. All selections persist through the Plasmoid configuration.

## Build, test & run

Requires Qt ≥ 6.7 (Quick, Multimedia, Test), KF6 (ECM, I18n, Config) and Plasma (libplasma) dev packages.

```sh
cmake -B build -DBUILD_TESTING=ON -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure          # all tests
./build/bin/pitchdetectortest testOpenStrings       # one test function (QtTest syntax)

# Run without installing (preferred over installing):
QT_PLUGIN_PATH=$PWD/build/bin plasmoidviewer -a io.github.amedeo03.plasmatuner -s 520x760
# Panel/popup mode:
QT_PLUGIN_PATH=$PWD/build/bin plasmoidviewer -a io.github.amedeo03.plasmatuner -f horizontal -l bottomedge
```

`plasmoidviewer` wipes the applet's saved configuration (`~/.config/plasmoidviewer-appletsrc`) every time it starts, so a preset config will not survive a launch. To test a specific setup, assign `Plasmoid.configuration.*` from QML at runtime.

To test with a real signal without touching the user's audio devices, create a virtual source, play a generated WAV into it, select it as the input, then unload the modules:
`pactl load-module module-null-sink sink_name=tunertest` followed by `pactl load-module module-remap-source master=tunertest.monitor source_name=tunertest_src`, then `paplay -d tunertest tone.wav`. The Qt device id is the PulseAudio source name.

## Architecture

The data flow runs from the audio thread to the main thread and then to QML:

- **`src/core/`** (static library `tunercore`, no QML, unit tested in `autotests/`):
  - `pitchdetector`: McLeod Pitch Method (NSDF from an FFT autocorrelation, built-in radix-2 FFT, parabolic interpolation). The window is a power of two of about 160 ms, which is needed to catch G0 (≈24.5 Hz, the lowest string supported).
  - `notes`: frequency ↔ MIDI conversion and note names (sharps, scientific octave numbers).
  - `tunings`: the instrument and tuning catalog. Each tuning preset is a transposition of standard tuning plus an optional drop of the lowest string.
- **`AudioCapture`** (`src/audiocapture.*`): lives on a dedicated `QThread` so the DSP never runs on the plasmashell GUI thread, and must only be called through queued invocations. It wraps a `QAudioSource` (Float format if supported), keeps one buffer per channel, and analyses the **loudest channel**, because audio interfaces often carry the instrument on a single channel. It applies a −60 dBFS noise gate and emits `analysed(frequency, clarity, levelDb)` about 25 times per second.
- **`Tuner`** (`src/tuner.*`, `QML_ELEMENT`): the main-thread facade. It lists input devices (index 0 = system default with id ""; a saved device that is unplugged falls back to the default without being forgotten). It also drops detections with clarity below 0.8, applies a 5-sample median filter, and holds the last note for 600 ms after the signal fades. It exposes the note, octave, cents and level.
- **`TuningCatalog`** (`src/tuningcatalog.*`, `QML_SINGLETON`): exposes `core/tunings` to QML as lists of maps.
- **QML** (`src/qml/`): `main.qml` holds the `PlasmoidItem` and a custom compact icon (`tuner-symbolic.svg`, drawn as a mask). `FullRepresentation.qml` owns the `Tuner` instance and the controls, and sanitizes config values (for example a string count that is invalid for the current instrument). `TunerGauge.qml` is the QtQuick.Shapes tachometer (±50 cents across ±60°).
- **Config schema**: `src/config/main.xml` is bundled as a resource. `plasma_add_applet` flattens resources, so it ends up at the QML module root, which is where libplasma looks for it.

Other build details:
- C++20 is required and set explicitly, because the ECM defaults are older.
- New `.qml`, `.cpp` and resource files must be listed in `src/CMakeLists.txt`.
- Resources are bundled flat next to the QML, so refer to them as `Qt.resolvedUrl("./file")`.

Licensing: the whole project is LGPL-2.1-or-later (`LICENSE` at the root, also declared in `src/metadata.json`). Source files carry no SPDX or copyright headers; don't add them.
