cmake_minimum_required(VERSION 3.20)
project(diktate)

find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(ECMQmlModule)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Quick
    Test
    Gui
    QuickControls2
    Widgets
    Multimedia
    Concurrent
)

find_package(KF6 REQUIRED COMPONENTS
    Kirigami
    I18n
    CoreAddons
    QQC2DesktopStyle
    IconThemes
    StatusNotifierItem
    Notifications
)

ecm_find_qmlmodule(org.kde.kirigami REQUIRED)

include(FetchContent)

# GPU acceleration options for whisper.cpp
option(DIKTATE_CUDA "Enable CUDA GPU acceleration (NVIDIA)" OFF)
option(DIKTATE_VULKAN "Enable Vulkan GPU acceleration" OFF)
option(DIKTATE_OPENBLAS "Enable OpenBLAS for optimized CPU inference (requires libopenblas-dev)" ON)

# Prepend /app to search paths so Flatpak-built dependencies are found first
list(APPEND CMAKE_PREFIX_PATH "/app")

# Try to find pre-installed whisper.cpp (e.g., from Flatpak module)
find_package(whisper QUIET)

if(whisper_FOUND)
    message(STATUS "Diktate: Using system whisper.cpp")
    set(DIKTATE_SYSTEM_WHISPER TRUE)
    find_library(GGML_LIB ggml)
    find_library(GGML_CPU_LIB ggml-cpu)
    find_library(GGML_BASE_LIB ggml-base)
else()
    set(DIKTATE_SYSTEM_WHISPER FALSE)

    if(DIKTATE_CUDA)
        set(GGML_CUDA ON CACHE BOOL "" FORCE)
        message(STATUS "Diktate: CUDA GPU acceleration enabled")
    endif()

    if(DIKTATE_VULKAN)
        set(GGML_VULKAN ON CACHE BOOL "" FORCE)
        message(STATUS "Diktate: Vulkan GPU acceleration enabled")
    endif()

    if(DIKTATE_OPENBLAS)
        find_package(BLAS QUIET)
        if(BLAS_FOUND)
            set(GGML_BLAS ON CACHE BOOL "" FORCE)
            set(GGML_BLAS_VENDOR "OpenBLAS" CACHE STRING "" FORCE)
            message(STATUS "Diktate: OpenBLAS CPU optimization enabled")
        else()
            message(STATUS "Diktate: OpenBLAS requested but BLAS not found, disabling")
            set(DIKTATE_OPENBLAS OFF)
        endif()
    else()
        set(GGML_BLAS OFF CACHE BOOL "" FORCE)
    endif()

    set(GGML_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)

    set(ORIGINAL_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(ORIGINAL_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    string(REPLACE "-Werror=undef" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "-Werror=undef" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-fno-exceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    set(WHISPER_OPTIMIZATION_FLAGS "-O3 -ffast-math -fno-finite-math-only")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WHISPER_OPTIMIZATION_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WHISPER_OPTIMIZATION_FLAGS}")
    message(STATUS "Diktate: Using aggressive optimization flags for whisper.cpp: ${WHISPER_OPTIMIZATION_FLAGS}")

    FetchContent_Declare(
        whisper
        GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp.git
        GIT_TAG        v1.7.3
    )

    FetchContent_MakeAvailable(whisper)

    set(CMAKE_C_FLAGS "${ORIGINAL_CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${ORIGINAL_CMAKE_CXX_FLAGS}")
endif()

# Try to find pre-installed libfvad (e.g., from Flatpak module)
find_library(FVAD_LIB fvad)
find_path(FVAD_INCLUDE_DIR fvad.h)

if(FVAD_LIB AND FVAD_INCLUDE_DIR)
    message(STATUS "Diktate: Using system libfvad")
    add_library(fvad STATIC IMPORTED)
    set_target_properties(fvad PROPERTIES
        IMPORTED_LOCATION ${FVAD_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${FVAD_INCLUDE_DIR}
    )
    set(DIKTATE_SYSTEM_FVAD TRUE)
else()
    FetchContent_Declare(
        fvad
        GIT_REPOSITORY https://github.com/dpirch/libfvad.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(fvad)
    set(DIKTATE_SYSTEM_FVAD FALSE)
endif()

message(STATUS "Diktate: WebRTC VAD (libfvad) enabled for voice activity detection")

add_subdirectory(src)

install(PROGRAMS org.koderoots.diktate.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES src/assets/org.koderoots.diktate.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
install(FILES src/assets/Diktate.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
