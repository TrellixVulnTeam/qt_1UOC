// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktreeview_p_p.h"

#include <QtCore/qobject.h>
#include <QtQml/qqmlcontext.h>

#include <QtQmlModels/private/qqmltreemodeltotablemodel_p_p.h>

/*!
    \qmltype TreeView
    \inqmlmodule QtQuick
    \ingroup qtquick-views
    \since 6.3
    \inherits TableView
    \brief Provides a tree view to display data from a QAbstractItemModel.

    A TreeView has a \l model that defines the data to be displayed, and a
    \l delegate that defines how the data should be displayed.

    TreeView inherits \l TableView. This means that even if the model
    has a parent-child tree structure, TreeView is internally using a
    proxy model that converts that structure into a flat table
    model that can be rendered by TableView. Each node in the tree ends up
    occupying one row in the table, where the first column renders the tree
    itself. By indenting each delegate item in that column according to its
    parent-child depth in the model, it will end up looking like a tree, even
    if it's technically still just a flat list of items.

    To allow for maximum flexibility, TreeView itself will not position the delegate
    items into a tree structure. This burden is placed on the delegate.
    \l {Qt Quick Controls} offers a ready-made TreeViewDelegate that can be
    used for this, which has the advantage that it works out-of-the-box and
    renders a tree which follows the style of the platform where the application runs.

    Even if TreeViewDelegate is customizable, there might be situations
    where you want to render the tree in a different way, or ensure that
    the delegate ends up as minimal as possible, perhaps for performance reasons.
    Creating your own delegate from scratch is easy, since TreeView offers
    a set of properties that can be used to position and render each node
    in the tree correctly.

    An example of a custom delegate is shown below:

    \snippet qml/treeview/qml-customdelegate.qml 0

    The properties that are marked as \c required will be filled in by
    TreeView, and are similar to attached properties. By marking them as
    required, the delegate indirectly informs TreeView that it should take
    responsibility for assigning them values. The following required properties
    can be added to a delegate:

    \list
        \li \c {required property TreeView treeView}
                - Points to the TreeView that contains the delegate item.
        \li \c {required property bool isTreeNode}
                - Is \c true if the delegate item represents a node in
                the tree. Only one column in the view will be used to draw the tree, and
                therefore, only delegate items in that column will have this
                property set to \c true.
                A node in the tree should typically be indented according to its
                \c depth, and show an indicator if \c hasChildren is \c true.
                Delegate items in other columns will have this property set to
                \c false, and will show data from the remaining columns
                in the model (and typically not be indented).
        \li \c {required property bool expanded}
                - Is \c true if the model item drawn by the delegate is expanded
                in the view.
        \li \c {required property int hasChildren}
                - Is \c true if the model item drawn by the delegate has children
                in the model.
        \li \c {required property int depth}
                - Contains the depth of the model item drawn by the delegate.
                The depth of a model item is the same as the number of ancestors
                it has in the model.
    \endlist

    See also \l {Required Properties}.

    \note A TreeView only accepts a model that inherits \l QAbstractItemModel.
*/

/*!
    \qmlmethod int QtQuick::TreeView::depth(row)

    Returns the depth (the number of parents up to the root) of the given \a row.

    \a row should be the row in the view (table row), and not a row in the model.
    If \a row is not between \c 0 and \l {TableView::}{rows}, the return value will
    be \c -1.

    \sa {TableView::}{modelIndex()}
*/

/*!
    \qmlmethod bool QtQuick::TreeView::isExpanded(row)

    Returns if the given \a row in the view is shown as expanded.

    \a row should be the row in the view (table row), and not a row in the model.
    If \a row is not between \c 0 and \l {TableView::}{rows}, the return value will
    be \c false.
*/

/*!
    \qmlmethod QtQuick::TreeView::expand(row)

    Expands the tree node at the given \a row in the view.

    \a row should be the row in the view (table row), and not a row in the model.

    \note this function will not affect the model, only
    the visual representation in the view.

    \sa collapse(), isExpanded(), expandRecursively()
*/

