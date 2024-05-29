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

        objectName: "simpleDialog"
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
                objectName: "customButton"
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
                objectName: "checkableButton"
                text: "Simple Button"
                font.pixelSize: MyStyle.px(15)
                checkable: true

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
                objectName: "firstCheckBox"
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "First CheckBox"
            }

            CheckBox {
                objectName: "secondCheckBox"
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "Second CheckBox"
            }

            CheckBox {
                objectName: "disabledCheckBox"
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
                objectName: "firstRadioButton"
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "First RadioButton"
            }

            RadioButton {
                objectName: "secondRadioButton"
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "Second RadioButton"
            }

            RadioButton {
                objectName: "disabledRadioButton"
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
                objectName: "simpleSwitch"
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
                text: "Switch"
            }

            Switch {
                objectName: "disabledSwitch"
                Layout.preferredWidth: page.prefferedWidth
                enabled: false
                Layout.fillHeight: true
                text: "Disabled Switch"
            }

            DelayButton {
                objectName: "delayButton"
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
                objectName: "simpleSlider"
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
            }

            RangeSlider {
                objectName: "rangeSlider"
                Layout.preferredWidth: page.prefferedWidth
                Layout.fillHeight: true
            }

            Dial {
                objectName: "simpleDial"
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
                objectName: "editableComboBox"
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
                objectName: "simpleSpinBox"
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
                objectName: "simpleScrollBar"
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
