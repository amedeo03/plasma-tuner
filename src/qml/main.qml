// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    toolTipMainText: i18nc("@info:tooltip", "Tuner")
    toolTipSubText: i18nc("@info:tooltip", "Tune your guitar or bass")

    compactRepresentation: MouseArea {
        property bool wasExpanded: false

        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        onPressed: wasExpanded = root.expanded
        onClicked: root.expanded = !wasExpanded

        Kirigami.Icon {
            anchors.fill: parent
            source: Qt.resolvedUrl("tuner-symbolic.svg")
            isMask: true
            color: Kirigami.Theme.textColor
            active: parent.containsMouse
        }
    }

    fullRepresentation: FullRepresentation {
        plasmoidItem: root
    }
}
