import QtQuick
import QtQuick.Controls

// Sélecteur de date par roues (jour, mois, année), adapté au tactile : l'année de départ est choisie par l'appelant
// (ex. année de naissance médiane d'une catégorie), ce qui évite de faire défiler des dizaines d'années.
Popup {
    id: root

    property int startYear: new Date().getFullYear() - 12
    readonly property int thisYear: new Date().getFullYear()
    property int maxYear: new Date().getFullYear()   // année la plus récente proposée
    property int yearCount: 70                         // nombre d'années proposées, en remontant
    readonly property var monthNames: ["janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août",
                                       "septembre", "octobre", "novembre", "décembre"]
    // Roues : jour 1..n, mois 0..11, année thisYear - index.
    readonly property int year: maxYear - yearT.currentIndex
    readonly property int month: monthT.currentIndex + 1
    readonly property int dayCount: new Date(year, month, 0).getDate()

    property string title: "Date"
    signal picked(string iso)

    // iso : "AAAA-MM-JJ" déjà saisi ("" : on part de l'année conseillée, au 1er janvier).
    function openAt(iso, defaultYear) {
        var y = defaultYear > 0 ? defaultYear : startYear, m = 1, d = 1;
        if (iso && iso.length === 10) {
            y = parseInt(iso.substring(0, 4), 10);
            m = parseInt(iso.substring(5, 7), 10);
            d = parseInt(iso.substring(8, 10), 10);
        }
        pending = { y: Math.max(maxYear - yearCount + 1, Math.min(maxYear, y)), m: m, d: d };
        open();
    }

    // Les roues ne prennent leur position qu'une fois affichées.
    property var pending: null
    onOpened: {
        if (!pending)
            return;
        yearT.currentIndex = maxYear - pending.y;
        monthT.currentIndex = pending.m - 1;
        dayT.currentIndex = Math.min(pending.d, new Date(pending.y, pending.m, 0).getDate()) - 1;
        yearT.positionViewAtIndex(yearT.currentIndex, Tumbler.Center);
        monthT.positionViewAtIndex(monthT.currentIndex, Tumbler.Center);
        dayT.positionViewAtIndex(dayT.currentIndex, Tumbler.Center);
    }

    function pad(n) { return (n < 10 ? "0" : "") + n; }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 16
    width: Math.min(parent ? parent.width - 32 : 360, 400)
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

    // Roue au style de l'application.
    component Wheel: Tumbler {
        id: wheel
        property var labels: null            // tableau de textes (sinon : numéro)
        visibleItemCount: 5
        height: 200
        delegate: Text {
            required property int index
            required property var modelData
            text: wheel.labels ? wheel.labels[index] : modelData
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 20
            font.weight: Tumbler.displacement === 0 ? Font.Bold : Font.Normal
            color: Tumbler.displacement === 0 ? Theme.text : Theme.textMuted
            opacity: 1.0 - Math.min(1.0, Math.abs(Tumbler.displacement) / (wheel.visibleItemCount / 2))
        }
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

            Rectangle {      // bande de la valeur choisie
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 40
                radius: 12
                color: Theme.accentSoft
            }
            Row {
                anchors.fill: parent
                Wheel {
                    id: dayT
                    width: parent.width * 0.22
                    model: root.dayCount
                    delegate: Text {
                        required property int index
                        text: index + 1
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 20
                        font.weight: Tumbler.displacement === 0 ? Font.Bold : Font.Normal
                        color: Tumbler.displacement === 0 ? Theme.text : Theme.textMuted
                        opacity: 1.0 - Math.min(1.0, Math.abs(Tumbler.displacement) / 2.5)
                    }
                }
                Wheel {
                    id: monthT
                    width: parent.width * 0.44
                    model: 12
                    labels: root.monthNames
                }
                Wheel {
                    id: yearT
                    width: parent.width * 0.34
                    model: root.yearCount
                    delegate: Text {
                        required property int index
                        text: root.maxYear - index
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 20
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
                    var d = Math.min(dayT.currentIndex + 1, root.dayCount);
                    root.close();
                    root.picked(root.year + "-" + root.pad(root.month) + "-" + root.pad(d));
                }
            }
            AppButton { kind: "ghost"; text: "Annuler"; onClicked: root.close() }
        }
    }
}
