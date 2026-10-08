pragma ComponentBehavior: Bound

import QtQuick
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    toolTipMainText: i18nc("@info:tooltip", "Tuner")
    toolTipSubText: i18nc("@info:tooltip", "Tune your guitar or bass")

    compactRepresentation: MouseArea {
        id: compactRoot

        property bool wasExpanded: false

        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        onPressed: wasExpanded = root.expanded
        onClicked: root.expanded = !compactRoot.wasExpanded

        Kirigami.Icon {
            anchors.fill: parent
            source: Qt.resolvedUrl("tuner-symbolic.svg")
            isMask: true
            color: Kirigami.Theme.textColor
            active: compactRoot.containsMouse
        }
    }

    fullRepresentation: FullRepresentation {
        plasmoidItem: root
    }
}
