import Qt.labs.qmlmodels 1.0
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

MyPage {
    TabBar {
        id: bar

        height: MyStyle.px(50)

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        TabButton {
            text: qsTr("GridView")
        }

        TabButton {
            text: qsTr("ListView")
        }

        TabButton {
            text: qsTr("PathView")
        }

        TabButton {
            text: qsTr("SwipeView")
        }

        TabButton {
            text: qsTr("TableView")
        }

    }

    ListModel {
        id: listModel

        ListElement {
            name: "Ivan Ivanov"
        }

        ListElement {
            name: "Dmitry Petrov"
        }

        ListElement {
            name: "Sergey Sidorov"
        }

        ListElement {
            name: "Alexey Smirnov"
        }

        ListElement {
            name: "Nikolay Vasiliev"
        }

        ListElement {
            name: "Pavel Morozov"
        }

        ListElement {
            name: "Alexander Popov"
        }

        ListElement {
            name: "Mikhail Kuznetsov"
        }

        ListElement {
            name: "Artem Volkov"
        }

        ListElement {
            name: "Vladimir Lebedev"
        }

        ListElement {
            name: "Konstantin Sokolov"
        }

        ListElement {
            name: "Boris Mikhailov"
        }

        ListElement {
            name: "Yuri Fedorov"
        }

        ListElement {
            name: "Anton Egorov"
        }

        ListElement {
            name: "Denis Kozlov"
        }

    }

    ListModel {
        id: smallListModel

        ListElement {
            name: "Ivan"
        }

        ListElement {
            name: "Dmitry"
        }

        ListElement {
            name: "Sergey"
        }

    }

    StackLayout {
        currentIndex: bar.currentIndex

        anchors {
            top: bar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        GridView {
            id: gridView

            anchors.fill: parent
            model: listModel
            cellWidth: MyStyle.px(150)
            cellHeight: MyStyle.px(150)

            delegate: Column {
                Image {
                    source: "pics/portrait.png"
                    anchors.horizontalCenter: parent.horizontalCenter
                    sourceSize.height: MyStyle.px(50)
                    sourceSize.width: MyStyle.px(50)
                }

                Text {
                    text: name
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: MyStyle.px(15)
                }

            }

        }

        ListView {
            anchors.fill: parent
            model: listModel
            clip: true

            delegate: Text {
                text: name
                height: MyStyle.px(50)
                font.pixelSize: MyStyle.px(20)
            }

        }

        Item {
            anchors.fill: parent

            Component {
                id: delegate

                Column {
                    id: wrapper

                    required property string name

                    opacity: PathView.isCurrentItem ? 1 : 0.5

                    Image {
                        anchors.horizontalCenter: nameText.horizontalCenter
                        width: 64
                        height: 64
                        source: "pics/portrait.png"
                    }

                    Text {
                        id: nameText

                        text: wrapper.name
                        font.pointSize: 16
                    }

                }

            }

            PathView {
                anchors.fill: parent
                model: smallListModel
                delegate: delegate

                path: Path {
                    startX: 120
                    startY: 100

                    PathQuad {
                        x: 120
                        y: 25
                        controlX: 260
                        controlY: 75
                    }

                    PathQuad {
                        x: 120
                        y: 100
                        controlX: -20
                        controlY: 75
                    }

                }

            }

        }

        Item {
            anchors.fill: parent

            SwipeView {
                id: swipeView

                currentIndex: 1
                anchors.fill: parent

                Text {
                    text: "FISRT PAGE"
                    font.pixelSize: MyStyle.px(50)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    text: "SECOND PAGE"
                    font.pixelSize: MyStyle.px(50)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    text: "THIRD PAGE"
                    font.pixelSize: MyStyle.px(50)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

            }

            PageIndicator {
                id: indicator

                count: swipeView.count
                currentIndex: swipeView.currentIndex
                anchors.bottom: swipeView.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }

        }

        TableView {
            anchors.fill: parent
            columnSpacing: 1
            rowSpacing: 1
            clip: true

            model: TableModel {
                rows: [{
                    "name": "cat",
                    "color": "black"
                }, {
                    "name": "dog",
                    "color": "brown"
                }, {
                    "name": "bird",
                    "color": "white"
                }]

                TableModelColumn {
                    display: "name"
                }

                TableModelColumn {
                    display: "color"
                }

            }

            delegate: Rectangle {
                implicitWidth: 100
                implicitHeight: 50
                border.width: 1

                Text {
                    text: display
                    anchors.centerIn: parent
                }

            }

        }

    }

}
