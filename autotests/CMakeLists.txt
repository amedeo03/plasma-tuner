# SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

include(ECMAddTests)

ecm_add_tests(
    notestest.cpp
    pitchdetectortest.cpp
    tuningstest.cpp
    LINK_LIBRARIES tunercore Qt6::Test
)
