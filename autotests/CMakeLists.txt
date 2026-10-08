include(ECMAddTests)

ecm_add_tests(
    notestest.cpp
    pitchdetectortest.cpp
    tuningstest.cpp
    LINK_LIBRARIES tunercore Qt6::Test
)
