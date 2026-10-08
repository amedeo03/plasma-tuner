#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

$XGETTEXT `find . -name \*.js -o -name \*.qml -o -name \*.cpp` -o $podir/plasma_applet_io.github.amedeo03.plasmatuner.pot
