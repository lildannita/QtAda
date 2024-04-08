import QtQuick 2.15

Rectangle {
    id: button

    signal buttonClicked()

    color: MyStyle.blueColor
    border.width: MyStyle.px(5)
    border.color: MyStyle.blackColor

    Text {
        anchors.centerIn: parent
        color: MyStyle.blackColor
        text: "Custom Button With MouseArea (Open Dialog)"
        font.pixelSize: MyStyle.px(15)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            button.buttonClicked();
        }
    }

}
