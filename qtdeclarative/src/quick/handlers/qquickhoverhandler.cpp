// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickhoverhandler_p.h"
#include <private/qquicksinglepointhandler_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHoverHandler, "qt.quick.handler.hover")

/*!
    \qmltype HoverHandler
    \instantiates QQuickHoverHandler
    \inherits SinglePointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for mouse and tablet hover.

    HoverHandler detects a hovering mouse or tablet stylus cursor.

    A binding to the \l hovered property is the easiest way to react when the
    cursor enters or leaves the \l {PointerHandler::parent}{parent} Item.
    The \l {SinglePointHandler::point}{point} property provides more detail,
    including the cursor position. The
    \l {PointerDeviceHandler::acceptedDevices}{acceptedDevices},
    \l {PointerDeviceHandler::acceptedPointerTypes}{acceptedPointerTypes},
    and \l {PointerDeviceHandler::acceptedModifiers}{acceptedModifiers}
    properties can be used to narrow the behavior to detect hovering of
    specific kinds of devices or while holding a modifier key.

    The \l cursorShape property allows changing the cursor whenever
    \l hovered changes to \c true.

    \sa MouseArea, PointHandler
*/

QQuickHoverHandler::QQuickHoverHandler(QQuickItem *parent)
    : QQuickSinglePointHandler(parent)
{
    // Tell QQuickPointerDeviceHandler::wantsPointerEvent() to ignore button state
    d_func()->acceptedButtons = Qt::NoButton;
}

QQuickHoverHandler::~QQuickHoverHandler()
{
    if (auto parent = parentItem())
        QQuickItemPrivate::get(parent)->setHasHoverInChild(false);
}

/*!
    \qmlproperty bool QtQuick::HoverHandler::blocking
    \since 6.3

    Whether this handler prevents other items or handlers behind it from
    being hovered at the same timee. This property is \c false by default.
*/
void QQuickHoverHandler::setBlocking(bool blocking)
{
    if (m_blocking == blocking)
        return;

    m_blocking = blocking;
    emit blockingChanged();
}

bool QQuickHoverHandler::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::HoverLeave:
        setHovered(false);
        setActive(false);
        break;
    default:
        return QQuickSinglePointHandler::event(event);
        break;
    }

    return true;
}

void QQuickHoverHandler::componentComplete()
{
    QQuickSinglePointHandler::componentComplete();
    if (auto par = parentItem()) {
        par->setAcceptHoverEvents(true);
        QQuickItemPrivate::get(par)->setHasHoverInChild(true);
    }
}

bool QQuickHoverHandler::wantsPointerEvent(QPointerEvent *event)
{
    // No state change should occur if a button is being pressed or released.
    if (event->isSinglePointEvent() && static_cast<QSinglePointEvent *>(event)->button())
        return false;
    auto &point = event->point(0);
    if (QQuickPointerDeviceHandler::wantsPointerEvent(event) && wantsEventPoint(event, point) && parentContains(point)) {
        // assume this is a mouse or tablet event, so there's only one point
        setPointId(point.id());
        return true;
    }

    // Some hover events come from QQuickWindow::tabletEvent(). In between,
    // some hover events come from QQWindowPrivate::flushFrameSynchronousEvents(),
    // but those look like mouse events. If a particular HoverHandler instance
    // is filtering for tablet events only (e.g. by setting
    // acceptedDevices:PointerDevice.Stylus), those events should not cause
    // the hovered property to transition to false prematurely.
    // If a QQuickPointerTabletEvent caused the hovered property to become true,
    // then only another QQuickPointerTabletEvent can make it become false.
    // But after kCursorOverrideTimeout ms, QQuickItemPrivate::effectiveCursorHandler()
    // will ignore it, just in case there is no QQuickPointerTabletEvent to unset it.
    // For example, a tablet proximity leave event could occur, but we don't deliver it to the window.
    if (!(m_hoveredTablet && QQuickDeliveryAgentPrivate::isMouseEvent(event)))
        setHovered(false);

    return false;
}

void QQuickHoverHandler::handleEventPoint(QPointerEvent *ev, QEventPoint &point)
{
    bool hovered = true;
    if (point.state() == QEventPoint::Released &&
            ev->pointingDevice()->pointerType() == QPointingDevice::PointerType::Finger)
        hovered = false;
    else if (QQuickDeliveryAgentPrivate::isTabletEvent(ev))
        m_hoveredTablet = true;
    setHovered(hovered);
}

/*!
    \qmlproperty bool QtQuick::HoverHandler::hovered
    \readonly

    Holds true whenever any pointing device cursor (mouse or tablet) is within
    the bounds of the \c parent Item, extended by the
    \l {PointerHandler::margin}{margin}, if any.
*/
void QQuickHoverHandler::setHovered(bool hovered)
{
    if (m_hovered != hovered) {
        qCDebug(lcHoverHandler) << objectName() << "hovered" << m_hovered << "->" << hovered;
        m_hovered = hovered;
        if (!hovered)
            m_hoveredTablet = false;
        emit hoveredChanged();
    }
}

/*!
    \internal
    \qmlproperty flags QtQuick::HoverHandler::acceptedButtons

    This property is not used in HoverHandler.
*/

