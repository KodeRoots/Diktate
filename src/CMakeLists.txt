add_executable(diktate)

ecm_add_qml_module(diktate
    URI
    io.github.denysmb.diktate
)

target_sources(diktate
    PRIVATE
    main.cpp
    audiorecorder.cpp

    audiorecorder.h
)

ecm_target_qml_sources(diktate
    SOURCES
    qml/Main.qml
)

target_link_libraries(diktate
    PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Multimedia
    KF6::I18n
    KF6::CoreAddons
    KF6::IconThemes
)

install(TARGETS diktate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
