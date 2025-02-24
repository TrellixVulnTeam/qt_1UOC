// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLPROPERTYNODE_H
#define QMLPROPERTYNODE_H

#include "aggregate.h"
#include "node.h"

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QmlPropertyNode : public Node
{
public:
    QmlPropertyNode(Aggregate *parent, const QString &name, QString type, bool attached);

    void setDataType(const QString &dataType) override { m_type = dataType; }
    void setStored(bool stored) { m_stored = toFlagValue(stored); }
    void setDefaultValue(const QString &value) { m_defaultValue = value; }
    void setDesignable(bool designable) { m_designable = toFlagValue(designable); }
    void setRequired() { m_required = toFlagValue(true); }

    [[nodiscard]] const QString &dataType() const { return m_type; }
    [[nodiscard]] const QString &defaultValue() const { return m_defaultValue; }
    [[nodiscard]] bool isReadOnlySet() const { return (readOnly_ != FlagValueDefault); }
    [[nodiscard]] bool isStored() const { return fromFlagValue(m_stored, true); }
    bool isWritable();
    bool isRequired();
    [[nodiscard]] bool isDefault() const override { return m_isDefault; }
    [[nodiscard]] bool isReadOnly() const override { return fromFlagValue(readOnly_, false); }
    [[nodiscard]] bool isAlias() const override { return m_isAlias; }
    [[nodiscard]] bool isAttached() const override { return m_attached; }
    [[nodiscard]] bool isQtQuickNode() const override { return parent()->isQtQuickNode(); }
    [[nodiscard]] QString qmlTypeName() const override { return parent()->qmlTypeName(); }
    [[nodiscard]] QString logicalModuleName() const override
    {
        return parent()->logicalModuleName();
    }
    [[nodiscard]] QString logicalModuleVersion() const override
    {
        return parent()->logicalModuleVersion();
    }
    [[nodiscard]] QString logicalModuleIdentifier() const override
    {
        return parent()->logicalModuleIdentifier();
    }
    [[nodiscard]] QString element() const override { return parent()->name(); }

    void markDefault() override { m_isDefault = true; }
    void markReadOnly(bool flag) override { readOnly_ = toFlagValue(flag); }

private:
    PropertyNode *findCorrespondingCppProperty();

private:
    QString m_type {};
    QString m_defaultValue {};
    FlagValue m_stored { FlagValueDefault };
    FlagValue m_designable { FlagValueDefault };
    bool m_isAlias { false };
    bool m_isDefault { false };
    bool m_attached {};
    FlagValue readOnly_ { FlagValueDefault };
    FlagValue m_required { FlagValueDefault };
};

QT_END_NAMESPACE

#endif // QMLPROPERTYNODE_H
