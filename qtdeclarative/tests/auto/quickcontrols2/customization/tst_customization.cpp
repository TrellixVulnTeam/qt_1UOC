// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtCore/private/qhooks_p.h>
#include <QtCore/qregularexpression.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

using namespace QQuickControlsTestUtils;

struct ControlInfo
{
    QString type;
    QStringList delegates;
};

static const ControlInfo ControlInfos[] = {
    { "AbstractButton", QStringList() << "background" << "contentItem" << "indicator" },
    { "ApplicationWindow", QStringList() << "background" },
    { "BusyIndicator", QStringList() << "background" << "contentItem" },
    { "Button", QStringList() << "background" << "contentItem" },
    { "CheckBox", QStringList() << "contentItem" << "indicator" },
    { "CheckDelegate", QStringList() << "background" << "contentItem" << "indicator" },
    { "ComboBox", QStringList() << "background" << "contentItem" << "indicator" }, // popup not created until needed
    { "Container", QStringList() << "background" << "contentItem" },
    { "Control", QStringList() << "background" << "contentItem" },
    { "DelayButton", QStringList() << "background" << "contentItem" },
    { "Dial", QStringList() << "background" << "handle" },
    { "Dialog", QStringList() << "background" << "contentItem" },
    { "DialogButtonBox", QStringList() << "background" << "contentItem" },
    { "Drawer", QStringList() << "background" << "contentItem" },
    { "Frame", QStringList() << "background" << "contentItem" },
    { "GroupBox", QStringList() << "background" << "contentItem" << "label" },
    { "ItemDelegate", QStringList() << "background" << "contentItem" },
    { "Label", QStringList() << "background" },
    { "Menu", QStringList() << "background" << "contentItem" },
    { "MenuBar", QStringList() << "background" << "contentItem" },
    { "MenuBarItem", QStringList() << "background" << "contentItem" },
    { "MenuItem", QStringList() << "arrow" << "background" << "contentItem" << "indicator" },
    { "MenuSeparator", QStringList() << "background" << "contentItem" },
    { "Page", QStringList() << "background" << "contentItem" },
    { "PageIndicator", QStringList() << "background" << "contentItem" },
    { "Pane", QStringList() << "background" << "contentItem" },
    { "Popup", QStringList() << "background" << "contentItem" },
    { "ProgressBar", QStringList() << "background" << "contentItem" },
    { "RadioButton", QStringList() << "contentItem" << "indicator" },
    { "RadioDelegate", QStringList() << "background" << "contentItem" << "indicator" },
    { "RangeSlider", QStringList() << "background" << "first.handle" << "second.handle" },
    { "RoundButton", QStringList() << "background" << "contentItem" },
    { "ScrollBar", QStringList() << "background" << "contentItem" },
    { "ScrollIndicator", QStringList() << "background" << "contentItem" },
    { "ScrollView", QStringList() << "background" },
    { "Slider", QStringList() << "background" << "handle" },
    { "SpinBox", QStringList() << "background" << "contentItem" << "up.indicator" << "down.indicator" },
    { "StackView", QStringList() << "background" << "contentItem" },
    { "SwipeDelegate", QStringList() << "background" << "contentItem" },
    { "SwipeView", QStringList() << "background" << "contentItem" },
    { "Switch", QStringList() << "contentItem" << "indicator" },
    { "SwitchDelegate", QStringList() << "background" << "contentItem" << "indicator" },
    { "TabBar", QStringList() << "background" << "contentItem" },
    { "TabButton", QStringList() << "background" << "contentItem" },
    { "TextField", QStringList() << "background" },
    { "TextArea", QStringList() << "background" },
    { "ToolBar", QStringList() << "background" << "contentItem" },
    { "ToolButton", QStringList() << "background" << "contentItem" },
    { "ToolSeparator", QStringList() << "background" << "contentItem" },
    { "ToolTip", QStringList() << "background" << "contentItem" },
    { "Tumbler", QStringList() << "background" << "contentItem" }
};

