// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef METATYPESJSONPROCESSOR_H
#define METATYPESJSONPROCESSOR_H

#include "qmltypesclassdescription.h"

#include <QtCore/qstring.h>
#include <QtCore/qvector.h>
#include <QtCore/qjsonobject.h>

class MetaTypesJsonProcessor
{
public:
    MetaTypesJsonProcessor(bool privateIncludes) : m_privateIncludes(privateIncludes) {}

    bool processTypes(const QStringList &files);
    bool processForeignTypes(const QStringList &foreignTypesFiles);

    void postProcessTypes();
    void postProcessForeignTypes();

    QVector<QJsonObject> types() const { return m_types; }
    QVector<QJsonObject> foreignTypes() const { return m_foreignTypes; }
    QStringList referencedTypes() const { return m_referencedTypes; }
    QStringList includes() const { return m_includes; }

    QString extractRegisteredTypes() const;

private:
    enum RegistrationMode {
        NoRegistration,
        ObjectRegistration,
        GadgetRegistration,
        NamespaceRegistration
    };

    static RegistrationMode qmlTypeRegistrationMode(const QJsonObject &classDef);
    void addRelatedTypes();

    void sortTypes(QVector<QJsonObject> &types);
    QString resolvedInclude(const QString &include);;
    void processTypes(const QJsonObject &types);
    void processForeignTypes(const QJsonObject &types);

    QStringList m_includes;
    QStringList m_referencedTypes;
    QVector<QJsonObject> m_types;
    QVector<QJsonObject> m_foreignTypes;
    bool m_privateIncludes = false;
};

#endif // METATYPESJSONPROCESSOR_H
