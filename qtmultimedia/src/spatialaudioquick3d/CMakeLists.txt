#####################################################################
## Quick3D.Sound Module:
#####################################################################

qt_internal_add_qml_module(Quick3DSpatialAudioPrivate
    URI "QtQuick3D.SpatialAudio"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QQuick3DAudioModule
    PLUGIN_TARGET quick3dspatialaudio
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    DEPENDENCIES QtQuick QtQuick3DPrivate QtMultimedia
    CONFIG_MODULE_NAME quick3dspatialaudio
    INTERNAL_MODULE
    SOURCES
        qquick3daudiolistener.cpp qquick3daudiolistener_p.h
        qquick3daudioroom.cpp qquick3daudioroom_p.h
        qquick3dspatialsound.cpp qquick3dspatialsound_p.h
        qquick3dambientsound.cpp qquick3dambientsound_p.h
        qquick3daudioengine.cpp qquick3daudioengine_p.h
        qtquick3daudioglobal_p.h
        qtquick3daudiotypes_p.h
    QML_FILES
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::SpatialAudio
    GENERATE_CPP_EXPORTS
)

target_sources(quick3dspatialaudio PRIVATE qquick3dspatialaudio_plugin.cpp)
