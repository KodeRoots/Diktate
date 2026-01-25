<div align="center">
  <img src="src/assets/io.github.denysmb.diktate.svg" width="128" height="128" alt="Diktate Icon"/>
  
  # Diktate
  
  **AI-powered speech-to-text application for KDE Plasma**
  
  A fast and private offline transcription tool using Whisper AI
  
  [![Download on Flathub](https://flathub.org/api/badge?svg)](https://flathub.org/apps/io.github.denysmb.diktate)
</div>

---

## About

Diktate is a native KDE application that provides accurate speech-to-text transcription using OpenAI's Whisper AI model. All processing happens locally on your device, ensuring your audio data remains completely private.

### Features

- **Offline transcription** - No internet connection required, your data never leaves your device
- **Multiple Whisper models** - Choose from tiny to large models based on your accuracy and speed needs
- **Easy model management** - Download and manage Whisper models directly from the app
- **GPU acceleration** - Optional CUDA and Vulkan support for faster transcription
- **Native KDE integration** - Built with Qt/QML and Kirigami for seamless Plasma desktop experience
- **Simple interface** - Record audio and get instant transcription with minimal clicks

## Installation

### Flathub (Recommended)

The easiest way to install Diktate is through Flathub:

[![Download on Flathub](https://flathub.org/api/badge?svg)](https://flathub.org/apps/io.github.denysmb.diktate)

Or via command line:
```bash
flatpak install flathub io.github.denysmb.diktate
```

## Build

### Prerequisites

- CMake 3.20 or higher
- Qt6 (Core, Quick, Gui, QuickControls2, Widgets, Multimedia, Concurrent)
- KDE Frameworks 6 (Kirigami, I18n, CoreAddons, QQC2DesktopStyle, IconThemes)
- C++17 compatible compiler
- Git

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://invent.kde.org/denysmb/Diktate.git
   cd Diktate
   ```

2. **Configure the build:**
   ```bash
   cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local
   ```

   Optional GPU acceleration flags:
   - `-DDIKTATE_CUDA=ON` - Enable CUDA support (NVIDIA GPUs)
   - `-DDIKTATE_VULKAN=ON` - Enable Vulkan support (broader GPU compatibility)
   - `-DDIKTATE_OPENBLAS=ON` - Enable OpenBLAS for optimized CPU inference

   Example with Vulkan:
   ```bash
   cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local -DDIKTATE_VULKAN=ON
   ```

3. **Build:**
   ```bash
   cmake --build build
   ```

4. **Install:**
   ```bash
   cmake --install build
   ```

5. **Update desktop database and icon cache:**
   ```bash
   update-desktop-database ~/.local/share/applications/
   gtk-update-icon-cache ~/.local/share/icons/hicolor/
   ```

6. **Run:**
   ```bash
   ~/.local/bin/diktate
   ```
   Or search for "Diktate" in your application launcher.

### System-wide Installation

For system-wide installation, omit the `CMAKE_INSTALL_PREFIX` and use sudo for install:
```bash
cmake -B build
cmake --build build
sudo cmake --install build
sudo update-desktop-database
sudo gtk-update-icon-cache /usr/share/icons/hicolor/
```

## Usage

1. Launch Diktate from your application menu
2. On first run, download a Whisper model (start with "base" for good balance of speed and accuracy)
3. Click the microphone button to start recording
4. Speak clearly into your microphone
5. Click stop when finished
6. View your transcription instantly

## License

This project is licensed under the GPL-3.0 License - see the LICENSE file for details.

## Credits

- Built with [whisper.cpp](https://github.com/ggerganov/whisper.cpp) - High-performance inference of OpenAI's Whisper
- Powered by [OpenAI Whisper](https://github.com/openai/whisper) - Robust speech recognition model