class tst_customization : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_customization();

private slots:
    void initTestCase() override;
    void cleanupTestCase();

    void init() override;
    void cleanup();

    void creation_data();
    void creation();

    void override_data();
    void override();

    void comboPopup();

private:
    void reset();
    void addHooks();
    void removeHooks();

    QObject* createControl(const QString &type, const QString &qml, QString *error);

    QQmlEngine *engine = nullptr;
};

typedef QHash<QObject *, QString> QObjectNameHash;
Q_GLOBAL_STATIC(QObjectNameHash, qt_objectNames)
Q_GLOBAL_STATIC(QStringList, qt_createdQObjects)
Q_GLOBAL_STATIC(QStringList, qt_destroyedQObjects)
Q_GLOBAL_STATIC(QStringList, qt_destroyedParentQObjects)
static int qt_unparentedItemCount = 0;

class ItemParentListener : public QQuickItem
{
    Q_OBJECT

public:
    ItemParentListener()
    {
        m_slotIndex = metaObject()->indexOfSlot("onParentChanged()");
        m_signalIndex = QMetaObjectPrivate::signalIndex(QMetaMethod::fromSignal(&QQuickItem::parentChanged));
    }

    int signalIndex() const { return m_signalIndex; }
    int slotIndex() const { return m_slotIndex; }

public slots:
    void onParentChanged()
    {
        const QQuickItem *item = qobject_cast<QQuickItem *>(sender());
        if (!item)
            return;

        if (!item->parentItem())
            ++qt_unparentedItemCount;
    }

private:
    int m_slotIndex;
    int m_signalIndex;
};
static ItemParentListener *qt_itemParentListener = nullptr;

extern "C" Q_DECL_EXPORT void qt_addQObject(QObject *object)
{
    // objectName is not set at construction time
    QObject::connect(object, &QObject::objectNameChanged, [object](const QString &objectName) {
        QString oldObjectName = qt_objectNames()->value(object);
        if (!oldObjectName.isEmpty())
            qt_createdQObjects()->removeOne(oldObjectName);
        // Only track object names from our QML files,
        // not e.g. contentItem object names (like "ApplicationWindow").
        if (objectName.contains("-")) {
            qt_createdQObjects()->append(objectName);
            qt_objectNames()->insert(object, objectName);
        }
    });

    if (qt_itemParentListener) {
        static const int signalIndex = qt_itemParentListener->signalIndex();
        static const int slotIndex = qt_itemParentListener->slotIndex();
        QMetaObject::connect(object, signalIndex, qt_itemParentListener, slotIndex);
    }
}

extern "C" Q_DECL_EXPORT void qt_removeQObject(QObject *object)
{
    QString objectName = object->objectName();
    if (!objectName.isEmpty())
        qt_destroyedQObjects()->append(objectName);
    qt_objectNames()->remove(object);

    QObject *parent = object->parent();
    if (parent) {
        QString parentName = parent->objectName();
        if (!parentName.isEmpty())
            qt_destroyedParentQObjects()->append(parentName);
    }
}

// We don't want to fail on warnings until QTBUG-98964 is fixed,
// as we deliberately prevent deferred execution in some of the tests here,
// which causes warnings.
tst_customization::tst_customization()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings)
{
}

void tst_customization::initTestCase()
{
    QQmlDataTest::initTestCase();

    qt_itemParentListener = new ItemParentListener;
}

void tst_customization::cleanupTestCase()
{
    delete qt_itemParentListener;
    qt_itemParentListener = nullptr;
}

void tst_customization::init()
{
    QQmlDataTest::init();

    engine = new QQmlEngine(this);
    engine->addImportPath(testFile("styles"));

    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&qt_addQObject);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&qt_removeQObject);
}

void tst_customization::cleanup()
{
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;

    delete engine;
    engine = nullptr;

    qmlClearTypeRegistrations();

    reset();
}