/*!
    \qmlmethod QtQuick::TreeView::expandRecursively(row = -1, depth = -1)
    \since 6.4

    Expands the tree node at the given \a row in the view recursively down to
    \a depth. \a depth should be relative to the depth of \a row. If
    \a depth is \c -1, the tree will be expanded all the way down to all leaves.

    For a model that has more than one root, you can also call this function
    with \a row equal to \c -1. This will expand all roots. Hence, calling
    expandRecursively(-1, -1), or simply expandRecursively(), will expand
    all nodes in the model.

    \a row should be the row in the view (table row), and not a row in the model.

    \note This function will not try to \l{QAbstractItemModel::fetchMore}{fetch more} data.
    \note This function will not affect the model, only the visual representation in the view.
    \warning If the model contains a large number of items, this function will
    take some time to execute.

    \sa collapseRecursively(), expand(), collapse(), isExpanded(), depth()
*/

/*!
    \qmlmethod QtQuick::TreeView::expandToIndex(QModelIndex index)
    \since 6.4

    Expands the tree from the given model \a index, and recursively all the way up
    to the root. The result will be that the delegate item that represents \a index
    becomes visible in the view (unless it ends up outside the viewport). To
    ensure that the row ends up visible in the viewport, you can do:

    \code
        expandToIndex(index)
        forceLayout()
        positionViewAtRow(rowAtIndex(index), Qt.AlignVCenter)
    \endcode

    \sa expand(), expandRecursively()
*/

/*!
    \qmlmethod QtQuick::TreeView::collapse(row)

    Collapses the tree node at the given \a row in the view.

    \a row should be the row in the view (table row), and not a row in the model.

    \note this function will not affect the model, only
    the visual representation in the view.

    \sa expand(), isExpanded()
*/

/*!
    \qmlmethod QtQuick::TreeView::collapseRecursively(row = -1)
    \since 6.4

    Collapses the tree node at the given \a row in the view recursively down to
    all leaves.

    For a model has more than one root, you can also call this function
    with \a row equal to \c -1. This will collapse all roots. Hence, calling
    collapseRecursively(-1), or simply collapseRecursively(), will collapse
    all nodes in the model.

    \a row should be the row in the view (table row), and not a row in the model.

    \note this function will not affect the model, only
    the visual representation in the view.

    \sa expandRecursively(), expand(), collapse(), isExpanded(), depth()
*/

/*!
    \qmlmethod QtQuick::TreeView::toggleExpanded(row)

    Toggles if the tree node at the given \a row should be expanded.
    This is a convenience for doing:

    \code
    if (isExpanded(row))
        collapse(row)
    else
        expand(row)
    \endcode

    \a row should be the row in the view (table row), and not a row in the model.
*/

/*!
    \qmlsignal QtQuick::TreeView::expanded(row, depth)

    This signal is emitted when a \a row is expanded in the view.
    \a row and \a depth will be equal to the arguments given to the call
    that caused the expansion to happen (\l expand() or \l expandRecursively()).
    In case of \l expand(), \a depth will always be \c 1.
    In case of \l expandToIndex(), \a depth will be the depth of the
    target index.

    \note when a row is expanded recursively, the expanded signal will
    only be emitted for that one row, and not for its descendants.

    \sa collapsed(), expand(), collapse(), toggleExpanded()
*/

/*!
    \qmlsignal QtQuick::TreeView::collapsed(row, recursively)

    This signal is emitted when a \a row is collapsed in the view.
    \a row will be equal to the argument given to the call that caused
    the collapse to happen (\l collapse() or \l collapseRecursively()).
    If the row was collapsed recursively, \a recursively will be \c true.

    \note when a row is collapsed recursively, the collapsed signal will
    only be emitted for that one row, and not for its descendants.

    \sa expanded(), expand(), collapse(), toggleExpanded()
*/

// Hard-code the tree column to be 0 for now
static const int kTreeColumn = 0;

QT_BEGIN_NAMESPACE

QQuickTreeViewPrivate::QQuickTreeViewPrivate()
    : QQuickTableViewPrivate()
{
}

QQuickTreeViewPrivate::~QQuickTreeViewPrivate()
{
}

QVariant QQuickTreeViewPrivate::modelImpl() const
{
    return m_assignedModel;
}

