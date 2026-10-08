add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_io.github.amedeo03.plasmatuner\")

# Music theory, DSP and instruments file parsing, free of QML so it can be unit tested.
add_library(tunercore STATIC
    core/notes.cpp
    core/instrumentconfig.cpp
    core/pitchdetector.cpp
)
qt_add_resources(tunercore "tunercore_defaults"
    PREFIX /plasmatuner
    BASE core
    FILES core/defaultinstruments.json
)
set_target_properties(tunercore PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(tunercore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tunercore PUBLIC
    Qt6::Core
    KF6::I18n
)

plasma_add_applet(io.github.amedeo03.plasmatuner
    QML_SOURCES
        qml/main.qml
        qml/FullRepresentation.qml
        qml/TunerGauge.qml
    CPP_SOURCES
        audiocapture.cpp
        instrumentcatalog.cpp
        tuner.cpp
    RESOURCES
        qml/tuner-symbolic.svg
        config/main.xml
    GENERATE_APPLET_CLASS
)

target_link_libraries(io.github.amedeo03.plasmatuner PRIVATE
    tunercore
    Qt6::Gui
    Qt6::Multimedia
    Plasma::Plasma
    KF6::I18n
)
