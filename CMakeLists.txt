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
)

ecm_find_qmlmodule(org.kde.kirigami REQUIRED)

include(FetchContent)

# GPU acceleration options for whisper.cpp
option(DIKTATE_CUDA "Enable CUDA GPU acceleration (NVIDIA)" OFF)
option(DIKTATE_VULKAN "Enable Vulkan GPU acceleration" OFF)
# OpenBLAS is ON by default for significantly faster CPU inference (15-30% improvement)
# Like SpeechNote, we always try to use OpenBLAS when available
option(DIKTATE_OPENBLAS "Enable OpenBLAS for optimized CPU inference (requires libopenblas-dev)" ON)

# Configure whisper.cpp build options before fetching
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

# Disable -Werror for whisper.cpp build (it has some -Wundef warnings)
set(GGML_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)

# Save original compiler flags before modifying for whisper.cpp
set(ORIGINAL_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(ORIGINAL_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Remove -Werror=undef from compile flags for whisper.cpp compatibility
# KDE's strict compiler settings conflict with whisper.cpp's preprocessor conditionals
string(REPLACE "-Werror=undef" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-Werror=undef" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Enable exceptions for whisper.cpp (required by Vulkan shader generator)
# KDE disables exceptions by default with -fno-exceptions
string(REPLACE "-fno-exceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Aggressive optimization flags for whisper.cpp (like SpeechNote does)
# -O3: Maximum optimization level
# -ffast-math: Faster floating-point operations (safe for neural network inference)
# -fno-finite-math-only: Ensure NaN/Inf handling still works (required by some code paths)
set(WHISPER_OPTIMIZATION_FLAGS "-O3 -ffast-math -fno-finite-math-only")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WHISPER_OPTIMIZATION_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WHISPER_OPTIMIZATION_FLAGS}")
message(STATUS "Diktate: Using aggressive optimization flags for whisper.cpp: ${WHISPER_OPTIMIZATION_FLAGS}")

# Fetch whisper.cpp
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG        v1.7.3  # Updated version with Flash Attention and better performance
)

# Fetch WebRTC VAD for industrial-grade voice activity detection
# This is much more accurate than simple RMS threshold checking
FetchContent_Declare(
    webrtcvad
    GIT_REPOSITORY https://github.com/webrtc-mirror/webrtc.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  common_audio/vad  # Only need VAD component
)

# Since WebRTC is large and we only need VAD, we'll use a minimal standalone implementation
# Fetch libfvad - a standalone WebRTC VAD implementation
FetchContent_Declare(
    fvad
    GIT_REPOSITORY https://github.com/dpirch/libfvad.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(whisper fvad)
message(STATUS "Diktate: WebRTC VAD (libfvad) enabled for voice activity detection")

# Restore original compiler flags for our own code
set(CMAKE_C_FLAGS "${ORIGINAL_CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${ORIGINAL_CMAKE_CXX_FLAGS}")

add_subdirectory(src)

install(PROGRAMS org.koderoots.diktate.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES src/assets/org.koderoots.diktate.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
