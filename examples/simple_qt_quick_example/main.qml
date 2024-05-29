import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Templates 2.15 as T
import QtQuick.Window 2.15

ApplicationWindow {
    id: main

    width: Screen.width / 2.5
    height: Screen.height / 1.5
    visible: true
    title: qsTr("Hello World")

    StackLayout {
        id: content

        width: parent.width
        height: main.contentItem.height
        currentIndex: tabBar.currentIndex

        MyClickPage {
        }

        MyTextPage {
        }

        MyViewPage {
        }

    }

    menuBar: MenuBar {
        objectName: "menuBar"
        visible: tabBar.currentIndex == 0

        Menu {
            objectName: "simpleMenu"
            title: "Menu"

            Action {
                objectName: "simpleAction"
                text: "Action"
            }

            MenuSeparator {
            }

            Menu {
                objectName: "simpleSubMenu"
                title: "Submenu"

                Action {
                    objectName: "simpleSubAction"
                    text: "Submenu action"
                }

                Menu {
                    title: "Empty submenu"
                }

            }

        }

        Menu {
            title: "Empty submenu"
        }

    }

    footer: T.TabBar {
        id: tabBar

        readonly property real itemWidth: tabBar.width / 5 - tabBar.spacing * 4

        spacing: 1
        width: parent.width
        height: MyStyle.px(50)
        currentIndex: 0

        MyTabButton {
            objectName: "clickItemsTabButton"
            buttonText: "Click Items"
        }

        MyTabButton {
            objectName: "textItemsTabButton"
            buttonText: "Text Items"
        }

        MyTabButton {
            objectName: "viewItemsTabButton"
            buttonText: "Views Items"
        }

        contentItem: ListView {
            model: tabBar.contentModel
            currentIndex: tabBar.currentIndex
            spacing: tabBar.spacing
            orientation: ListView.Horizontal
            flickableDirection: Flickable.AutoFlickIfNeeded
            boundsBehavior: Flickable.StopAtBounds
        }

        background: Rectangle {
            color: MyStyle.greyColor
        }

    }

}
