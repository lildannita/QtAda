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
    None,
};

#define CHECK_GUI_CLASS(T)                                                                         \
    static_assert(std::is_same<T, QWidget>::value || std::is_same<T, QQuickItem>::value,           \
                  "Class must be QWidget or QQuickItem")
#define CHECK_GUI_ENUM(T)                                                                          \
    static_assert(std::is_same<T, WidgetClass>::value || std::is_same<T, QuickClass>::value,       \
                  "Enum must be WidgetClass or QuickClass")
} // namespace QtAda::core
