import QtQuick
import "SupportLink.js" as Link

// Page « Paramètres » : assistance (ticket GitHub « SUP - … », e-mail), informations sur l'application, accès aux données.
Item {
    id: page

    property string mode: "browse"            // browse, support, privacy
    property bool dryRun: false               // tests : ne pas ouvrir le navigateur
    readonly property bool wide: width >= 760
    signal navigate(string section)

    function goBack() {
        if (mode === "browse")
            return false;
        mode = "browse";
        return true;
    }

    function openSupport(cat) {
        form.openNew(cat);
        mode = "support";
    }

    function openContact() {
        if (!dryRun && !Qt.openUrlExternally(Link.contactUrl()))
            toast.show("Impossible d'ouvrir l'application de messagerie sur cet appareil.");
    }

    // La politique de confidentialité se lit dans l'application (aucun navigateur à ouvrir).
    function openPrivacy() {
        mode = "privacy";
    }

    onVisibleChanged: if (!visible) mode = "browse"

    component SettingRow: Item {
        id: row
        property string icon: ""
        property string title: ""
        property string subtitle: ""
        property string trailing: ""          // texte à droite (sinon chevron)
        property bool chevron: true
        property int subtitleLines: 1         // > 1 : le sous-titre passe à la ligne (texte long sans espaces, ex. SHA-1)
        signal activated()
        width: parent.width
        height: 64 + (subtitleLines - 1) * 16
        Rectangle {
            id: ic
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 44
            height: 44
            radius: 14
            color: Theme.surfaceHi
            border.width: 1
            border.color: Theme.borderSubtle
            Icon { anchors.centerIn: parent; name: row.icon; color: Theme.accent; size: 22 }
        }
        Column {
            anchors.left: ic.right
            anchors.leftMargin: 14
            anchors.right: tail.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                width: parent.width
                elide: Text.ElideRight
                text: row.title
                font.pixelSize: 16
                font.weight: Font.Bold
                color: Theme.text
            }
            Text {
                visible: row.subtitle !== ""
                width: parent.width
                elide: Text.ElideRight
                wrapMode: row.subtitleLines > 1 ? Text.WrapAnywhere : Text.NoWrap
                maximumLineCount: row.subtitleLines
                text: row.subtitle
                font.pixelSize: 12
                color: Theme.textMuted
            }
        }
        Item {
            id: tail
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: row.trailing !== "" ? trailText.implicitWidth : (row.chevron ? 20 : 0)
            height: 24
            Text {
                id: trailText
                visible: row.trailing !== ""
                anchors.verticalCenter: parent.verticalCenter
                text: row.trailing
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.textMuted
            }
            Icon {
                visible: row.trailing === "" && row.chevron
                anchors.verticalCenter: parent.verticalCenter
                name: "chevron"
                color: Theme.textMuted
                size: 20
            }
        }
        MouseArea {
            anchors.fill: parent
            enabled: row.chevron || row.trailing === ""
            onClicked: row.activated()
        }
    }

    Item {
        id: browse
        anchors.fill: parent
        visible: page.mode === "browse"

        PageHeader {
            id: header
            width: parent.width
            title: "Paramètres"
            subtitle: "Assistance et informations"
        }

        Flickable {
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            contentWidth: width
            contentHeight: col.height + 40
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: col
                x: Theme.gutter + 4
                y: 16
                width: Math.min(parent.width - 2 * (Theme.gutter + 4), 720)
                spacing: 18

                Text {
                    text: "ASSISTANCE"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: 1.4
                    color: Theme.text
                }
                Card {
                    objectName: "supportCard"
                    width: parent.width
                    height: supportCol.height + 2 * padding
                    padding: 14
                    radius: Theme.radiusLg
                    Column {
                        id: supportCol
                        width: parent.width
                        SettingRow {
                            objectName: "rowBug"
                            icon: "flag"; title: "Signaler un problème"
                            subtitle: "Un message à l'équipe FootCoach"
                            onActivated: page.openSupport("bug")
                        }
                        Rectangle { width: parent.width; height: 1; color: Theme.borderSubtle }
                        SettingRow {
                            objectName: "rowIdea"
                            icon: "bulb"; title: "Proposer une amélioration"
                            subtitle: "Une idée pour faire évoluer FootCoach"
                            onActivated: page.openSupport("enhancement")
                        }
                        Rectangle { width: parent.width; height: 1; color: Theme.borderSubtle }
                        SettingRow {
                            objectName: "rowContact"
                            icon: "mail"; title: "Contacter le support"
                            subtitle: Link.CONTACT
                            onActivated: page.openContact()
                        }
                    }
                }

                Text {
                    text: "INFORMATIONS"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: 1.4
                    color: Theme.text
                }
                Card {
                    width: parent.width
                    height: infoCol.height + 2 * padding
                    padding: 14
                    radius: Theme.radiusLg
                    Column {
                        id: infoCol
                        width: parent.width
                        SettingRow {
                            objectName: "rowVersion"
                            icon: "info"; title: "Version"; chevron: false
                            trailing: "FootCoach " + Qt.application.version
                        }
                        Rectangle { width: parent.width; height: 1; color: Theme.borderSubtle }
                        SettingRow {
                            objectName: "rowRevision"
                            icon: "info"; title: "Révision (SHA-1)"; chevron: false
                            subtitleLines: buildInfo.dirty ? 3 : 2
                            subtitle: buildInfo.sha1 + (buildInfo.dirty ? " (modifications non commitées)" : "")
                        }
                        Rectangle { width: parent.width; height: 1; color: Theme.borderSubtle }
                        SettingRow {
                            objectName: "rowPrivacy"
                            icon: "shield"; title: "Politique de confidentialité"
                            subtitle: "Vos données restent sur cet appareil"
                            onActivated: page.openPrivacy()
                        }
                        Rectangle { width: parent.width; height: 1; color: Theme.borderSubtle }
                        SettingRow {
                            objectName: "rowData"
                            icon: "copy"; title: "Données"
                            subtitle: "Exporter ou importer un fichier FootCoach"
                            onActivated: page.navigate("data")
                        }
                    }
                }
            }
        }
    }

    PrivacyPage {
        id: privacy
        objectName: "privacyPage"
        anchors.fill: parent
        visible: page.mode === "privacy"
        showBack: true
        onCloseRequested: page.mode = "browse"
    }

    SupportForm {
        id: form
        objectName: "supportForm"
        anchors.fill: parent
        visible: page.mode === "support"
        dryRun: page.dryRun
        onCloseRequested: page.mode = "browse"
        onOpened: (kind) => {
            page.mode = "browse";
            toast.show(kind === "mail" ? "Votre messagerie s'ouvre : relisez puis envoyez le message."
                                       : "Signalement envoyé, merci !");
        }
    }

    Toast { id: toast }
}