void QQuickTreeViewPrivate::setModelImpl(const QVariant &newModel)
{
    Q_Q(QQuickTreeView);

    if (newModel == m_assignedModel)
        return;

    m_assignedModel = newModel;
    QVariant effectiveModel = m_assignedModel;
    if (effectiveModel.userType() == qMetaTypeId<QJSValue>())
        effectiveModel = effectiveModel.value<QJSValue>().toVariant();

    if (effectiveModel.isNull())
        m_treeModelToTableModel.setModel(nullptr);
    else if (const auto qaim = qvariant_cast<QAbstractItemModel*>(effectiveModel))
        m_treeModelToTableModel.setModel(qaim);
    else
        qmlWarning(q) << "TreeView only accepts a model of type QAbstractItemModel";


    scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::All);
    emit q->modelChanged();
}

void QQuickTreeViewPrivate::initItemCallback(int serializedModelIndex, QObject *object)
{
    updateRequiredProperties(serializedModelIndex, object, true);
    QQuickTableViewPrivate::initItemCallback(serializedModelIndex, object);
}

void QQuickTreeViewPrivate::itemReusedCallback(int serializedModelIndex, QObject *object)
{
    updateRequiredProperties(serializedModelIndex, object, false);
    QQuickTableViewPrivate::itemReusedCallback(serializedModelIndex, object);
}

void QQuickTreeViewPrivate::dataChangedCallback(
        const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_Q(QQuickTreeView);
    Q_UNUSED(roles);

    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        for (int column = topLeft.column(); column <= bottomRight.column(); ++column) {
            const QPoint cell(column, row);
            auto item = q->itemAtCell(cell);
            if (!item)
                continue;

            const int serializedModelIndex = modelIndexAtCell(QPoint(column, row));
            updateRequiredProperties(serializedModelIndex, item, false);
        }
    }
}

void QQuickTreeViewPrivate::updateRequiredProperties(int serializedModelIndex, QObject *object, bool init)
{
    Q_Q(QQuickTreeView);
    const QPoint cell = cellAtModelIndex(serializedModelIndex);
    const int row = cell.y();
    const int column = cell.x();

    setRequiredProperty("treeView", QVariant::fromValue(q), serializedModelIndex, object, init);
    setRequiredProperty("isTreeNode", column == kTreeColumn, serializedModelIndex, object, init);
    setRequiredProperty("hasChildren", m_treeModelToTableModel.hasChildren(row), serializedModelIndex, object, init);
    setRequiredProperty("expanded", q->isExpanded(row), serializedModelIndex, object, init);
    setRequiredProperty("depth", m_treeModelToTableModel.depthAtRow(row), serializedModelIndex, object, init);
}

void QQuickTreeViewPrivate::updateSelection(const QRect &oldSelection, const QRect &newSelection)
{
    Q_Q(QQuickTreeView);

    const QRect oldRect = oldSelection.normalized();
    const QRect newRect = newSelection.normalized();

    if (oldSelection == newSelection)
        return;

    // Select the rows inside newRect that doesn't overlap with oldRect
    for (int row = newRect.y(); row <= newRect.y() + newRect.height(); ++row) {
        if (oldRect.y() != -1 && oldRect.y() <= row && row <= oldRect.y() + oldRect.height())
            continue;
        const QModelIndex startIndex = q->modelIndex(newRect.x(), row);
        const QModelIndex endIndex = q->modelIndex(newRect.x() + newRect.width(), row);
        selectionModel->select(QItemSelection(startIndex, endIndex), QItemSelectionModel::Select);
    }

    if (oldRect.x() != -1) {
        // Since oldRect is valid, this update is a continuation of an already existing selection!

        // Select the columns inside newRect that don't overlap with oldRect
        for (int column = newRect.x(); column <= newRect.x() + newRect.width(); ++column) {
            if (oldRect.x() <= column && column <= oldRect.x() + oldRect.width())
                continue;
            for (int row = newRect.y(); row <= newRect.y() + newRect.height(); ++row)
                selectionModel->select(q->modelIndex(column, row), QItemSelectionModel::Select);
        }

        // Unselect the rows inside oldRect that don't overlap with newRect
        for (int row = oldRect.y(); row <= oldRect.y() + oldRect.height(); ++row) {
            if (newRect.y() <= row && row <= newRect.y() + newRect.height())
                continue;
            const QModelIndex startIndex = q->modelIndex(oldRect.x(), row);
            const QModelIndex endIndex = q->modelIndex(oldRect.x() + oldRect.width(), row);
            selectionModel->select(QItemSelection(startIndex, endIndex), QItemSelectionModel::Deselect);
        }

        // Unselect the columns inside oldRect that don't overlap with newRect
        for (int column = oldRect.x(); column <= oldRect.x() + oldRect.width(); ++column) {
            if (newRect.x() <= column && column <= newRect.x() + newRect.width())
                continue;
            // Since we're not allowed to call select/unselect on the selectionModel with
            // indices from different parents, and since indicies from different parents are
            // expected when working with trees, we need to unselect the indices in the column
            // one by one, rather than the whole column in one go. This, however, can cause a
            // lot of selection fragments in the selectionModel, which eventually can hurt
            // performance. But large selections containing a lot of columns is not normally
            // the case for a treeview, so accept this potential corner case for now.
            for (int row = newRect.y(); row <= newRect.y() + newRect.height(); ++row)
                selectionModel->select(q->modelIndex(column, row), QItemSelectionModel::Deselect);
        }
    }
}

