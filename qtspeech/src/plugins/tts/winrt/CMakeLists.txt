qt_internal_add_plugin(QTextToSpeechWinRTPlugin
    OUTPUT_NAME qtexttospeech_winrt
    PLUGIN_TYPE texttospeech
    SOURCES
        qtexttospeech_winrt.cpp qtexttospeech_winrt.h
        qtexttospeech_winrt_audiosource.cpp qtexttospeech_winrt_audiosource.h
        qtexttospeech_winrt_plugin.cpp qtexttospeech_winrt_plugin.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::TextToSpeech
        Qt::Multimedia
        shlwapi
        runtimeobject
)

qt_internal_extend_target(QTextToSpeechWinRTPlugin CONDITION MSVC
    COMPILE_OPTIONS
        /Zc:twoPhase-
)
