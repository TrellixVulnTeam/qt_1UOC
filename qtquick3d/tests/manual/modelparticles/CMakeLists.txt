qt_internal_add_manual_test(manual_test_modelparticles
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(manual_test_modelparticles "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
