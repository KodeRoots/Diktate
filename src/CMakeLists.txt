add_executable(diktate)

ecm_add_qml_module(diktate
    URI
    org.koderoots.diktate
)

target_sources(diktate
    PRIVATE
    main.cpp
    audiorecorder.cpp
    whispertranscriber.cpp
    modelmanager.cpp
    audiofileprocessor.cpp
    vad.cpp
    cpuinfo.cpp
    gpuinfo.cpp

    audiorecorder.h
    whispertranscriber.h
    modelmanager.h
    audiofileprocessor.h
    vad.h
    cpuinfo.h
    gpuinfo.h
)

ecm_target_qml_sources(diktate
    SOURCES
    qml/Main.qml
)

if(NOT DIKTATE_SYSTEM_FVAD)
    target_include_directories(diktate
        PRIVATE
        ${fvad_SOURCE_DIR}/include
    )
endif()

target_link_libraries(diktate
    PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    Qt6::Network
    KF6::I18n
    KF6::CoreAddons
    KF6::IconThemes
    whisper
    fvad
)

if(DIKTATE_SYSTEM_WHISPER)
    target_link_libraries(diktate
        PRIVATE
        ${GGML_LIB}
        ${GGML_CPU_LIB}
        ${GGML_BASE_LIB}
        OpenMP::OpenMP_CXX
    )
endif()

install(TARGETS diktate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
