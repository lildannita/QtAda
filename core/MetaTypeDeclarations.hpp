#pragma once

#include <QMetaMethod>
#include <QMetaObject>

Q_DECLARE_METATYPE(Qt::ConnectionType)
Q_DECLARE_METATYPE(QMetaMethod::Access)
Q_DECLARE_METATYPE(QMetaMethod::MethodType)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_DECLARE_METATYPE(const QMetaObject *)
#endif

//! TODO: декларировать по мере необходимости
// #include <QTimeZone>
// Q_DECLARE_METATYPE(QTimeZone)

// #ifdef QT_GUI_LIB
// #include <QMargins>
// #include <QPainterPath>
// #include <QMatrix4x4>
// #include <QPainter>
// #include <QPaintEngine>
// #include <QValidator>

// #include <QGuiApplication>
// #include <QSurfaceFormat>

// Q_DECLARE_METATYPE(Qt::FillRule)
// Q_DECLARE_METATYPE(Qt::InputMethodHints)
// Q_DECLARE_METATYPE(Qt::MouseButtons)
// Q_DECLARE_METATYPE(Qt::TransformationMode)
// Q_DECLARE_METATYPE(QPainterPath)
// Q_DECLARE_METATYPE(QPolygonF)
// Q_DECLARE_METATYPE(QMargins)
// Q_DECLARE_METATYPE(Qt::WindowType)
// Q_DECLARE_METATYPE(Qt::WindowState)
// Q_DECLARE_METATYPE(const QMatrix4x4 *)
// Q_DECLARE_METATYPE(const QValidator *)

// Q_DECLARE_METATYPE(QPainter::CompositionMode)
// Q_DECLARE_METATYPE(QPaintEngine::PolygonDrawMode)

// Q_DECLARE_METATYPE(QSurfaceFormat)
// #endif