void tst_customization::reset()
{
    qt_unparentedItemCount = 0;
    qt_createdQObjects()->clear();
    qt_destroyedQObjects()->clear();
    qt_destroyedParentQObjects()->clear();
}

QObject* tst_customization::createControl(const QString &name, const QString &qml, QString *error)
{
    QQmlComponent component(engine);
    component.setData("import QtQuick; import QtQuick.Window; import QtQuick.Controls; " + name.toUtf8() + " { " + qml.toUtf8() + " }", QUrl());
    QObject *obj = component.create();
    if (!obj)
        *error = component.errorString();
    return obj;
}

void tst_customization::creation_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QStringList>("delegates");

    // the "empty" style does not contain any delegates
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("empty:" + control.type)) << "empty" << control.type << QStringList();

    // the "incomplete" style is missing bindings to the delegates (must be created regardless)
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("incomplete:" + control.type)) << "incomplete" << control.type << control.delegates;

    // the "identified" style has IDs in the delegates (prevents deferred execution)
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("identified:" + control.type)) << "identified" << control.type << control.delegates;

    // the "simple" style simulates a proper style and contains bindings to/in delegates
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("simple:" + control.type)) << "simple" << control.type << control.delegates;

    // the "override" style overrides all delegates in the "simple" style
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("override:" + control.type)) << "override" << control.type << control.delegates;
}

void tst_customization::creation()
{
    QFETCH(QString, style);
    QFETCH(QString, type);
    QFETCH(QStringList, delegates);

    QQuickStyle::setStyle(style);

    QString error;
    QScopedPointer<QObject> control(createControl(type, "", &error));
    QVERIFY2(control, qPrintable(error));

    QByteArray templateType = "QQuick" + type.toUtf8();
    QVERIFY2(control->inherits(templateType), qPrintable(type + " does not inherit " + templateType + " (" + control->metaObject()->className() + ")"));

    // <control>-<style>
    QString controlName = type.toLower() + "-" + style;
    QCOMPARE(control->objectName(), controlName);
    QVERIFY2(qt_createdQObjects()->removeOne(controlName), qPrintable(controlName + " was not created as expected"));

    for (QString delegate : qAsConst(delegates)) {
        QStringList properties = delegate.split(".", Qt::SkipEmptyParts);

        // <control>-<delegate>-<style>(-<override>)
        delegate.append("-" + style);
        delegate.prepend(type.toLower() + "-");

        QVERIFY2(qt_createdQObjects()->removeOne(delegate), qPrintable(delegate + " was not created as expected"));

        // verify that the delegate instance has the expected object name
        // in case of grouped properties, we must query the properties step by step
        QObject *instance = control.data();
        while (!properties.isEmpty()) {
            QString property = properties.takeFirst();
            instance = instance->property(property.toUtf8()).value<QObject *>();
            QVERIFY2(instance, qPrintable("property was null: " + property));
        }
        QCOMPARE(instance->objectName(), delegate);
    }

    QEXPECT_FAIL("identified:ComboBox", "ComboBox::popup with an ID is created at construction time", Continue);

    QVERIFY2(qt_createdQObjects()->isEmpty(), qPrintable("unexpectedly created: " + qt_createdQObjects->join(", ")));
    QVERIFY2(qt_destroyedQObjects()->isEmpty(), qPrintable("unexpectedly destroyed: " + qt_destroyedQObjects->join(", ") + " were unexpectedly destroyed"));

    QVERIFY2(qt_destroyedParentQObjects()->isEmpty(), qPrintable("delegates/children of: " + qt_destroyedParentQObjects->join(", ") + " were unexpectedly destroyed"));
}

void tst_customization::override_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QStringList>("delegates");
    QTest::addColumn<QString>("nonDeferred");
    QTest::addColumn<bool>("identify");

    // NOTE: delegates with IDs prevent deferred execution

    // default delegates with IDs, override with custom delegates with no IDs
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("identified:" + control.type)) << "identified" << control.type << control.delegates << "identified" << false;

    // default delegates with no IDs, override with custom delegates with IDs
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("simple:" + control.type)) << "simple" << control.type << control.delegates << "" << true;

    // default delegates with IDs, override with custom delegates with IDs
    for (const ControlInfo &control : ControlInfos)
        QTest::newRow(qPrintable("overidentified:" + control.type)) << "identified" << control.type << control.delegates << "identified" << true;

