// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qbuttongroup_p.h"

#include "private/qabstractbutton_p.h"

QT_BEGIN_NAMESPACE

// detect a checked button other than the current one
void QButtonGroupPrivate::detectCheckedButton()
{
    QAbstractButton *previous = checkedButton;
    checkedButton = nullptr;
    if (exclusive)
        return;
    for (int i = 0; i < buttonList.count(); i++) {
        if (buttonList.at(i) != previous && buttonList.at(i)->isChecked()) {
            checkedButton = buttonList.at(i);
            return;
        }
    }
}

/*!
    \class QButtonGroup
    \brief The QButtonGroup class provides a container to organize groups of
    button widgets.

    \ingroup organizers
    \ingroup geomanagement
    \inmodule QtWidgets

    QButtonGroup provides an abstract container into which button widgets can
    be placed. It does not provide a visual representation of this container
    (see QGroupBox for a container widget), but instead manages the states of
    each of the buttons in the group.

    An \l {QButtonGroup::exclusive} {exclusive} button group switches
    off all checkable (toggle) buttons except the one that has been
    clicked. By default, a button group is exclusive. The buttons in a
    button group are usually checkable \l{QPushButton}s, \l{QCheckBox}es
    (normally for non-exclusive button groups), or \l{QRadioButton}s.
    If you create an exclusive button group, you should ensure that
    one of the buttons in the group is initially checked; otherwise,
    the group will initially be in a state where no buttons are
    checked.

    A button can be added to the group with addButton() and removed
    with removeButton(). If the group is exclusive, the
    currently checked button is available with checkedButton(). If a
    button is clicked, the buttonClicked() signal is emitted; for a
    checkable button in an exclusive group this means that the button
    has been checked. The list of buttons in the group is returned by
    buttons().

    In addition, QButtonGroup can map between integers and buttons.
    You can assign an integer id to a button with setId(), and
    retrieve it with id(). The id of the currently checked button is
    available with checkedId(), and there is an overloaded signal
    buttonClicked() which emits the id of the button. The id \c {-1}
    is reserved by QButtonGroup to mean "no such button". The purpose
    of the mapping mechanism is to simplify the representation of enum
    values in a user interface.

    \sa QGroupBox, QPushButton, QCheckBox, QRadioButton
*/

/*!
    Constructs a new, empty button group with the given \a parent.

    \sa addButton(), setExclusive()
*/
QButtonGroup::QButtonGroup(QObject *parent)
    : QObject(*new QButtonGroupPrivate, parent)
{
}

/*!
    Destroys the button group.
*/
QButtonGroup::~QButtonGroup()
{
    Q_D(QButtonGroup);
    for (int i = 0; i < d->buttonList.count(); ++i)
        d->buttonList.at(i)->d_func()->group = nullptr;
}

/*!
    \property QButtonGroup::exclusive
    \brief whether the button group is exclusive

    If this property is \c true, then only one button in the group can be checked
    at any given time. The user can click on any button to check it, and that
    button will replace the existing one as the checked button in the group.

    In an exclusive group, the user cannot uncheck the currently checked button
    by clicking on it; instead, another button in the group must be clicked
    to set the new checked button for that group.

    By default, this property is \c true.
*/
bool QButtonGroup::exclusive() const
{
    Q_D(const QButtonGroup);
    return d->exclusive;
}

void QButtonGroup::setExclusive(bool exclusive)
{
    Q_D(QButtonGroup);
    d->exclusive = exclusive;
}



/*!
    \fn void QButtonGroup::buttonClicked(QAbstractButton *button)

    This signal is emitted when the given \a button is clicked. A
    button is clicked when it is first pressed and then released, when
    its shortcut key is typed, or when QAbstractButton::click()
    or QAbstractButton::animateClick() is programmatically called.


    \sa checkedButton(), QAbstractButton::clicked()
*/

/*!
    \fn void QButtonGroup::idClicked(int id)
    \since 5.15

    This signal is emitted when a button with the given \a id is
    clicked.

    \sa checkedButton(), QAbstractButton::clicked()
*/

/*!
    \fn void QButtonGroup::buttonPressed(QAbstractButton *button)
    \since 4.2

    This signal is emitted when the given \a button is pressed down.

    \sa QAbstractButton::pressed()
*/

