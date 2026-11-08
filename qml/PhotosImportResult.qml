import QtQuick
import QtQuick.Controls

// Compte-rendu de l'import de photos en lot (voir Backend::importPhotos) : compteurs, puis les fichiers à corriger.
Popup {
    id: root

    property var result: ({ added: 0, replaced: 0, failed: 0, items: [] })
    readonly property var problems: {
        var out = [];
        var items = result.items || [];
        for (var i = 0; i < items.length; ++i)
            if (items[i].status === "unknown" || items[i].status === "error")
                out.push(items[i]);
        return out;
    }

    function show(r) {
        result = r;
        open();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 20
    width: Math.min(parent ? parent.width - 32 : 360, 460)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? Math.min(parent.height * 0.06, Math.max(12, parent.height - height - 12)) : 0
    height: Math.min(implicitHeight, parent ? parent.height - 24 : implicitHeight)
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Flickable {
        id: scroller
        implicitHeight: col.implicitHeight
        contentWidth: width
        contentHeight: col.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: col
            width: scroller.width
            spacing: 14
            Text {
                width: parent.width
                text: "Photos importées"
                font.pixelSize: 20
                font.weight: Font.Bold
                color: Theme.text
            }
            Text {
                objectName: "photosSummary"
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: 15
                color: Theme.text
                text: {
                    var parts = [];
                    if (root.result.added > 0)
                        parts.push(root.result.added + (root.result.added > 1 ? " photos ajoutées" : " photo ajoutée"));
                    if (root.result.replaced > 0)
                        parts.push(root.result.replaced + (root.result.replaced > 1 ? " photos remplacées" : " photo remplacée"));
                    if (root.result.failed > 0)
                        parts.push(root.result.failed + (root.result.failed > 1 ? " fichiers ignorés" : " fichier ignoré"));
                    return parts.length ? parts.join(", ") + "." : "Aucune photo importée.";
                }
            }
            Text {
                visible: root.problems.length > 0
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: Theme.textMuted
                text: "Le fichier doit s'appeler comme le numéro de licence du joueur (par exemple 9603907336.jpg)."
            }
            Column {
                visible: root.problems.length > 0
                width: parent.width
                spacing: 4
                Repeater {
                    model: Math.min(root.problems.length, 10)
                    Text {
                        required property int index
                        objectName: "photosProblem"
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 13
                        color: Theme.danger
                        text: root.problems[index].file + " : " + root.problems[index].message
                    }
                }
                Text {
                    visible: root.problems.length > 10
                    width: parent.width
                    font.pixelSize: 13
                    color: Theme.textMuted
                    text: "… et " + (root.problems.length - 10) + " autre(s) fichier(s)."
                }
            }
            Row {
                width: parent.width
                layoutDirection: Qt.RightToLeft
                AppButton { objectName: "photosClose"; kind: "primary"; text: "Fermer"; onClicked: root.close() }
            }
        }
    }
}
