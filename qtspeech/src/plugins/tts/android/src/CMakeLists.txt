qt_internal_add_plugin(QTextToSpeechEngineAndroid
    OUTPUT_NAME qttexttospeech_android
    PLUGIN_TYPE texttospeech
    SOURCES
        qtexttospeech_android.cpp qtexttospeech_android.h
        qtexttospeech_android_plugin.cpp qtexttospeech_android_plugin.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::TextToSpeech
)

add_dependencies(QTextToSpeechEngineAndroid QtAndroidTextToSpeech)
