pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

import plasma.applet.io.github.amedeo03.plasmatuner

ColumnLayout {
    id: full

    required property PlasmoidItem plasmoidItem

    // Selections are saved by name. A name missing from the instruments file
    // (renamed or deleted entry) falls back to the first valid entry.
    readonly property var instruments: InstrumentCatalog.instruments
    readonly property int instrumentIndex: indexOfName(instruments, Plasmoid.configuration.instrument)
    readonly property var instrument: instruments[instrumentIndex] ?? null
    readonly property var tunings: instrument?.valid ? instrument.tunings : []
    readonly property int tuningIndex: indexOfName(tunings, Plasmoid.configuration.tuning)
    readonly property var tuning: tunings[tuningIndex] ?? null
    readonly property var tuningNotes: tuning?.valid ? tuning.notes : []
    // Problem with the selected instrument or tuning, shown in place of the target notes.
    readonly property string selectionError: {
        if (instrument && !instrument.valid) {
            return instrument.error;
        }
        if (tuning && !tuning.valid) {
            return tuning.error;
        }
        return "";
    }

    // Only listen while the tuner is on screen: an open popup, or the full
    // representation shown inline on the desktop.
    readonly property bool shown: plasmoidItem.expanded || (visible && Window.window !== null && Window.window.visible && !(plasmoidItem.compactRepresentationItem?.visible ?? false))

    function indexOfName(entries, name) {
        const index = entries.findIndex(entry => entry.name === name);
        if (index >= 0) {
            return index;
        }
        return Math.max(0, entries.findIndex(entry => entry.valid));
    }

    function editInstruments() {
        Qt.openUrlExternally(InstrumentCatalog.fileUrl);
    }

    // Instrument or tuning list with a warning icon on invalid entries.
    component EntryComboBox: PC3.ComboBox {
        id: comboBox

        textRole: "name"
        delegate: PC3.ItemDelegate {
            required property var modelData
            required property int index

            width: ListView.view.width
            text: modelData.name
            icon.name: modelData.valid ? "" : "dialog-warning"
            highlighted: comboBox.highlightedIndex === index
        }
    }

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

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: InstrumentCatalog.fileError.length > 0
        type: Kirigami.MessageType.Error
        text: InstrumentCatalog.fileError
        actions: Kirigami.Action {
            text: i18nc("@action:button", "Edit Instruments…")
            icon.name: "document-edit"
            onTriggered: full.editInstruments()
        }
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

    RowLayout {
        Layout.fillWidth: true
        visible: full.selectionError.length > 0
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignTop
            implicitWidth: Kirigami.Units.iconSizes.smallMedium
            implicitHeight: Kirigami.Units.iconSizes.smallMedium
            source: "dialog-warning"
        }
        PC3.Label {
            Layout.fillWidth: true
            text: i18nc("@info %1 is the problem", "Configuration error: %1", full.selectionError)
            color: Kirigami.Theme.negativeTextColor
            wrapMode: Text.Wrap
        }
    }

    // Target notes of the selected tuning, lowest string first.
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        visible: full.selectionError.length === 0
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: full.tuningNotes

            Rectangle {
                id: stringNote

                required property var modelData
                readonly property bool current: tuner.hasSignal && tuner.midiNote === stringNote.modelData.midi
                readonly property bool inTune: stringNote.current && Math.abs(tuner.cents) <= 5

                implicitWidth: Math.max(implicitHeight, noteLabel.implicitWidth + Kirigami.Units.largeSpacing * 2)
                implicitHeight: noteLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
                radius: Kirigami.Units.cornerRadius
                color: inTune ? Kirigami.Theme.positiveBackgroundColor : current ? Kirigami.Theme.highlightColor : "transparent"
                border.width: 1
                border.color: inTune ? Kirigami.Theme.positiveTextColor : Qt.alpha(Kirigami.Theme.textColor, 0.3)

                PC3.Label {
                    id: noteLabel

                    anchors.centerIn: parent
                    text: stringNote.modelData.label
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

            EntryComboBox {
                Layout.fillWidth: true
                model: full.instruments
                currentIndex: full.instrumentIndex
                onActivated: index => Plasmoid.configuration.instrument = full.instruments[index].name
            }
            PC3.ToolButton {
                icon.name: "document-edit"
                onClicked: full.editInstruments()
                PC3.ToolTip.text: i18nc("@info:tooltip", "Edit instruments…")
                PC3.ToolTip.visible: hovered
                Accessible.name: PC3.ToolTip.text
            }
        }

        PC3.Label {
            Layout.alignment: Qt.AlignRight
            text: i18nc("@label:listbox", "Tuning:")
        }
        EntryComboBox {
            Layout.fillWidth: true
            enabled: full.tunings.length > 0
            model: full.tunings
            currentIndex: full.tuningIndex
            onActivated: index => Plasmoid.configuration.tuning = full.tunings[index].name
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
