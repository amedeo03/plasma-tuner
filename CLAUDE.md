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
- Instruments and tunings come from a user-editable JSON file, `~/.config/plasma-tuner/instruments.json`, shared by all widget instances. It is created from the built-in defaults (`src/core/defaultinstruments.json`: guitar 6/7/8 and bass 4/5/6 strings, five tunings each) when missing, and reloaded live when it changes. Format: `{"instruments": [{"name", "stringCount", "tunings": [{"name", "stringNotes": ["E2", ...]}]}]}`, with notes listed from the lowest string.
- Validation rules:
  - names are required and unique (an instrument among instruments, a tuning within its instrument);
  - `stringCount` is a whole number from 1 to 12;
  - `stringNotes` length equals `stringCount`;
  - notes are a letter A–G, an optional `#` or `b` (not ♯/♭, not lowercase) and an octave, from C0 to C8.
- Invalid instruments and tunings stay in their dropdowns with a warning icon, and show their error in place of the target notes when selected (the needle keeps working). A file that is unusable as a whole (syntax error, no `instruments` array) shows an error banner and falls back to the built-in instruments.
- Selected instrument and tuning are saved **by name**, so the selection survives reordering. A missing name falls back to the first valid entry.
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
QT_PLUGIN_PATH=$PWD/build/bin plasmoidviewer -a io.github.amedeo03.plasmatuner -s 520x800
# Panel/popup mode:
QT_PLUGIN_PATH=$PWD/build/bin plasmoidviewer -a io.github.amedeo03.plasmatuner -f horizontal -l bottomedge
```

Always pass a window size (`-s 520x800`): the viewer places the applet at the window's centre, so the default 640×480 window shows only the gauge.

Run the viewer with `XDG_CONFIG_HOME=/tmp/<dir>` to test instruments files without touching the user's real `~/.config/plasma-tuner/instruments.json`.

Prefer log-based checks over screenshots: the user may be using the desktop, and `spectacle -a` captures whatever window has focus. QML `console.log` output only reaches stderr with `QT_FORCE_STDERR_LOGGING=1 QT_LOGGING_RULES="qml.debug=true;js.debug=true"`.

`plasmoidviewer` wipes the applet's saved configuration (`~/.config/plasmoidviewer-appletsrc`) every time it starts, so a preset config will not survive a launch. To test a specific setup, assign `Plasmoid.configuration.*` from QML at runtime.

To test with a real signal without touching the user's audio devices, create a virtual source, play a generated WAV into it, select it as the input, then unload the modules:
`pactl load-module module-null-sink sink_name=tunertest` followed by `pactl load-module module-remap-source master=tunertest.monitor source_name=tunertest_src`, then `paplay -d tunertest tone.wav`. The Qt device id is the PulseAudio source name.

## Linting & CI

```sh
cmake --build build --target clang-format                                   # format C++ in place (KDE style)
git ls-files '*.cpp' '*.h' | xargs clang-format --dry-run --Werror          # format check, as in CI
cmake --build build --target all_qmllint                                    # QML lint; fails on any warning
```

- `.github/workflows/ci.yml` runs on pushes and pull requests to `main`. It uses an `archlinux:latest` container, because GitHub's Ubuntu images lack Qt ≥ 6.7 and Plasma 6. Steps: configure with `-DCMAKE_COMPILE_WARNING_AS_ERROR=ON`, clang-format check, build, qmllint, then `ctest` with `QT_QPA_PLATFORM=offscreen`.
- To reproduce CI locally, run those steps in `docker run archlinux:latest` (copy the repo in, don't build in the mounted source tree).
- `.clang-format` is generated from ECM's KDE style by `kde_clang_format()` at configure time, and is gitignored. Configure before running the format check.
- qmllint settings: `.qmllint.ini` sets `MaxWarnings=0`, because the Qt-generated lint targets otherwise exit 0 on warnings. `.contextProperties.ini` declares Plasma's injected `i18n*()` functions, which qmllint can't see. Don't switch to the `KI18n` singleton to silence those warnings: it wouldn't use the applet's translation domain, since plasmashell shares one QML engine across applets.
- QML files use `pragma ComponentBehavior: Bound`. Delegates and inline components must reach outer objects through ids, and their own properties through their own id.

## Architecture

The data flow runs from the audio thread to the main thread and then to QML:

- **`src/core/`** (static library `tunercore`, no QML, unit tested in `autotests/`):
  - `pitchdetector`: McLeod Pitch Method (NSDF from an FFT autocorrelation, built-in radix-2 FFT, parabolic interpolation). The window is a power of two of about 160 ms, which is needed to catch G0 (≈24.5 Hz, the lowest string supported).
  - `notes`: frequency ↔ MIDI conversion and note names (sharps, scientific octave numbers).
  - `notes` also has `parse()`, which turns "F#1"/"Bb0" into MIDI numbers plus display labels ("F♯1"/"B♭0"); the user's spelling is preserved.
  - `instrumentconfig`: parses and validates the instruments JSON. Invalid entries are kept, with their `error` string, rather than dropped. `defaultJson()` reads the built-in file, which is compiled into `tunercore` as a Qt resource (`:/plasmatuner/defaultinstruments.json`).
- **`AudioCapture`** (`src/audiocapture.*`): lives on a dedicated `QThread` so the DSP never runs on the plasmashell GUI thread, and must only be called through queued invocations. It wraps a `QAudioSource` (Float format if supported), keeps one buffer per channel, and analyses the **loudest channel**, because audio interfaces often carry the instrument on a single channel. It applies a −60 dBFS noise gate and emits `analysed(frequency, clarity, levelDb)` about 25 times per second.
- **`Tuner`** (`src/tuner.*`, `QML_ELEMENT`): the main-thread facade. It lists input devices (index 0 = system default with id ""; a saved device that is unplugged falls back to the default without being forgotten). It also drops detections with clarity below 0.8, applies a 5-sample median filter, and holds the last note for 600 ms after the signal fades. It exposes the note, octave, cents and level.
- **`InstrumentCatalog`** (`src/instrumentcatalog.*`, `QML_SINGLETON`): owns the instruments file. It creates the file if it's missing, watches both the file and its directory (editors often save by replacing the file), reloads after a 200 ms debounce, and exposes `instruments` (nested maps with `valid`/`error`), `fileError` and `fileUrl`.
- **QML** (`src/qml/`): `main.qml` holds the `PlasmoidItem` and a custom compact icon (`tuner-symbolic.svg`, drawn as a mask). `FullRepresentation.qml` owns the `Tuner` instance and the controls, resolves the saved instrument/tuning names against the catalog, and shows the error area and the file-error banner. It also opens the file with `Qt.openUrlExternally`. `TunerGauge.qml` is the QtQuick.Shapes tachometer (±50 cents across ±60°).
- **Config schema**: `src/config/main.xml` is bundled as a resource. `plasma_add_applet` flattens resources, so it ends up at the QML module root, which is where libplasma looks for it.

Other build details:
- C++20 is required and set explicitly, because the ECM defaults are older.
- moc can't parse raw string literals (`R"(...)"`): a `#` inside one makes it silently skip the class, which shows up as an "undefined reference to vtable" link error. Test fixtures therefore use the `json()` helper in `autotests/instrumentconfigtest.cpp`, which takes single-quoted JSON.
- New `.qml`, `.cpp` and resource files must be listed in `src/CMakeLists.txt`.
- Resources are bundled flat next to the QML, so refer to them as `Qt.resolvedUrl("./file")`.

Licensing: the whole project is LGPL-2.1-or-later (`LICENSE` at the root, also declared in `src/metadata.json`). Source files carry no SPDX or copyright headers; don't add them.