/*!
    \fn void QButtonGroup::idPressed(int id)
    \since 5.15

    This signal is emitted when a button with the given \a id is
    pressed down.

    \sa QAbstractButton::pressed()
*/

/*!
    \fn void QButtonGroup::buttonReleased(QAbstractButton *button)
    \since 4.2

    This signal is emitted when the given \a button is released.

    \sa QAbstractButton::released()
*/

/*!
    \fn void QButtonGroup::idReleased(int id)
    \since 5.15

    This signal is emitted when a button with the given \a id is
    released.

    \sa QAbstractButton::released()
*/

/*!
    \fn void QButtonGroup::buttonToggled(QAbstractButton *button, bool checked)
    \since 5.2

    This signal is emitted when the given \a button is toggled.
    \a checked is true if the button is checked, or false if the button is unchecked.

    \sa QAbstractButton::toggled()
*/

/*!
    \fn void QButtonGroup::idToggled(int id, bool checked)
    \since 5.15

    This signal is emitted when a button with the given \a id is toggled.
    \a checked is true if the button is checked, or false if the button is unchecked.

    \sa QAbstractButton::toggled()
*/

/*!
    Adds the given \a button to the button group. If \a id is -1,
    an id will be assigned to the button.
    Automatically assigned ids are guaranteed to be negative,
    starting with -2. If you are assigning your own ids, use
    positive values to avoid conflicts.

    \sa removeButton(), buttons()
*/
void QButtonGroup::addButton(QAbstractButton *button, int id)
{
    Q_D(QButtonGroup);
    if (QButtonGroup *previous = button->d_func()->group)
        previous->removeButton(button);
    button->d_func()->group = this;
    d->buttonList.append(button);
    if (id == -1) {
        const QHash<QAbstractButton*, int>::const_iterator it
                = std::min_element(d->mapping.cbegin(), d->mapping.cend());
        if (it == d->mapping.cend())
            d->mapping[button] = -2;
        else
            d->mapping[button] = *it - 1;
    } else {
        d->mapping[button] = id;
    }

    if (d->exclusive && button->isChecked())
        button->d_func()->notifyChecked();
}

/*!
    Removes the given \a button from the button group.

    \sa addButton(), buttons()
*/
void QButtonGroup::removeButton(QAbstractButton *button)
{
    Q_D(QButtonGroup);
    if (d->checkedButton == button) {
        d->detectCheckedButton();
    }
    if (button->d_func()->group == this) {
        button->d_func()->group = nullptr;
        d->buttonList.removeAll(button);
        d->mapping.remove(button);
    }
}

/*!
    Returns the button group's list of buttons. This may be empty.

    \sa addButton(), removeButton()
*/
QList<QAbstractButton*> QButtonGroup::buttons() const
{
    Q_D(const QButtonGroup);
    return d->buttonList;
}

/*!
    Returns the button group's checked button, or \nullptr if no
    buttons are checked.

    \sa buttonClicked()
*/
QAbstractButton *QButtonGroup::checkedButton() const
{
    Q_D(const QButtonGroup);
    return d->checkedButton;
}

/*!
    \since 4.1

    Returns the button with the specified \a id, or \nullptr if no
    such button exists.
*/
QAbstractButton *QButtonGroup::button(int id) const
{
    Q_D(const QButtonGroup);
    return d->mapping.key(id);
}

/*!
    \since 4.1

    Sets the \a id for the specified \a button. Note that \a id cannot
    be -1.

    \sa id()
*/
void QButtonGroup::setId(QAbstractButton *button, int id)
{
    Q_D(QButtonGroup);
    if (button && id != -1)
        d->mapping[button] = id;
}

/*!
    \since 4.1

    Returns the id for the specified \a button, or -1 if no such button
    exists.


    \sa setId()
*/
int QButtonGroup::id(QAbstractButton *button) const
{
    Q_D(const QButtonGroup);
    return d->mapping.value(button, -1);
}

/*!
    \since 4.1

    Returns the id of the checkedButton(), or -1 if no button is checked.

    \sa setId()
*/
int QButtonGroup::checkedId() const
{
    Q_D(const QButtonGroup);
    return d->mapping.value(d->checkedButton, -1);
}

QT_END_NAMESPACE

#include "moc_qbuttongroup.cpp"