#ifndef Q_OS_MACOS // QTBUG-65671

    // test that the built-in styles don't have undesired IDs in their delegates
    const QStringList styles = QQuickStylePrivate::builtInStyles();
    for (const QString &style : styles) {
        for (const ControlInfo &control : ControlInfos)
            QTest::newRow(qPrintable(style + ":" + control.type)) << style << control.type << control.delegates << "" << false;
    }

#endif
}

void tst_customization::override()
{
    QFETCH(QString, style);
    QFETCH(QString, type);
    QFETCH(QStringList, delegates);
    QFETCH(QString, nonDeferred);
    QFETCH(bool, identify);

    QQuickStyle::setStyle(style);

    QString qml;
    qml += QString("objectName: '%1-%2-override'; ").arg(type.toLower()).arg(style);
    for (const QString &delegate : delegates) {
        QString id = identify ? QString("id: %1;").arg(delegate) : QString();
        qml += QString("%1: Item { %2 objectName: '%3-%1-%4-override' } ").arg(delegate).arg(id.replace(".", "")).arg(type.toLower()).arg(style);
    }

    QString error;
    QScopedPointer<QObject> control(createControl(type, qml, &error));
    QVERIFY2(control, qPrintable(error));

    // If there are no intentional IDs in the default delegates nor in the overridden custom
    // delegates, no item should get un-parented during the creation process. An item being
    // unparented means that a delegate got destroyed, so there must be an internal ID in one
    // of the delegates in the tested style.
    if (!identify && nonDeferred.isEmpty()) {
        QEXPECT_FAIL("Universal:ApplicationWindow", "ApplicationWindow.qml contains an intentionally unparented FocusRectangle", Continue);
        QCOMPARE(qt_unparentedItemCount, 0);
    }

    // <control>-<style>-override
    QString controlName = type.toLower() + "-" + style + "-override";
    QCOMPARE(control->objectName(), controlName);
    QVERIFY2(qt_createdQObjects()->removeOne(controlName), qPrintable(controlName + " was not created as expected"));

    for (QString delegate : qAsConst(delegates)) {
        QStringList properties = delegate.split(".", Qt::SkipEmptyParts);

        // <control>-<delegate>-<style>(-override)
        delegate.append("-" + style);
        delegate.prepend(type.toLower() + "-");

        if (!nonDeferred.isEmpty())
            QVERIFY2(qt_createdQObjects()->removeOne(delegate), qPrintable(delegate + " was not created as expected"));

        delegate.append("-override");
        QVERIFY2(qt_createdQObjects()->removeOne(delegate), qPrintable(delegate + " was not created as expected"));

        // verify that the delegate instance has the expected object name
        // in case of grouped properties, we must query the properties step by step
        QObject *instance = control.data();
        while (!properties.isEmpty()) {
            QString property = properties.takeFirst();
            instance = instance->property(property.toUtf8()).value<QObject *>();
            QVERIFY2(instance, qPrintable("property was null: " + property));
        }
        QCOMPARE(instance->objectName(), delegate);
    }

    QEXPECT_FAIL("identified:ComboBox", "ComboBox::popup with an ID is created at construction time", Continue);
    QEXPECT_FAIL("overidentified:ComboBox", "ComboBox::popup with an ID is created at construction time", Continue);
    QVERIFY2(qt_createdQObjects()->isEmpty(), qPrintable("unexpectedly created: " + qt_createdQObjects->join(", ")));

    if (!nonDeferred.isEmpty()) {
        // There were items for which deferred execution was not possible.
        for (QString delegateName : qAsConst(delegates)) {
            if (!delegateName.contains("-"))
                delegateName.append("-" + nonDeferred);
            delegateName.prepend(type.toLower() + "-");

            const int delegateIndex = qt_destroyedQObjects()->indexOf(delegateName);
            QVERIFY2(delegateIndex == -1, qPrintable(delegateName + " was unexpectedly destroyed"));

            const auto controlChildren = control->children();
            const auto childIt = std::find_if(controlChildren.constBegin(), controlChildren.constEnd(), [delegateName](const QObject *child) {
                return child->objectName() == delegateName;
            });
            // We test other delegates (like the background) here, so make sure we don't end up with XPASSes by using the wrong delegate.
            if (delegateName.contains(QLatin1String("handle"))) {
                QEXPECT_FAIL("identified:RangeSlider", "For some reason, items that are belong to grouped properties fail here", Abort);
                QEXPECT_FAIL("overidentified:RangeSlider", "For some reason, items that are belong to grouped properties fail here", Abort);
            }
            if (delegateName.contains(QLatin1String("indicator"))) {
                QEXPECT_FAIL("identified:SpinBox", "For some reason, items that are belong to grouped properties fail here", Abort);
                QEXPECT_FAIL("overidentified:SpinBox", "For some reason, items that are belong to grouped properties fail here", Abort);
            }
            QVERIFY2(childIt != controlChildren.constEnd(), qPrintable(QString::fromLatin1(
                "Expected delegate \"%1\" to still be a QObject child of \"%2\"").arg(delegateName).arg(controlName)));

            const auto *delegate = qobject_cast<QQuickItem*>(*childIt);
            // Ensure that the item is hidden, etc.
            QVERIFY(delegate);
            QCOMPARE(delegate->isVisible(), false);
            QCOMPARE(delegate->parentItem(), nullptr);
        }
    }

    QVERIFY2(qt_destroyedQObjects()->isEmpty(), qPrintable("unexpectedly destroyed: " + qt_destroyedQObjects->join(", ")));
}

