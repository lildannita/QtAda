import QtQml 2.15
import QtQuick 2.15
pragma Singleton

QtObject {
    id: valStyle

    property real screenPixelDensity: 1
    readonly property color yellowColor: "#EFD09E"
    readonly property color greenColor: "#D2D8B3"
    readonly property color blueColor: "#90A9B7"
    readonly property color brownColor: "#D4AA7D"
    readonly property color blackColor: "#272727"
    readonly property color greyColor: "#627C85"

    // Функция пересчета миллиметров в пиксели, с учетом PDI
    function px(mm) {
        return valStyle.screenPixelDensity * mm;
    }

}
