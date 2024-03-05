import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Rectangle {
        id: rect
        anchors.fill: parent
        color: "red"

        MyTextField {
            id: text
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: parent.height * 0.3
        }

        MyButton {
            id: b1

            anchors {
                left: parent.left
                right: parent.right
                bottom: b2.top
            }

            colorTest: "blue"
            height: parent.height * 0.1
            onButtonClicked: {
                text.text = "TEST TEXT 1";
            }
        }

        MyButton {
            id: b2
            colorTest: "black"
            anchors {
                left: parent.left
                right: parent.right
                bottom: b3.bottom
            }

            height: parent.height * 0.1
            onButtonClicked: {
                text.text = "TEST TEXT 2";
            }
        }

        MyButton {
            id: b3
            colorTest: "grey"
            anchors {
                left: parent.left
                right: parent.right
                bottom: b4.bottom
            }

            height: parent.height * 0.1
            onButtonClicked: {
                text.text = "TEST TEXT 2";
            }
        }

        MyButton {
            id: b4
            colorTest: "red"
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            height: parent.height * 0.1
            onButtonClicked: {
                text.text = "TEST TEXT 2";
            }
        }
    }
}
