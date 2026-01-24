#ifndef GPUINFO_H
#define GPUINFO_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief GPU backend detection and information for whisper.cpp
 *
 * Detects available GPU backends at runtime by checking for:
 * - CUDA (NVIDIA GPUs)
 * - Vulkan (cross-platform GPU compute)
 * - OpenCL (legacy, broad support)
 * - Metal (macOS/iOS)
 *
 * Note: The actual backend used by whisper.cpp is determined at compile time.
 * This class helps users understand what's available and provides the
 * use_gpu toggle and gpu_device selection for multi-GPU systems.
 */
class GpuInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool gpuAvailable READ gpuAvailable CONSTANT)
    Q_PROPERTY(QStringList availableBackends READ availableBackends CONSTANT)
    Q_PROPERTY(int gpuCount READ gpuCount CONSTANT)
    Q_PROPERTY(QStringList gpuNames READ gpuNames CONSTANT)

public:
    enum class Backend {
        CPU,
        CUDA,
        Vulkan,
        OpenCL,
        Metal
    };
    Q_ENUM(Backend)

    explicit GpuInfo(QObject *parent = nullptr);

    /**
     * @brief Check if any GPU acceleration is available
     */
    bool gpuAvailable() const;

    /**
     * @brief Get list of available backend names
     */
    QStringList availableBackends() const;

    /**
     * @brief Get number of available GPUs
     */
    int gpuCount() const;

    /**
     * @brief Get names of available GPUs
     */
    QStringList gpuNames() const;

    /**
     * @brief Log GPU information to stderr
     */
    static void logInfo();

    /**
     * @brief Check if a specific backend was compiled in
     */
    Q_INVOKABLE bool hasBackend(Backend backend) const;

    /**
     * @brief Get recommended backend based on available hardware
     */
    Q_INVOKABLE QString recommendedBackend() const;

private:
    void detect();
    bool detectCuda();
    bool detectVulkan();
    bool detectOpenCL();
    bool detectMetal();

    bool m_hasCuda = false;
    bool m_hasVulkan = false;
    bool m_hasOpenCL = false;
    bool m_hasMetal = false;

    int m_gpuCount = 0;
    QStringList m_gpuNames;
};

#endif // GPUINFO_H
