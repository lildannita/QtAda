#include "CommonFilters.hpp"

namespace QtAda::core::filters {
static QString clickType(const QEvent *event, bool isReleaseInside) noexcept
{
    return isReleaseInside
               ? (event->type() == QEvent::MouseButtonDblClick ? QStringLiteral("DblClick")
                                                               : QStringLiteral("Click"))
               : QStringLiteral("Press");
}

QString buttonEventCommand(const QString &path, const QEvent *event, bool isReleaseInside,
                           const QString &buttonText) noexcept
{
    return QStringLiteral("button%1(%2);%3")
        .arg(clickType(event, isReleaseInside), path,
             buttonText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Button text: '%1'").arg(buttonText.simplified()));
}

QString mouseAreaEventCommand(const QString &path, const QEvent *event,
                              bool isReleaseInside) noexcept
{
    return QStringLiteral("mouseArea%1(%2);").arg(clickType(event, isReleaseInside), path);
}

QString checkButtonCommand(const QString &path, bool isChecked, bool isDoubleCheck,
                           const QString &buttonText) noexcept
{
    auto generate = [&path, &buttonText](bool isChecked) {
        return QStringLiteral("checkButton(%1, %2);%3")
            .arg(path, isChecked ? "true" : "false",
                 buttonText.isEmpty()
                     ? ""
                     : QStringLiteral(" // Button text: %1").arg(buttonText.simplified()));
    };

    if (isDoubleCheck) {
        return QStringLiteral("%1\n%2").arg(generate(!isChecked), generate(isChecked));
    }
    else {
        return generate(isChecked);
    }
}

QString selectItemCommand(const QString &path, const QString &statement) noexcept
{
    return QStringLiteral("selectItem(%1, %2);").arg(path, statement);
}

QString setDelayProgressCommand(const QString &path, double progress) noexcept
{
    return QStringLiteral("setDelayProgress(%1, %2);").arg(path).arg(progress);
}

QString selectTabCommand(const QString &path, const QString &statement) noexcept
{
    return QStringLiteral("selectTabItem(%1, %2);").arg(path, statement);
}

QString treeViewCommand(const QString &path, bool isExpand, const QString &indexPath,
                        const QString &delegateText) noexcept
{
    return QStringLiteral("%1Delegate(%2, %3);%4")
        .arg(isExpand ? "expand" : "collapse", path, indexPath,
             delegateText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Delegate text: '%1'").arg(delegateText.simplified()));
}

QString undoCommand(const QString &path, int index, const QString &delegateText) noexcept
{
    return QStringLiteral("undoCommand(%1, %2);%3")
        .arg(path)
        .arg(index)
        .arg(delegateText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Delegate text: '%1'").arg(delegateText.simplified()));
}

QString selectViewItemCommand(const QString &path, int index) noexcept
{
    return QStringLiteral("selectViewItem(%1, %2);").arg(path).arg(index);
}

QString actionCommand(const QString &path, const QString &text, bool isSeparator, bool isMenu,
                      std::optional<bool> checked) noexcept
{
    return QStringLiteral("%1triggerAction(%2%3);%4")
        .arg(isSeparator ? "// Looks like useless separator click\n// "
                         : (isMenu ? "// Looks like useless menu click\n// " : ""),
             path,
             // На момент нажатия check box еще не поменяет свое состояние
             checked.has_value() ? QStringLiteral(", %1").arg(*checked ? "false" : "true") : "",
             text.isEmpty() ? "" : QStringLiteral(" // Action text: '%1'").arg(text.simplified()));
}

QString delegateClickCommand(const QString &path, const QString &statement, bool isDouble,
                             const QString &text) noexcept
{
    return QStringLiteral("delegate%1Click(%2, %3);%4")
        .arg(isDouble ? "Dbl" : "", path, statement,
             text.isEmpty() ? ""
                            : QStringLiteral(" // Delegate text: '%1'").arg(text.simplified()));
}

QString setSelectionCommand(const QString &path, const QString &selection) noexcept
{
    //! TODO: При "обновлении" скрипта может все-равно получиться конфликт имен.
    //! В будущем нужно будет придумать как самим решать эту проблему. Сейчас можно
    //! было бы оставить это на пользователя, но в ущерб "краткости" кода и в пользу
    //! надежности было решено передавать selectionData сразу в виде аргумента.
    //! static uint selectionDataCount = 0;
    //! const auto selectionLet
    //!     = QStringLiteral("selectionData%1")
    //!           .arg(selectionDataCount > 0 ? QString::number(selectionDataCount) : "");
    //! selectionDataCount++;
    //! return QStringLiteral("let %1 = [%2];\n%3setSelection('%4', %1);")
    //!     .arg(selectionLet)
    //!     .arg(selection)
    //!     .arg(SCRIPT_COMMAND_PREFIX)
    //!     .arg(path);
    return QStringLiteral("setSelection(%1, [%2]);").arg(path, selection);
}

QString clearSelectionCommand(const QString &path) noexcept
{
    return QStringLiteral("clearSelection(%1);").arg(path);
}

QString setTextCommand(const QString &path, const QString &text, const QString &indexPath) noexcept
{
    return QStringLiteral("setText(%1%2, '%3');")
        .arg(path, indexPath.isEmpty() ? "" : QStringLiteral(", %1").arg(indexPath), text);
}

QString closeCommand(const QString &path, bool isDialog) noexcept
{
    return QStringLiteral("close%1(%2);").arg(isDialog ? "Dialog" : "Window", path);
}
} // namespace QtAda::core::filters
