include(ECMAddTests)

ecm_add_tests(
    instrumentconfigtest.cpp
    notestest.cpp
    pitchdetectortest.cpp
    LINK_LIBRARIES tunercore Qt6::Test
)
