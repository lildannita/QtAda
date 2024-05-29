#pragma once

#include <QWidget>
#include <QQuickItem>

namespace QtAda::core {
enum class WidgetClass {
    Button,
    RadioButton,
    CheckBox,
    Slider,
    ComboBox,
    SpinBox,
    Calendar,
    Menu,
    MenuBar,
    TabBar,
    TreeView,
    UndoView,
    ItemView,
    ColumnViewGrip,
    Dialog,
    Window,
    KeySequenceEdit,
    TextEdit,
    PlainTextEdit,
    LineEdit,
    None,
};

enum class QuickClass {
    Button,
    MouseArea,
    TabButton,
    RadioButton,
    CheckBox,
    Switch,
    DelayButton,
    Slider,
    RangeSlider,
    Dial,
    ScrollBar,
    SpinBox,
    ComboBox,
    ItemDelegate,
    Tumbler,
    MenuBarItem,
    MenuItem,
    ItemView,
    PathView,
    SwipeView,
    //! TODO: по идее обработка нажатия на этот компонент бесполезна, так
    //! как он отображает только текстовые значения, нажатия на которые ни
    //! к чему не приводят, но нужно проверить.
    //! TableView,
    TextInput,
    TextEdit,
    //! TODO: эти классы являются прямыми потомками TextInput и TextEdit,
    //! так что на этапе обработки событий может *скорее всего* они не нужны.
    //! TextField,
    //! TextArea,
    Window,
    // Нужно для QuickEventFilter::processKeyEvent
    ExtSpinBox,
    ExtComboBox,
    None,
};

#define CHECK_GUI_CLASS(T)                                                                         \
    static_assert(std::is_same_v<T, QWidget> || std::is_same_v<T, QQuickItem>,                     \
                  "Class must be QWidget or QQuickItem")
#define CHECK_GUI_ENUM(T)                                                                          \
    static_assert(std::is_same_v<T, WidgetClass> || std::is_same_v<T, QuickClass>,                 \
                  "Enum must be WidgetClass or QuickClass")
} // namespace QtAda::core