/*!
    \qmlproperty flags QtQuick::HoverHandler::acceptedDevices

    The types of pointing devices that can activate the pointer handler.

    By default, this property is set to
    \l{QInputDevice::DeviceType}{PointerDevice.AllDevices}.
    If you set it to an OR combination of device types, it will ignore pointer
    events from the non-matching devices.

    For example, an item could be made to respond to mouse hover in one way,
    and stylus hover in another way, with two handlers:

    \snippet pointerHandlers/hoverMouseOrStylus.qml 0

    The available device types are as follows:

    \value PointerDevice.Mouse          A mouse.
    \value PointerDevice.TouchScreen    A touchscreen.
    \value PointerDevice.TouchPad       A touchpad or trackpad.
    \value PointerDevice.Stylus         A stylus on a graphics tablet.
    \value PointerDevice.Airbrush       An airbrush on a graphics tablet.
    \value PointerDevice.Puck           A digitizer with crosshairs, on a graphics tablet.
    \value PointerDevice.AllDevices     Any type of pointing device.

    \sa QInputDevice::DeviceType
*/

/*!
    \qmlproperty flags QtQuick::HoverHandler::acceptedPointerTypes

    The types of pointing instruments (generic, stylus, eraser, and so on)
    that can activate the pointer handler.

    By default, this property is set to
    \l {QPointingDevice::PointerType} {PointerDevice.AllPointerTypes}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching events.

    For example, you could provide feedback by changing the cursor depending on
    whether a stylus or eraser is hovering over a graphics tablet:

    \snippet pointerHandlers/hoverStylusOrEraser.qml 0

    The available pointer types are as follows:

    \value PointerDevice.Generic          A mouse or a device that emulates a mouse.
    \value PointerDevice.Finger           A finger on a touchscreen (hover detection is unlikely).
    \value PointerDevice.Pen              A stylus on a graphics tablet.
    \value PointerDevice.Eraser           An eraser on a graphics tablet.
    \value PointerDevice.Cursor           A digitizer with crosshairs, on a graphics tablet.
    \value PointerDevice.AllPointerTypes  Any type of pointing device.

    \sa QPointingDevice::PointerType
*/

/*!
    \qmlproperty flags QtQuick::HoverHandler::acceptedModifiers

    If this property is set, a hover event is handled only if the given keyboard
    modifiers are pressed. The event is ignored without the modifiers.

    This property is set to \c Qt.KeyboardModifierMask by default, resulting
    in handling hover events regardless of any modifier keys.

    For example, an \l[QML]{Item} could have two handlers of the same type, one
    of which is enabled only if the required keyboard modifiers are pressed:

    \snippet pointerHandlers/hoverModifiers.qml 0

    The available modifiers are as follows:

    \value Qt.NoModifier        No modifier key is allowed.
    \value Qt.ShiftModifier     A Shift key on the keyboard must be pressed.
    \value Qt.ControlModifier   A Ctrl key on the keyboard must be pressed.
    \value Qt.AltModifier       An Alt key on the keyboard must be pressed.
    \value Qt.MetaModifier      A Meta key on the keyboard must be pressed.
    \value Qt.KeypadModifier    A keypad button must be pressed.
    \value Qt.GroupSwitchModifier A Mode_switch key on the keyboard must be pressed.
                                X11 only (unless activated on Windows by a command line argument).
    \value Qt.KeyboardModifierMask The handler ignores modifier keys.

    \sa Qt::KeyboardModifier
*/

/*!
    \since 5.15
    \qmlproperty Qt::CursorShape QtQuick::HoverHandler::cursorShape
    This property holds the cursor shape that will appear whenever
    \l hovered is \c true and no other handler is overriding it.

    The available cursor shapes are:
    \list
    \li Qt.ArrowCursor
    \li Qt.UpArrowCursor
    \li Qt.CrossCursor
    \li Qt.WaitCursor
    \li Qt.IBeamCursor
    \li Qt.SizeVerCursor
    \li Qt.SizeHorCursor
    \li Qt.SizeBDiagCursor
    \li Qt.SizeFDiagCursor
    \li Qt.SizeAllCursor
    \li Qt.BlankCursor
    \li Qt.SplitVCursor
    \li Qt.SplitHCursor
    \li Qt.PointingHandCursor
    \li Qt.ForbiddenCursor
    \li Qt.WhatsThisCursor
    \li Qt.BusyCursor
    \li Qt.OpenHandCursor
    \li Qt.ClosedHandCursor
    \li Qt.DragCopyCursor
    \li Qt.DragMoveCursor
    \li Qt.DragLinkCursor
    \endlist

    The default value of this property is not set, which allows any active
    handler on the same \e parent item to determine the cursor shape.
    This property can be reset to the initial condition by setting it to
    \c undefined.

    If any handler with defined \c cursorShape is
    \l {PointerHandler::active}{active}, that cursor will appear.
    Else if the HoverHandler has a defined \c cursorShape, that cursor will appear.
    Otherwise, the \l {QQuickItem::cursor()}{cursor} of \e parent item will appear.

    \note When this property has not been set, or has been set to \c undefined,
    if you read the value it will return \c Qt.ArrowCursor.

    \sa Qt::CursorShape, QQuickItem::cursor()
*/

QT_END_NAMESPACE

#include "moc_qquickhoverhandler_p.cpp"
