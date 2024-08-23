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
    return QStringLiteral("%1button%2(%3);%4")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(clickType(event, isReleaseInside))
        .arg(path)
        .arg(buttonText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Button text: '%1'").arg(buttonText.simplified()));
}

QString mouseAreaEventCommand(const QString &path, const QEvent *event,
                              bool isReleaseInside) noexcept
{
    return QStringLiteral("%1mouseArea%2(%3);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(clickType(event, isReleaseInside))
        .arg(path);
}

QString checkButtonCommand(const QString &path, bool isChecked, bool isDoubleCheck,
                           const QString &buttonText) noexcept
{
    auto generate = [&path, &buttonText](bool isChecked) {
        return QStringLiteral("%1checkButton(%2, %3);%4")
            .arg(SCRIPT_COMMAND_PREFIX)
            .arg(path)
            .arg(isChecked ? "true" : "false")
            .arg(buttonText.isEmpty()
                     ? ""
                     : QStringLiteral(" // Button text: %1").arg(buttonText.simplified()));
    };

    if (isDoubleCheck) {
        return QStringLiteral("%1\n%2").arg(generate(!isChecked)).arg(generate(isChecked));
    }
    else {
        return generate(isChecked);
    }
}

QString selectItemCommand(const QString &path, const QString &statement) noexcept
{
    return QStringLiteral("%1selectItem(%2, %3);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(statement);
}

QString setDelayProgressCommand(const QString &path, double progress) noexcept
{
    return QStringLiteral("%1setDelayProgress(%2, %3);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(progress);
}

QString selectTabCommand(const QString &path, const QString &statement) noexcept
{
    return QStringLiteral("%1selectTabItem(%2, %3);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(statement);
}

QString treeViewCommand(const QString &path, bool isExpand, const QString &indexPath,
                        const QString &delegateText) noexcept
{
    return QStringLiteral("%1%2Delegate(%3, %4);%5")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(isExpand ? "expand" : "collapse")
        .arg(path)
        .arg(indexPath)
        .arg(delegateText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Delegate text: '%1'").arg(delegateText.simplified()));
}

QString undoCommand(const QString &path, int index, const QString &delegateText) noexcept
{
    return QStringLiteral("%1undoCommand(%2, %3);%4")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(index)
        .arg(delegateText.isEmpty()
                 ? ""
                 : QStringLiteral(" // Delegate text: '%1'").arg(delegateText.simplified()));
}

QString selectViewItemCommand(const QString &path, int index) noexcept
{
    return QStringLiteral("%1selectViewItem(%2, %3);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(index);
}

QString actionCommand(const QString &path, const QString &text, bool isSeparator, bool isMenu,
                      std::optional<bool> checked) noexcept
{
    return QStringLiteral("%1%2triggerAction(%3%4);%5")
        .arg(isSeparator ? "// Looks like useless separator click\n// "
                         : (isMenu ? "// Looks like useless menu click\n// " : ""))
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(checked.has_value()
                 // На момент нажатия check box еще не поменяет свое состояние
                 ? QStringLiteral(", %1").arg(*checked ? "false" : "true")
                 : "")
        .arg(text.isEmpty() ? "" : QStringLiteral(" // Action text: '%1'").arg(text.simplified()));
}

QString delegateClickCommand(const QString &path, const QString &statement, bool isDouble,
                             const QString &text) noexcept
{
    return QStringLiteral("%1delegate%2Click(%3, %4);%5")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(isDouble ? "Dbl" : "")
        .arg(path)
        .arg(statement)
        .arg(text.isEmpty() ? ""
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
    return QStringLiteral("%1setSelection(%2, [%3]);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(selection);
}

QString clearSelectionCommand(const QString &path) noexcept
{
    return QStringLiteral("%1clearSelection(%2);").arg(SCRIPT_COMMAND_PREFIX).arg(path);
}

QString setTextCommand(const QString &path, const QString &text, const QString &indexPath) noexcept
{
    return QStringLiteral("%1setText(%2%3, '%4');")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(path)
        .arg(indexPath.isEmpty() ? "" : QStringLiteral(", %1").arg(indexPath))
        .arg(text);
}

QString closeCommand(const QString &path, bool isDialog) noexcept
{
    return QStringLiteral("%1close%2(%3);")
        .arg(SCRIPT_COMMAND_PREFIX)
        .arg(isDialog ? "Dialog" : "Window")
        .arg(path);
}
} // namespace QtAda::core::filters
