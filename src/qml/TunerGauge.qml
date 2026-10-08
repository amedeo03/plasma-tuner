// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import QtQuick.Shapes
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC3

// Tachometer-style needle showing the deviation from the nearest note.
Item {
    id: gauge

    // -50 (flat) to +50 (sharp) cents
    property real cents: 0
    // Whether a note is currently detected; the needle rests at 0 otherwise.
    property bool active: false
    // Within this many cents the note counts as in tune.
    property real tolerance: 5

    readonly property bool inTune: active && Math.abs(cents) <= tolerance
    // Degrees from the vertical for ±50 cents.
    readonly property real sweep: 60
    readonly property real thickness: Kirigami.Units.smallSpacing * 1.5
    readonly property real centreX: width / 2
    readonly property real centreY: height - Kirigami.Units.largeSpacing
    // Largest radius fitting the ±sweep arc in the item.
    readonly property real radius: Math.max(0, Math.min((width / 2 - thickness) / Math.sin(sweep * Math.PI / 180), centreY - thickness))

    function angle(value) {
        return Math.max(-50, Math.min(50, value)) / 50 * sweep;
    }

    implicitWidth: Kirigami.Units.gridUnit * 16
    implicitHeight: Kirigami.Units.gridUnit * 8

    // Arc for a cents range; angles in Shapes start at 3 o'clock, clockwise.
    component Band: ShapePath {
        property real from
        property real to
        property real arcRadius: gauge.radius - gauge.thickness / 2

        fillColor: "transparent"
        strokeWidth: gauge.thickness
        capStyle: ShapePath.FlatCap

        PathAngleArc {
            centerX: gauge.centreX
            centerY: gauge.centreY
            radiusX: arcRadius
            radiusY: arcRadius
            startAngle: -90 + gauge.angle(from)
            sweepAngle: gauge.angle(to) - gauge.angle(from)
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        Band {
            from: -50
            to: 50
            strokeColor: Qt.alpha(Kirigami.Theme.textColor, 0.2)
        }
        Band {
            from: -15
            to: 15
            strokeColor: Qt.alpha(Kirigami.Theme.neutralTextColor, 0.6)
        }
        Band {
            from: -gauge.tolerance
            to: gauge.tolerance
            strokeColor: Kirigami.Theme.positiveTextColor
        }
    }

    // Ticks every 5 cents, longer every 10.
    Repeater {
        model: 21

        Item {
            required property int index
            readonly property bool major: index % 2 === 0

            x: gauge.centreX
            y: gauge.centreY
            rotation: gauge.angle(index * 5 - 50)

            Rectangle {
                x: -width / 2
                y: -gauge.radius + gauge.thickness * 1.5
                width: parent.major ? 2 : 1
                height: gauge.radius * (parent.major ? 0.1 : 0.05)
                color: Kirigami.Theme.textColor
                opacity: parent.major ? 0.8 : 0.5
            }
        }
    }

    Repeater {
        model: [-50, -25, 0, 25, 50]

        PC3.Label {
            required property int modelData
            readonly property real labelRadius: gauge.radius * 0.72
            readonly property real radians: gauge.angle(modelData) * Math.PI / 180

            x: gauge.centreX + labelRadius * Math.sin(radians) - width / 2
            y: gauge.centreY - labelRadius * Math.cos(radians) - height / 2
            text: modelData > 0 ? "+" + modelData : modelData
            font: Kirigami.Theme.smallFont
            opacity: 0.7
        }
    }

    PC3.Label {
        x: gauge.centreX - gauge.radius * 0.6 - width / 2
        y: gauge.centreY - height
        text: "♭"
        font.pixelSize: Kirigami.Units.gridUnit * 1.5
        color: gauge.active && gauge.cents < -gauge.tolerance ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.textColor
        opacity: gauge.active && gauge.cents < -gauge.tolerance ? 1 : 0.3
    }

    PC3.Label {
        x: gauge.centreX + gauge.radius * 0.6 - width / 2
        y: gauge.centreY - height
        text: "♯"
        font.pixelSize: Kirigami.Units.gridUnit * 1.5
        color: gauge.active && gauge.cents > gauge.tolerance ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.textColor
        opacity: gauge.active && gauge.cents > gauge.tolerance ? 1 : 0.3
    }

    // Needle
    Item {
        x: gauge.centreX
        y: gauge.centreY
        rotation: gauge.active ? gauge.angle(gauge.cents) : 0
        opacity: gauge.active ? 1 : 0.35

        Behavior on rotation {
            NumberAnimation {
                duration: Kirigami.Units.shortDuration
                easing.type: Easing.OutCubic
            }
        }
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
            }
        }

        Rectangle {
            x: -width / 2
            y: -height
            width: 3
            height: gauge.radius * 0.92
            radius: width / 2
            antialiasing: true
            color: gauge.inTune ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
        }
    }

    Rectangle {
        x: gauge.centreX - width / 2
        y: gauge.centreY - height / 2
        width: Kirigami.Units.largeSpacing * 1.5
        height: width
        radius: width / 2
        color: Kirigami.Theme.textColor
    }
}
