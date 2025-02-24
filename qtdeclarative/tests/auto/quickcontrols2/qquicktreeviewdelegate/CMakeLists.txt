#####################################################################
## tst_qquicktreeviewdelegate Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquicktreeviewdelegate
    SOURCES
        testmodel.h testmodel.cpp
        tst_qquicktreeviewdelegate.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickTemplates2Private
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qquicktreeviewdelegate CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquicktreeviewdelegate CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
