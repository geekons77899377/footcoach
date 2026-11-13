import QtQuick
import QtQuick.Controls

// Choix d'un stade dans la liste du club (schéma 11), depuis la fiche d'un match. La liste ne montre que les lieux
// disponibles ; on la charge et on la met à jour dans Paramètres > Stades des matchs. Recherche par commune, nom,
// terrain ou adresse, sans accents ni majuscules.
Popup {
    id: root

    property int clubId: -1
    property var allVenues: []
    property string query: ""
    property real navInset: AppState.bottomInset
    signal picked(var venue)

    readonly property var shown: {
        var q = fold(query.trim());
        if (q === "")
            return allVenues;
        var words = q.split(/\s+/), out = [];
        for (var i = 0; i < allVenues.length; ++i) {
            var v = allVenues[i];
            var hay = fold([v.city, v.postcode, v.name, v.field, v.address].join(" "));
            var ok = true;
            for (var w = 0; w < words.length && ok; ++w)
                ok = hay.indexOf(words[w]) >= 0;
            if (ok)
                out.push(v);
        }
        return out;
    }

    function fold(s) {
        return String(s || "").normalize("NFD").replace(/[̀-ͯ]/g, "").toLowerCase();
    }

    function openFor(club) {
        clubId = club;
        allVenues = club > 0 ? backend.venues(club, false) : [];
        query = "";
        search.text = "";
        open();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 12
    width: Math.min(parent ? parent.width - 24 : 360, 560)
    height: parent ? Math.min(parent.height - navInset - 48, 620) : 480
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? parent.height - height - 12 - navInset : 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Column {
        spacing: 10

        Item {
            width: parent.width
            height: 40
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "CHOISIR UN STADE"
                font.pixelSize: 12
                font.weight: Font.Bold
                font.letterSpacing: 1.2
                color: Theme.textMuted
            }
            IconButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                iconName: "close"
                accessibleName: "Fermer"
                onClicked: root.close()
            }
        }

        Rectangle {
            width: parent.width
            height: 48
            radius: Theme.radiusSm
            color: Theme.bg
            border.width: 1
            border.color: search.activeFocus ? Theme.accent : Theme.border
            Icon {
                id: searchIcon
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                name: "search"
                color: Theme.textMuted
                size: 20
            }
            IconButton {
                id: searchGo
                objectName: "venueSearchGo"
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.verticalCenter: parent.verticalCenter
                diameter: 42
                iconName: "search"
                iconColor: Theme.accent
                accessibleName: "Rechercher"
                onClicked: { root.query = search.text; search.focus = false; }
            }
            TextField {
                KeyboardRetry {}
                id: search
                objectName: "venueSearch"
                anchors.left: searchIcon.right
                anchors.right: searchGo.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 4
                color: Theme.text
                placeholderText: "Commune, stade, rue… puis Entrée"
                placeholderTextColor: Theme.textDim
                background: null
                font.pixelSize: 16
                inputMethodHints: Qt.ImhNoPredictiveText
                // Recherche lancée par Entrée ou la loupe, pas à chaque lettre : sur téléphone, refaire la liste pendant
                // la frappe fermait le clavier (constat sur appareil, 13/11/2026). Effacer le texte remet toute la liste.
                EnterKey.type: Qt.EnterKeySearch
                onAccepted: { root.query = text; focus = false; }
                onTextChanged: if (text.trim() === "") root.query = ""
            }
        }

        Text {
            objectName: "venueCount"
            width: parent.width
            leftPadding: 6
            text: root.allVenues.length === 0 ? ""
                  : root.shown.length + (root.shown.length > 1 ? " stades" : " stade")
            font.pixelSize: 12
            color: Theme.textMuted
        }

        Text {
            objectName: "venueEmpty"
            visible: root.shown.length === 0
            width: parent.width
            leftPadding: 6
            rightPadding: 6
            wrapMode: Text.Wrap
            text: root.allVenues.length === 0
                  ? "Aucun stade dans la liste du club. Chargez-la dans Paramètres > Stades des matchs, ou tapez l'adresse dans le champ."
                  : "Aucun stade ne correspond à « " + root.query.trim() + " »."
            font.pixelSize: 14
            color: Theme.textMuted
        }

        ListView {
            id: list
            objectName: "venueList"
            width: parent.width
            height: Math.max(0, root.availableHeight - y)
            clip: true
            model: root.shown
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index
                width: list.width
                height: 66
                radius: 14
                color: tap.pressed ? Theme.surfaceHi : "transparent"
                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Row {
                        width: parent.width
                        spacing: 8
                        Text {
                            width: Math.min(implicitWidth, parent.width - (homeTag.visible ? homeTag.width + 8 : 0))
                            elide: Text.ElideRight
                            text: row.modelData.city + " · " + row.modelData.name
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Rectangle {
                            id: homeTag
                            visible: row.modelData.isHome
                            width: homeText.implicitWidth + 14
                            height: 20
                            radius: 10
                            color: Theme.accentSoft
                            Text { id: homeText; anchors.centerIn: parent; text: "Stade du club"; font.pixelSize: 11; font.weight: Font.Bold; color: Theme.accent }
                        }
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        text: [row.modelData.field, row.modelData.address].filter(function (s) { return s && s !== ""; }).join(" · ")
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.borderSubtle }
                MouseArea {
                    id: tap
                    anchors.fill: parent
                    onClicked: {
                        root.picked(row.modelData);
                        root.close();
                    }
                }
            }
        }
    }
}
