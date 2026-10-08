# Plasma Tuner

A chromatic tuner applet for KDE Plasma 6, for electric guitars (6, 7 and 8
strings) and basses (4, 5 and 6 strings) plugged into an audio interface.

Click the tuning-fork icon in the panel to open the tuner. It shows:

* the detected note with its octave, and how many cents sharp or flat it is,
  on a tachometer-style gauge;
* the target notes of the selected tuning;
* selectors for the input device, instrument, tuning and the A4 reference
  pitch.

The microphone is only open while the tuner is visible. Your selections are
remembered.

## Instruments

Instruments and their tunings are read from
`~/.config/plasma-tuner/instruments.json`. The file is created on first use
with electric guitars (6, 7 and 8 strings) and basses (4, 5 and 6 strings), each
with E standard, Drop D, D standard, Drop C and C standard tunings. Click the
edit button next to the instrument selector to open it. Changes are picked up
as soon as you save.

```json
{
    "instruments": [
        {
            "name": "Electric guitar (6 strings)",
            "stringCount": 6,
            "tunings": [
                { "name": "E standard", "stringNotes": ["E2", "A2", "D3", "G3", "B3", "E4"] },
                { "name": "Open G", "stringNotes": ["D2", "G2", "D3", "G3", "B3", "D4"] }
            ]
        }
    ]
}
```

* `name`: required, and unique (instruments among instruments, tunings within
  their instrument).
* `stringCount`: a whole number from 1 to 12.
* `stringNotes`: one note per string, from the lowest string. A note is a letter
  A–G, an optional `#` (sharp) or `b` (flat), and an octave: `E2`, `F#1`, `Bb0`.
  Notes from C0 to C8 are accepted.

Invalid entries stay in the list, marked with a warning icon; selecting one
explains what is wrong. If the file itself can't be read (for example a JSON
syntax error), the tuner shows the error and uses the built-in instruments
until the file is fixed.

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

## License

Plasma Tuner is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 2.1 of the License, or (at your option) any
later version. See [LICENSE](LICENSE).
