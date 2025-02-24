// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "newactiondialog_p.h"
#include "ui_newactiondialog.h"
#include "richtexteditor_p.h"
#include "actioneditor_p.h"
#include "formwindowbase_p.h"
#include "qdesigner_utils_p.h"
#include "iconloader_p.h"

#include <QtDesigner/abstractformwindow.h>
#include <QtDesigner/abstractformeditor.h>

#include <QtWidgets/qpushbutton.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {
// Returns a combination of ChangeMask flags
unsigned ActionData::compare(const ActionData &rhs) const
{
    unsigned rc = 0;
    if (text != rhs.text)
        rc |= TextChanged;
    if (name != rhs.name)
        rc |= NameChanged;
    if (toolTip != rhs.toolTip)
        rc |= ToolTipChanged ;
    if (icon != rhs.icon)
        rc |= IconChanged ;
    if (checkable != rhs.checkable)
        rc |= CheckableChanged;
    if (keysequence != rhs.keysequence)
        rc |= KeysequenceChanged ;
    return rc;
}

// -------------------- NewActionDialog
NewActionDialog::NewActionDialog(ActionEditor *parent) :
    QDialog(parent, Qt::Sheet),
    m_ui(new Ui::NewActionDialog),
    m_actionEditor(parent),
    m_autoUpdateObjectName(true)
{
    m_ui->setupUi(this);

    m_ui->tooltipEditor->setTextPropertyValidationMode(ValidationRichText);
    connect(m_ui->toolTipToolButton, &QAbstractButton::clicked, this, &NewActionDialog::slotEditToolTip);

    m_ui->keysequenceResetToolButton->setIcon(createIconSet(QStringLiteral("resetproperty.png")));
    connect(m_ui->keysequenceResetToolButton, &QAbstractButton::clicked,
            this, &NewActionDialog::slotResetKeySequence);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    focusText();
    updateButtons();

    QDesignerFormWindowInterface *form = parent->formWindow();
    m_ui->iconSelector->setFormEditor(form->core());
    FormWindowBase *formBase = qobject_cast<FormWindowBase *>(form);

    if (formBase) {
        m_ui->iconSelector->setPixmapCache(formBase->pixmapCache());
        m_ui->iconSelector->setIconCache(formBase->iconCache());
    }
}

NewActionDialog::~NewActionDialog()
{
    delete m_ui;
}

void NewActionDialog::focusName()
{
    m_ui->editObjectName->setFocus();
}

void NewActionDialog::focusText()
{
    m_ui->editActionText->setFocus();
}

void NewActionDialog::focusTooltip()
{
    m_ui->tooltipEditor->setFocus();
}

void NewActionDialog::focusShortcut()
{
    m_ui->keySequenceEdit->setFocus();
}

void NewActionDialog::focusCheckable()
{
    m_ui->checkableCheckBox->setFocus();
}

QString NewActionDialog::actionText() const
{
    return m_ui->editActionText->text();
}

QString NewActionDialog::actionName() const
{
    return m_ui->editObjectName->text();
}

ActionData NewActionDialog::actionData() const
{
    ActionData rc;
    rc.text = actionText();
    rc.name = actionName();
    rc.toolTip = m_ui->tooltipEditor->text();
    rc.icon = m_ui->iconSelector->icon();
    rc.icon.setTheme(m_ui->iconThemeEditor->theme());
    rc.checkable = m_ui->checkableCheckBox->checkState() == Qt::Checked;
    rc.keysequence = PropertySheetKeySequenceValue(m_ui->keySequenceEdit->keySequence());
    return rc;
}

void NewActionDialog::setActionData(const ActionData &d)
{
    m_ui->editActionText->setText(d.text);
    m_ui->editObjectName->setText(d.name);
    m_ui->iconSelector->setIcon(d.icon.unthemed());
    m_ui->iconThemeEditor->setTheme(d.icon.theme());
    m_ui->tooltipEditor->setText(d.toolTip);
    m_ui->keySequenceEdit->setKeySequence(d.keysequence.value());
    m_ui->checkableCheckBox->setCheckState(d.checkable ? Qt::Checked : Qt::Unchecked);

    // Suppress updating of the object name from the text for existing actions.
    m_autoUpdateObjectName = d.name.isEmpty();
    updateButtons();
}

void NewActionDialog::on_editActionText_textEdited(const QString &text)
{
    if (m_autoUpdateObjectName)
        m_ui->editObjectName->setText(ActionEditor::actionTextToName(text));

    updateButtons();
}

void NewActionDialog::on_editObjectName_textEdited(const QString&)
{
    updateButtons();
    m_autoUpdateObjectName = false;
}

void NewActionDialog::slotEditToolTip()
{
    const QString oldToolTip = m_ui->tooltipEditor->text();
    RichTextEditorDialog richTextDialog(m_actionEditor->core(), this);
    richTextDialog.setText(oldToolTip);
    if (richTextDialog.showDialog() == QDialog::Rejected)
        return;
    const QString newToolTip = richTextDialog.text();
    if (newToolTip != oldToolTip)
        m_ui->tooltipEditor->setText(newToolTip);
}

void NewActionDialog::slotResetKeySequence()
{
    m_ui->keySequenceEdit->setKeySequence(QKeySequence());
    m_ui->keySequenceEdit->setFocus(Qt::MouseFocusReason);
}

void NewActionDialog::updateButtons()
{
    QPushButton *okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!actionText().isEmpty() && !actionName().isEmpty());
}

} // namespace qdesigner_internal

QT_END_NAMESPACE
