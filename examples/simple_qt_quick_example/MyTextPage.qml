import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

MyPage {
    id: page

    readonly property real prefferedHeight: MyStyle.px(30)
    readonly property real prefferedWidth: (page.width - layout.spacing * 2) / 3
    readonly property real prefferedWidthForPair: (page.width - layout.spacing) / 2

    ColumnLayout {
        spacing: 10

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        RowLayout {
            id: layout

            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight

            Rectangle {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                color: MyStyle.greenColor

                TextInput {
                    objectName: "simpleTextInput"
                    anchors.fill: parent
                    text: "Original TextInput"
                    font.pixelSize: MyStyle.px(20)
                }

            }

            Rectangle {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                color: MyStyle.greenColor

                MyTextInput {
                    objectName: "customTextInput"
                    anchors.fill: parent
                    text: "Custom TextInput"
                    font.pixelSize: MyStyle.px(20)
                }

            }

            Rectangle {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                color: MyStyle.greenColor

                TextField {
                    objectName: "simpleTextField"
                    anchors.fill: parent
                    text: "Orignal TextField"
                    font.pixelSize: MyStyle.px(15)
                }

            }

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight * 3

            Rectangle {
                Layout.preferredWidth: page.prefferedWidthForPair
                Layout.fillHeight: true
                color: MyStyle.greenColor

                TextEdit {
                    objectName: "simpleTextEdit"
                    anchors.fill: parent
                    text: "Orignal TextEdit"
                    font.pixelSize: MyStyle.px(20)
                }

            }

            Rectangle {
                Layout.preferredWidth: page.prefferedWidthForPair
                Layout.fillHeight: true
                color: MyStyle.greenColor

                TextArea {
                    objectName: "simpleTextArea"
                    anchors.fill: parent
                    text: "Orignal TextArea"
                    font.pixelSize: MyStyle.px(20)
                }

            }

        }

    }

}