QQuickTreeView::QQuickTreeView(QQuickItem *parent)
    : QQuickTableView(*(new QQuickTreeViewPrivate), parent)
{
    Q_D(QQuickTreeView);

    setSelectionBehavior(SelectRows);

    // Note: QQuickTableView will only ever see the table model m_treeModelToTableModel, and
    // never the actual tree model that is assigned to us by the application.
    const auto modelAsVariant = QVariant::fromValue(std::addressof(d->m_treeModelToTableModel));
    d->QQuickTableViewPrivate::setModelImpl(modelAsVariant);
    QObjectPrivate::connect(&d->m_treeModelToTableModel, &QAbstractItemModel::dataChanged,
                            d, &QQuickTreeViewPrivate::dataChangedCallback);
}

QQuickTreeView::~QQuickTreeView()
{
}

int QQuickTreeView::depth(int row) const
{
    Q_D(const QQuickTreeView);
    if (row < 0 || row >= d->m_treeModelToTableModel.rowCount())
        return -1;

    return d->m_treeModelToTableModel.depthAtRow(row);
}

bool QQuickTreeView::isExpanded(int row) const
{
    Q_D(const QQuickTreeView);
    if (row < 0 || row >= d->m_treeModelToTableModel.rowCount())
        return false;

    return d->m_treeModelToTableModel.isExpanded(row);
}

void QQuickTreeView::expand(int row)
{
    if (row >= 0)
        expandRecursively(row, 1);
}

void QQuickTreeView::expandRecursively(int row, int depth)
{
    Q_D(QQuickTreeView);
    if (row >= d->m_treeModelToTableModel.rowCount())
        return;
    if (row < 0 && row != -1)
        return;
    if (depth == 0 || depth < -1)
        return;

    auto expandRowRecursively = [this, d, depth](int startRow) {
        d->m_treeModelToTableModel.expandRecursively(startRow, depth);
        // Update the expanded state of the startRow. The descendant rows that gets
        // expanded will get the correct state set from initItem/itemReused instead.
        for (int c = leftColumn(); c <= rightColumn(); ++c) {
            const QPoint treeNodeCell(c, startRow);
            if (const auto item = itemAtCell(treeNodeCell))
                d->setRequiredProperty("expanded", true, d->modelIndexAtCell(treeNodeCell), item, false);
        }
    };

    if (row >= 0) {
        // Expand only one row recursively
        const bool isExpanded = d->m_treeModelToTableModel.isExpanded(row);
        if (isExpanded && depth == 1)
            return;
        expandRowRecursively(row);
    } else {
        // Expand all root nodes recursively
        const auto model = d->m_treeModelToTableModel.model();
        for (int r = 0; r < model->rowCount(); ++r) {
            const int rootRow = d->m_treeModelToTableModel.itemIndex(model->index(r, 0));
            if (rootRow != -1)
                expandRowRecursively(rootRow);
        }
    }

    emit expanded(row, depth);
}

