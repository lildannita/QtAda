import QtQuick 2.15
import QtQuick.Templates 2.15 as T

T.TabButton {
    id: tabButton

    required property string buttonText

    implicitHeight: parent.height
    padding: 0
    spacing: 0

    contentItem: Text {
        anchors.centerIn: parent
        text: tabButton.buttonText
        font.pixelSize: MyStyle.px(15)
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    background: Rectangle {
        anchors.fill: parent
        color: tabButton.checked ? MyStyle.greenColor : MyStyle.brownColor
        border.width: 3
        border.color: MyStyle.blackColor
    }

}
