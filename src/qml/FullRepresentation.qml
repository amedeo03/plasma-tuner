// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

import plasma.applet.io.github.amedeo03.plasmatuner

ColumnLayout {
    id: full

    required property PlasmoidItem plasmoidItem

    // The configuration may hold values that are invalid for the current
    // instrument (e.g. 4 strings after switching to guitar): sanitize them.
    readonly property int instrument: Plasmoid.configuration.instrument === 1 ? 1 : 0
    readonly property var stringCounts: TuningCatalog.stringCounts(instrument)
    readonly property int stringCount: stringCounts.includes(Plasmoid.configuration.stringCount) ? Plasmoid.configuration.stringCount : stringCounts[0]
    readonly property var tunings: TuningCatalog.tunings(instrument, stringCount)
    readonly property int tuningIndex: Math.max(0, Math.min(tunings.length - 1, Plasmoid.configuration.tuning))
    readonly property var tuningNotes: tunings.length > 0 ? tunings[tuningIndex].notes : []

    // Only listen while the tuner is on screen: an open popup, or the full
    // representation shown inline on the desktop.
    readonly property bool shown: plasmoidItem.expanded || (visible && Window.window !== null && Window.window.visible && !(plasmoidItem.compactRepresentationItem?.visible ?? false))

    Layout.minimumWidth: Kirigami.Units.gridUnit * 18
    Layout.preferredWidth: Kirigami.Units.gridUnit * 22
    Layout.minimumHeight: implicitHeight
    spacing: Kirigami.Units.largeSpacing

    Tuner {
        id: tuner

        active: full.shown
        deviceId: Plasmoid.configuration.inputDevice
        referencePitch: Plasmoid.configuration.referencePitch
    }

    TunerGauge {
        Layout.fillWidth: true
        Layout.preferredHeight: Kirigami.Units.gridUnit * 8
        cents: tuner.cents
        active: tuner.hasSignal
    }

    // Detected note
    ColumnLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: 0
        opacity: tuner.hasSignal ? 1 : 0.4

        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.smallSpacing

            PC3.Label {
                text: tuner.midiNote >= 0 ? tuner.noteName : "–"
                font.pixelSize: Kirigami.Units.gridUnit * 3
                font.weight: Font.Bold
                color: tuner.hasSignal && Math.abs(tuner.cents) <= 5 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
            }
            PC3.Label {
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: Kirigami.Units.smallSpacing * 2
                visible: tuner.midiNote >= 0
                text: tuner.octave
                font.pixelSize: Kirigami.Units.gridUnit * 1.5
            }
        }

        PC3.Label {
            Layout.alignment: Qt.AlignHCenter
            text: {
                if (tuner.errorString.length > 0) {
                    return tuner.errorString;
                }
                if (tuner.midiNote < 0) {
                    return i18nc("@info", "Play a string");
                }
                const cents = Math.round(tuner.cents);
                return i18nc("@info deviation in cents, then frequency", "%1 cents · %2 Hz", (cents > 0 ? "+" : "") + cents, tuner.frequency.toFixed(2));
            }
            color: tuner.errorString.length > 0 ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
            wrapMode: Text.Wrap
        }
    }

    // Target notes of the selected tuning, lowest string first.
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: full.tuningNotes

            Rectangle {
                id: stringNote

                required property var modelData
                readonly property bool current: tuner.hasSignal && tuner.midiNote === modelData.midi
                readonly property bool inTune: current && Math.abs(tuner.cents) <= 5

                implicitWidth: Math.max(implicitHeight, noteLabel.implicitWidth + Kirigami.Units.largeSpacing * 2)
                implicitHeight: noteLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
                radius: Kirigami.Units.cornerRadius
                color: inTune ? Kirigami.Theme.positiveBackgroundColor : current ? Kirigami.Theme.highlightColor : "transparent"
                border.width: 1
                border.color: inTune ? Kirigami.Theme.positiveTextColor : Qt.alpha(Kirigami.Theme.textColor, 0.3)

                PC3.Label {
                    id: noteLabel

                    anchors.centerIn: parent
                    text: stringNote.modelData.name + stringNote.modelData.octave
                    color: stringNote.current && !stringNote.inTune ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                }
            }
        }
    }

    Kirigami.Separator {
        Layout.fillWidth: true
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        PC3.Label {
            Layout.alignment: Qt.AlignRight
            text: i18nc("@label:listbox", "Input:")
        }
        PC3.ComboBox {
            Layout.fillWidth: true
            model: tuner.devices
            textRole: "name"
            valueRole: "id"
            currentIndex: tuner.deviceIndex
            onActivated: Plasmoid.configuration.inputDevice = currentValue
        }

        Item {
            Layout.preferredWidth: 1
        }
        PC3.ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 1
            value: tuner.level
            PC3.ToolTip.text: i18nc("@info:tooltip", "Input level")
            PC3.ToolTip.visible: hovered
        }

        PC3.Label {
            Layout.alignment: Qt.AlignRight
            text: i18nc("@label:listbox", "Instrument:")
        }
        RowLayout {
            Layout.fillWidth: true

            PC3.ComboBox {
                Layout.fillWidth: true
                model: TuningCatalog.instruments()
                textRole: "text"
                valueRole: "value"
                currentIndex: full.instrument
                onActivated: Plasmoid.configuration.instrument = currentValue
            }
            PC3.ComboBox {
                model: full.stringCounts.map(count => ({
                    value: count,
                    text: i18ncp("@item:inlistbox", "%1 string", "%1 strings", count)
                }))
                textRole: "text"
                valueRole: "value"
                currentIndex: full.stringCounts.indexOf(full.stringCount)
                onActivated: Plasmoid.configuration.stringCount = currentValue
            }
        }

        PC3.Label {
            Layout.alignment: Qt.AlignRight
            text: i18nc("@label:listbox", "Tuning:")
        }
        PC3.ComboBox {
            Layout.fillWidth: true
            model: full.tunings
            textRole: "text"
            currentIndex: full.tuningIndex
            onActivated: Plasmoid.configuration.tuning = currentIndex
        }

        PC3.Label {
            Layout.alignment: Qt.AlignRight
            text: i18nc("@label:spinbox", "Reference A4:")
        }
        RowLayout {
            Layout.fillWidth: true

            // Value in tenths of Hz, as SpinBox only handles integers.
            PC3.SpinBox {
                id: referenceSpinBox

                Layout.fillWidth: true
                from: 3800
                to: 5000
                stepSize: 1
                editable: true
                value: Math.round(Plasmoid.configuration.referencePitch * 10)
                textFromValue: (value, locale) => i18nc("@item:valuesuffix frequency", "%1 Hz", Number(value / 10).toLocaleString(locale, 'f', 1))
                valueFromText: (text, locale) => Math.round(Number.fromLocaleString(locale, text.replace(/[^0-9.,]/g, "")) * 10)
                validator: RegularExpressionValidator {
                    regularExpression: /\d{1,3}([.,]\d?)?\s*(Hz)?/
                }
                onValueModified: Plasmoid.configuration.referencePitch = value / 10
            }
            PC3.ToolButton {
                icon.name: "edit-undo"
                enabled: referenceSpinBox.value !== 4400
                onClicked: Plasmoid.configuration.referencePitch = 440
                PC3.ToolTip.text: i18nc("@info:tooltip", "Reset to 440 Hz")
                PC3.ToolTip.visible: hovered
                Accessible.name: PC3.ToolTip.text
            }
        }
    }
}