void QQuickTreeView::expandToIndex(const QModelIndex &index)
{
    Q_D(QQuickTreeView);

    if (!index.isValid()) {
        qmlWarning(this) << "index is not valid: " << index;
        return;
    }

    if (index.model() != d->m_treeModelToTableModel.model()) {
        qmlWarning(this) << "index doesn't belong to correct model: " << index;
        return;
    }

    if (rowAtIndex(index) != -1) {
        // index is already visible
        return;
    }

    int depth = 1;
    QModelIndex parent = index.parent();
    int row = rowAtIndex(parent);

    while (parent.isValid()) {
        if (row != -1) {
            // The node is already visible, since it maps to a row in the table!
            d->m_treeModelToTableModel.expandRow(row);

            // Update the state of the already existing delegate item
            for (int c = leftColumn(); c <= rightColumn(); ++c) {
                const QPoint treeNodeCell(c, row);
                if (const auto item = itemAtCell(treeNodeCell))
                    d->setRequiredProperty("expanded", true, d->modelIndexAtCell(treeNodeCell), item, false);
            }

            // When we hit a node that is visible, we know that all other nodes
            // up to the parent have to be visible as well, so we can stop.
            break;
        } else {
            d->m_treeModelToTableModel.expand(parent);
            parent = parent.parent();
            row = rowAtIndex(parent);
            depth++;
        }
    }

    emit expanded(row, depth);
}

void QQuickTreeView::collapse(int row)
{
    Q_D(QQuickTreeView);
    if (row < 0 || row >= d->m_treeModelToTableModel.rowCount())
        return;

    if (!d->m_treeModelToTableModel.isExpanded(row))
        return;

    d_func()->m_treeModelToTableModel.collapseRow(row);

    for (int c = leftColumn(); c <= rightColumn(); ++c) {
        const QPoint treeNodeCell(c, row);
        if (const auto item = itemAtCell(treeNodeCell))
            d->setRequiredProperty("expanded", false, d->modelIndexAtCell(treeNodeCell), item, false);
    }

    emit collapsed(row, false);
}

void QQuickTreeView::collapseRecursively(int row)
{
    Q_D(QQuickTreeView);
    if (row >= d->m_treeModelToTableModel.rowCount())
        return;
    if (row < 0 && row != -1)
        return;

    auto collapseRowRecursive = [this, d](int startRow) {
        // Always collapse descendants recursively,
        // even if the top row itself is already collapsed.
        d->m_treeModelToTableModel.collapseRecursively(startRow);
        // Update the expanded state of the (still visible) startRow
        for (int c = leftColumn(); c <= rightColumn(); ++c) {
            const QPoint treeNodeCell(c, startRow);
            if (const auto item = itemAtCell(treeNodeCell))
                d->setRequiredProperty("expanded", false, d->modelIndexAtCell(treeNodeCell), item, false);
        }
    };

    if (row >= 0) {
        collapseRowRecursive(row);
    } else {
        // Collapse all root nodes recursively
        const auto model = d->m_treeModelToTableModel.model();
        for (int r = 0; r < model->rowCount(); ++r) {
            const int rootRow = d->m_treeModelToTableModel.itemIndex(model->index(r, 0));
            if (rootRow != -1)
                collapseRowRecursive(rootRow);
        }
    }

    emit collapsed(row, true);
}

void QQuickTreeView::toggleExpanded(int row)
{
    if (isExpanded(row))
        collapse(row);
    else
        expand(row);
}

QModelIndex QQuickTreeView::modelIndex(const QPoint &cell) const
{
    Q_D(const QQuickTreeView);
    const QModelIndex tableIndex = d->m_treeModelToTableModel.index(cell.y(), cell.x());
    return d->m_treeModelToTableModel.mapToModel(tableIndex);
}

QPoint QQuickTreeView::cellAtIndex(const QModelIndex &index) const
{
    const QModelIndex tableIndex = d_func()->m_treeModelToTableModel.mapFromModel(index);
    return QPoint(tableIndex.column(), tableIndex.row());
}

QModelIndex QQuickTreeView::modelIndex(int column, int row) const
{
    return modelIndex({column, row});
}

void QQuickTreeView::keyPressEvent(QKeyEvent *event)
{
    event->ignore();

    if (!keyNavigationEnabled())
        return;
    if (!selectionModel())
        return;

    const int row = cellAtIndex(selectionModel()->currentIndex()).y();
    switch (event->key()) {
    case Qt::Key_Left:
        collapse(row);
        event->accept();
        break;
    case Qt::Key_Right:
        expand(row);
        event->accept();
        break;
    default:
        break;
    }

    if (!event->isAccepted())
        QQuickTableView::keyPressEvent(event);
}

QT_END_NAMESPACE

#include "moc_qquicktreeview_p.cpp"
