import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.15

MyPage {
    id: page

    readonly property real prefferedHeight: MyStyle.px(30)
    readonly property real prefferedWidth: (page.width - layout.spacing * 2) / 3

    Dialog {
        id: dialog

        title: "Enter Something"
        standardButtons: Dialog.Close
    }

    ColumnLayout {
        spacing: 10

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight

            MyButton {
                onButtonClicked: {
                    dialog.open();
                }

                anchors {
                    left: parent.left
                    right: parent.horizontalCenter
                    top: parent.top
                    bottom: parent.bottom
                    rightMargin: MyStyle.px(5)
                }

            }

            Button {
                text: "Simple Button"
                font.pixelSize: MyStyle.px(15)

                anchors {
                    left: parent.horizontalCenter
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    leftMargin: MyStyle.px(5)
                }

                background: Rectangle {
                    color: MyStyle.blueColor
                    border.color: MyStyle.blackColor
                    border.width: MyStyle.px(5)
                }

            }

        }

        RowLayout {
            id: layout

            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight

            CheckBox {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "First CheckBox"
            }

            CheckBox {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "Second CheckBox"
            }

            CheckBox {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                enabled: false
                text: "Disabled CheckBox"
            }

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight

            RadioButton {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "First RadioButton"
            }

            RadioButton {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "Second RadioButton"
            }

            RadioButton {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                enabled: false
                text: "Disabled RadioButton"
            }

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight

            Switch {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "Switch"
            }

            Switch {
                Layout.preferredWidth: page.prefferedWidth
                enabled: false
                Layout.fillHeight: true
                text: "Disabled Switch"
            }

            DelayButton {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                delay: 3000
                text: "DelayButton"
            }

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight * 3

            Slider {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
            }

            RangeSlider {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
            }

            Dial {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
            }

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight

            ComboBox {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true

                model: ListModel {
                    ListElement {
                        text: "First"
                    }

                    ListElement {
                        text: "Second"
                    }

                    ListElement {
                        text: "Third"
                    }

                }

            }

            ComboBox {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                editable: true

                model: ListModel {
                    ListElement {
                        text: "Banana"
                    }

                    ListElement {
                        text: "Apple"
                    }

                    ListElement {
                        text: "Coconut"
                    }

                }

            }

            SpinBox {
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                editable: true
            }

        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: tumbler.height

            MyTumbler {
                id: tumbler

                anchors.centerIn: parent
            }

        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: page.prefferedHeight

            ScrollBar {
                hoverEnabled: true
                active: true
                policy: ScrollBar.AlwaysOn
                orientation: Qt.Horizontal
                size: 0.5

                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }

            }

        }

    }

}
