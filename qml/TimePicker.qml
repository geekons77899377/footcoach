import QtQuick
import QtQuick.Controls

// Sélecteur d'heure par roues (heures, minutes), au style de DatePicker.
Popup {
    id: root

    property string title: "Heure"
    property var pending: null
    signal picked(string time)         // "HH:mm"

    function openAt(time) {
        var h = 18, m = 30;
        if (/^\d{2}:\d{2}$/.test(time || "")) {
            h = parseInt(time.substring(0, 2), 10);
            m = parseInt(time.substring(3, 5), 10);
        }
        pending = { h: h, m: m };
        open();
    }

    function pad(n) { return (n < 10 ? "0" : "") + n; }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 16
    width: Math.min(parent ? parent.width - 32 : 320, 320)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? Math.max(12, (parent.height - height) / 3) : 0
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    onOpened: {
        if (!pending)
            return;
        hourT.currentIndex = pending.h;
        minT.currentIndex = pending.m;
        hourT.positionViewAtIndex(pending.h, Tumbler.Center);
        minT.positionViewAtIndex(pending.m, Tumbler.Center);
    }

    contentItem: Column {
        spacing: 12
        Text {
            width: parent.width
            text: root.title
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.text
        }
        Item {
            width: parent.width
            height: 200
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 40
                radius: 12
                color: Theme.accentSoft
            }
            Row {
                anchors.fill: parent
                Tumbler {
                    id: hourT
                    width: parent.width / 2
                    height: 200
                    visibleItemCount: 5
                    model: 24
                    delegate: Text {
                        required property int index
                        text: root.pad(index)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 22
                        font.weight: Tumbler.displacement === 0 ? Font.Bold : Font.Normal
                        color: Tumbler.displacement === 0 ? Theme.text : Theme.textMuted
                        opacity: 1.0 - Math.min(1.0, Math.abs(Tumbler.displacement) / 2.5)
                    }
                }
                Tumbler {
                    id: minT
                    width: parent.width / 2
                    height: 200
                    visibleItemCount: 5
                    model: 60
                    delegate: Text {
                        required property int index
                        text: root.pad(index)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 22
                        font.weight: Tumbler.displacement === 0 ? Font.Bold : Font.Normal
                        color: Tumbler.displacement === 0 ? Theme.text : Theme.textMuted
                        opacity: 1.0 - Math.min(1.0, Math.abs(Tumbler.displacement) / 2.5)
                    }
                }
            }
        }
        Row {
            width: parent.width
            spacing: 10
            layoutDirection: Qt.RightToLeft
            AppButton {
                kind: "primary"
                text: "Valider"
                onClicked: {
                    root.close();
                    root.picked(root.pad(hourT.currentIndex) + ":" + root.pad(minT.currentIndex));
                }
            }
            AppButton { kind: "ghost"; text: "Annuler"; onClicked: root.close() }
        }
    }
}
