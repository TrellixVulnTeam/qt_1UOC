// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qquickwidget_plugin.h"

#include <QtDesigner/default_extensionfactory.h>
#include <QtDesigner/qextensionmanager.h>

#include <QtCore/qplugin.h>
#include <QtCore/qdebug.h>
#include <QtQuickWidgets/qquickwidget.h>

#include <QtQuick/QQuickWindow>


QT_BEGIN_NAMESPACE

QQuickWidgetPlugin::QQuickWidgetPlugin(QObject *parent)
    : QObject(parent)
{
}

QString QQuickWidgetPlugin::name() const
{
    return QStringLiteral("QQuickWidget");
}

QString QQuickWidgetPlugin::group() const
{
    return QStringLiteral("Display Widgets");
}

QString QQuickWidgetPlugin::toolTip() const
{
    return QStringLiteral("A widget for displaying a Qt Quick 2 user interface.");
}

QString QQuickWidgetPlugin::whatsThis() const
{
    return toolTip();
}

QString QQuickWidgetPlugin::includeFile() const
{
    return QStringLiteral("<QtQuickWidgets/QQuickWidget>");
}

QIcon QQuickWidgetPlugin::icon() const
{
    return QIcon(QStringLiteral(":/qt-project.org/qquickwidget/images/qquickwidget.png"));
}

bool QQuickWidgetPlugin::isContainer() const
{
    return false;
}

QWidget *QQuickWidgetPlugin::createWidget(QWidget *parent)
{
    const auto graphicsApi = QQuickWindow::graphicsApi();
    if (graphicsApi != QSGRendererInterface::OpenGL) {
        qWarning("Qt Designer: The QQuickWidget custom widget plugin is disabled because it requires OpenGL RHI (current: %d).",
             int(graphicsApi));
        return {};
    }

    QQuickWidget *result = new QQuickWidget(parent);
    connect(result, &QQuickWidget::sceneGraphError,
            this, &QQuickWidgetPlugin::sceneGraphError);
    return result;
}

bool QQuickWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

void QQuickWidgetPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (m_initialized)
        return;

    m_initialized = true;
}

QString QQuickWidgetPlugin::domXml() const
{
    if (QQuickWindow::graphicsApi() != QSGRendererInterface::OpenGL)
        return {};

    return QStringLiteral(R"(
<ui language="c++">
    <widget class="QQuickWidget" name="quickWidget">
        <property name="resizeMode">
            <enum>QQuickWidget::SizeRootObjectToView</enum>
        </property>
        <property name="geometry">
            <rect>
                <x>0</x>
                <y>0</y>
                <width>300</width>
                <height>200</height>
            </rect>
        </property>
    </widget>
</ui>
)");
}

void QQuickWidgetPlugin::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message)
{
    qWarning() << Q_FUNC_INFO << ':' << message;
}

QT_END_NAMESPACE