void tst_customization::comboPopup()
{
    QQuickStyle::setStyle("simple");

    {
        // test that ComboBox::popup is created when accessed
        QQmlComponent component(engine);
        component.setData("import QtQuick.Controls; ComboBox { }", QUrl());
        QScopedPointer<QQuickItem> comboBox(qobject_cast<QQuickItem *>(component.create()));
        QVERIFY(comboBox);

        QVERIFY(!qt_createdQObjects()->contains("combobox-popup-simple"));

        QObject *popup = comboBox->property("popup").value<QObject *>();
        QVERIFY(popup);
        QVERIFY(qt_createdQObjects()->contains("combobox-popup-simple"));
    }

    reset();

    {
        // test that ComboBox::popup is created when it becomes visible
        QQuickWindow window;
        window.resize(300, 300);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));

        QQmlComponent component(engine);
        component.setData("import QtQuick.Controls; ComboBox { }", QUrl());
        QScopedPointer<QQuickItem> comboBox(qobject_cast<QQuickItem *>(component.create()));
        QVERIFY(comboBox);

        comboBox->setParentItem(window.contentItem());
        QVERIFY(!qt_createdQObjects()->contains("combobox-popup-simple"));

        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
        QVERIFY(qt_createdQObjects()->contains("combobox-popup-simple"));
    }

    reset();

    {
        // test that ComboBox::popup is completed upon component completion (if appropriate)
        QQmlComponent component(engine);
        component.setData("import QtQuick; import QtQuick.Controls; ComboBox { id: control; contentItem: Item { visible: !control.popup.visible } popup: Popup { property bool wasCompleted: false; Component.onCompleted: wasCompleted = true } }", QUrl());
        QScopedPointer<QQuickItem> comboBox(qobject_cast<QQuickItem *>(component.create()));
        QVERIFY(comboBox);

        QObject *popup = comboBox->property("popup").value<QObject *>();
        QVERIFY(popup);
        QCOMPARE(popup->property("wasCompleted"), QVariant(true));
    }
}

QTEST_MAIN(tst_customization)

#include "tst_customization.moc"
