<!--
    SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
    SPDX-License-Identifier: CC0-1.0
-->

# Plasma Tuner

A chromatic tuner applet for KDE Plasma 6, for electric guitars (6, 7 and 8
strings) and basses (4, 5 and 6 strings) plugged into an audio interface.

Click the tuning-fork icon in the panel to open the tuner. It shows:

* the detected note with its octave, and how many cents sharp or flat it is,
  on a tachometer-style gauge;
* the target notes of the selected tuning (E standard, Drop D, D standard,
  Drop C, C standard);
* selectors for the input device, instrument, number of strings, tuning and the
  A4 reference pitch.

The microphone is only open while the tuner is visible. Your selections are
remembered.

## Build

Requires Qt ≥ 6.7 (including Qt Multimedia), KDE Frameworks 6 and libplasma
development packages.

```
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build
```

Try it without installing:

```
QT_PLUGIN_PATH=$PWD/build/bin plasmoidviewer -a io.github.amedeo03.plasmatuner
```

Install it and add it to a panel:

```
sudo cmake --install build
plasmashell --replace   # or log out and back in
```
