import QtQuick 2.15

Rectangle {
    id: button

    required property color colorTest

    signal buttonClicked()

    radius: 10
    color: button.colorTest

    Text {
        anchors.centerIn: parent
        color: "white"
        text: "test button"
        font.pixelSize: 15
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            button.buttonClicked();
        }
    }
}
