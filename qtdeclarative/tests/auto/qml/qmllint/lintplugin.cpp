// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lintplugin.h"

using namespace Qt::StringLiterals;

class ElementTest : public QQmlSA::ElementPass
{
public:
    ElementTest(QQmlSA::PassManager *manager) : QQmlSA::ElementPass(manager)
    {
        m_rectangle = resolveType(u"QtQuick", u"Rectangle");
    }

    bool shouldRun(const QQmlSA::Element &element) override
    {
        return element->baseType() == m_rectangle;
    }

    void run(const QQmlSA::Element &element) override
    {
        auto property = element->property(u"radius"_s);
        if (!property.isValid() || element->property(u"radius"_s).typeName() != u"double") {
            emitWarning(u"Failed to verify radius property", element->sourceLocation());
            return;
        }

        auto bindings = element->propertyBindings(u"radius"_s);
        if (bindings.isEmpty() || bindings.constFirst().numberValue() != 5) {
            emitWarning(u"Failed to verify radius property binding", element->sourceLocation());
            return;
        }

        emitWarning(u"ElementTest OK", element->sourceLocation());
    }

private:
    QQmlSA::Element m_rectangle;
};

class PropertyTest : public QQmlSA::PropertyPass
{
public:
    PropertyTest(QQmlSA::PassManager *manager) : QQmlSA::PropertyPass(manager) { }

    void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                   const QQmlJSMetaPropertyBinding &binding, const QQmlSA::Element &bindingScope,
                   const QQmlSA::Element &value) override
    {
        emitWarning(u"Saw binding on %1 property %2 with value %3 (and type %4) in scope %5"_s
                            .arg(element->baseTypeName(), propertyName,
                                 value.isNull()
                                         ? u"NULL"_s
                                         : (value->internalName().isNull() ? value->baseTypeName()
                                                                           : value->baseTypeName()))
                            .arg(binding.bindingType())
                            .arg(bindingScope->baseTypeName()),
                    bindingScope->sourceLocation());
    }

    void onRead(const QQmlSA::Element &element, const QString &propertyName,
                const QQmlSA::Element &readScope, QQmlJS::SourceLocation location) override
    {
        emitWarning(u"Saw read on %1 property %2 in scope %3"_s.arg(
                            element->baseTypeName(), propertyName, readScope->baseTypeName()),
                    location);
    }

    void onWrite(const QQmlSA::Element &element, const QString &propertyName,
                 const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                 QQmlJS::SourceLocation location) override
    {
        emitWarning(u"Saw write on %1 property %2 with value %3 in scope %4"_s.arg(
                            element->baseTypeName(), propertyName,
                            (value->internalName().isNull() ? value->baseTypeName()
                                                            : value->internalName()),
                            writeScope->baseTypeName()),
                    location);
    }
};

class HasImportedModuleTest : public QQmlSA::ElementPass
{
public:
    HasImportedModuleTest(QQmlSA::PassManager *manager, QString message)
        : QQmlSA::ElementPass(manager), m_message(message)
    {
    }

    bool shouldRun(const QQmlSA::Element &element) override
    {
        Q_UNUSED(element)
        return true;
    }

    void run(const QQmlSA::Element &element) override
    {
        Q_UNUSED(element)
        emitWarning(m_message);
    }

private:
    QString m_message;
};

void LintPlugin::registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement)
{
    if (!rootElement->filePath().endsWith(u"_pluginTest.qml"))
        return;

    manager->registerElementPass(std::make_unique<ElementTest>(manager));
    manager->registerPropertyPass(std::make_unique<PropertyTest>(manager), "QtQuick", "Text",
                                  "text");
    manager->registerPropertyPass(std::make_unique<PropertyTest>(manager), "", "", "x");
    manager->registerPropertyPass(std::make_unique<PropertyTest>(manager), "QtQuick", "ListView");
    if (manager->hasImportedModule("QtQuick.Controls")) {
        if (manager->hasImportedModule("QtQuick")) {
            if (manager->hasImportedModule("QtQuick.Window")) {
                manager->registerElementPass(std::make_unique<HasImportedModuleTest>(
                        manager, "QtQuick.Controls, QtQuick and QtQuick.Window present"));
            }
        } else {
            manager->registerElementPass(std::make_unique<HasImportedModuleTest>(
                    manager, "QtQuick.Controls and NO QtQuick present"));
        }
    }
}
