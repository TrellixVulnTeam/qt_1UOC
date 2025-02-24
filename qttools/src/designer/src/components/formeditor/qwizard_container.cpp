// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qwizard_container.h"

#include <QtDesigner/qextensionmanager.h>
#include <QtDesigner/abstractformeditor.h>

#include <QtWidgets/qwizard.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

using WizardPageList = QList<QWizardPage *>;

namespace qdesigner_internal {

QWizardContainer::QWizardContainer(QWizard *widget, QObject *parent) :
    QObject(parent),
    m_wizard(widget)
{
}

int QWizardContainer::count() const
{
    return m_wizard->pageIds().size();
}

QWidget *QWizardContainer::widget(int index) const
{
    QWidget *rc = nullptr;
    if (index >= 0) {
        const auto idList = m_wizard->pageIds();
        if (index < idList.size())
            rc = m_wizard->page(idList.at(index));
    }
    return rc;
}

int QWizardContainer::currentIndex() const
{
    return m_wizard->pageIds().indexOf(m_wizard->currentId());
}

void QWizardContainer::setCurrentIndex(int index)
{
    if (index < 0 || m_wizard->pageIds().isEmpty())
        return;

    int currentIdx = currentIndex();

    if (currentIdx == -1) {
        m_wizard->restart();
        currentIdx = currentIndex();
    }

    if (currentIdx == index)
        return;

    const int d = qAbs(index - currentIdx);
    if (index > currentIdx) {
        for (int i = 0; i < d; i++)
            m_wizard->next();
    } else {
        for (int i = 0; i < d; i++)
            m_wizard->back();
    }
}

static const char *msgWrongType = "** WARNING Attempt to add oject that is not of class WizardPage to a QWizard";

void QWizardContainer::addWidget(QWidget *widget)
{
    QWizardPage *page = qobject_cast<QWizardPage *>(widget);
    if (!page) {
        qWarning("%s", msgWrongType);
        return;
    }
    m_wizard->addPage(page);
    // Might be -1 after adding the first page
    setCurrentIndex(m_wizard->pageIds().size() - 1);
}

void QWizardContainer::insertWidget(int index, QWidget *widget)
{
    enum { delta = 5 };

    QWizardPage *newPage = qobject_cast<QWizardPage *>(widget);
    if (!newPage) {
        qWarning("%s", msgWrongType);
        return;
    }

    const auto idList = m_wizard->pageIds();
    const int pageCount = idList.size();
    if (index >= pageCount) {
        addWidget(widget);
        return;
    }

    // Insert before, reshuffle ids if required
    const int idBefore = idList.at(index);
    const int newId = idBefore - 1;
    const bool needsShuffle =
        (index == 0 && newId < 0)                        // At start: QWizard refuses to insert id -1
        || (index > 0 && idList.at(index - 1) == newId); // In-between
    if (needsShuffle) {
        // Create a gap by shuffling pages
        WizardPageList pageList;
        pageList.push_back(newPage);
        for (int i = index; i < pageCount; i++) {
            pageList.push_back(m_wizard->page(idList.at(i)));
            m_wizard->removePage(idList.at(i));
        }
        int newId = idBefore + delta;
        for (QWizardPage *page : qAsConst(pageList)) {
            m_wizard->setPage(newId, page);
            newId += delta;
        }
    } else {
        // Gap found, just insert
        m_wizard->setPage(newId, newPage);
    }
    // Might be at -1 after adding the first page
    setCurrentIndex(index);
}

void QWizardContainer::remove(int index)
{
    if (index < 0)
        return;

    const auto idList = m_wizard->pageIds();
    if (index >= idList.size())
        return;

    m_wizard->removePage(idList.at(index));
    // goto next page, preferably
    const int newSize = idList.size() - 1;
    if (index < newSize) {
        setCurrentIndex(index);
    } else {
        if (newSize > 0)
            setCurrentIndex(newSize - 1);
    }
}

// ---------------- QWizardPagePropertySheet
const char *QWizardPagePropertySheet::pageIdProperty = "pageId";

QWizardPagePropertySheet::QWizardPagePropertySheet(QWizardPage *object, QObject *parent) :
    QDesignerPropertySheet(object, parent),
    m_pageIdIndex(createFakeProperty(QLatin1String(pageIdProperty), QString()))
{
    setAttribute(m_pageIdIndex, true);
}

bool QWizardPagePropertySheet::reset(int index)
{
    if (index == m_pageIdIndex) {
        setProperty(index, QString());
        return true;
    }
    return QDesignerPropertySheet::reset(index);
}

// ---------------- QWizardPropertySheet
QWizardPropertySheet::QWizardPropertySheet(QWizard *object, QObject *parent) :
    QDesignerPropertySheet(object, parent),
    m_startId(QStringLiteral("startId"))
{
}

bool QWizardPropertySheet::isVisible(int index) const
{
    if (propertyName(index) == m_startId)
        return false;
    return QDesignerPropertySheet::isVisible(index);
}
}

QT_END_NAMESPACE
