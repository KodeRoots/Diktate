#include "gpuinfo.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

#include <cstdio>

GpuInfo::GpuInfo(QObject *parent)
    : QObject(parent)
{
    detect();
}

void GpuInfo::detect()
{
    // Reset state
    m_hasCuda = false;
    m_hasVulkan = false;
    m_hasOpenCL = false;
    m_hasMetal = false;
    m_gpuCount = 0;
    m_gpuNames.clear();

    // Always try to detect hardware for informational purposes
    detectCuda();
    detectVulkan();

    // Check compile-time flags for actual backend availability
#ifdef GGML_USE_CUDA
    m_hasCuda = (m_gpuCount > 0);  // Only true if hardware exists
#endif

#ifdef GGML_USE_VULKAN
    m_hasVulkan = detectVulkan();
#endif

#ifdef GGML_USE_OPENCL
    m_hasOpenCL = detectOpenCL();
#endif

#ifdef GGML_USE_METAL
    m_hasMetal = detectMetal();
#endif
}

bool GpuInfo::detectCuda()
{
    // Check for NVIDIA GPU via /proc/driver/nvidia or nvidia-smi
    QFile nvidiaVersion(QStringLiteral("/proc/driver/nvidia/version"));
    if (nvidiaVersion.exists()) {
        // NVIDIA driver is loaded
        // Try to get GPU info via nvidia-smi
        QProcess nvidiaSmi;
        nvidiaSmi.start(QStringLiteral("nvidia-smi"),
                        QStringList() << QStringLiteral("--query-gpu=name,memory.total")
                                      << QStringLiteral("--format=csv,noheader"));
        if (nvidiaSmi.waitForFinished(5000)) {
            QString output = QString::fromUtf8(nvidiaSmi.readAllStandardOutput());
            QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                QStringList parts = line.split(QLatin1Char(','));
                if (!parts.isEmpty()) {
                    QString gpuName = parts.first().trimmed();
                    if (!gpuName.isEmpty()) {
                        m_gpuNames.append(gpuName);
                        m_gpuCount++;
                    }
                }
            }
        }
        return m_gpuCount > 0;
    }
    return false;
}

bool GpuInfo::detectVulkan()
{
    // Check for Vulkan via vulkaninfo or /dev/dri
    QDir driDir(QStringLiteral("/dev/dri"));
    if (!driDir.exists()) {
        return false;
    }

    // Try vulkaninfo for detailed GPU info
    QProcess vulkanInfo;
    vulkanInfo.start(QStringLiteral("vulkaninfo"), QStringList() << QStringLiteral("--summary"));
    if (vulkanInfo.waitForFinished(5000)) {
        QString output = QString::fromUtf8(vulkanInfo.readAllStandardOutput());

        // Parse GPU names from vulkaninfo output
        // Look for lines like "deviceName = AMD Radeon..."
        QRegularExpression deviceNameRe(QStringLiteral("deviceName\\s*=\\s*(.+)"));
        QRegularExpressionMatchIterator matches = deviceNameRe.globalMatch(output);

        while (matches.hasNext()) {
            QRegularExpressionMatch match = matches.next();
            QString gpuName = match.captured(1).trimmed();
            // Avoid duplicates and filter out software renderers
            if (!gpuName.isEmpty() && !m_gpuNames.contains(gpuName)) {
                // Skip software renderers like llvmpipe, lavapipe, swiftshader
                if (gpuName.contains(QStringLiteral("llvmpipe"), Qt::CaseInsensitive) ||
                    gpuName.contains(QStringLiteral("lavapipe"), Qt::CaseInsensitive) ||
                    gpuName.contains(QStringLiteral("swiftshader"), Qt::CaseInsensitive) ||
                    gpuName.contains(QStringLiteral("software"), Qt::CaseInsensitive)) {
                    continue;
                }
                m_gpuNames.append(gpuName);
                m_gpuCount++;
            }
        }

        return output.contains(QStringLiteral("Vulkan Instance"));
    }

    // Fallback: check if any render nodes exist
    QStringList renderNodes = driDir.entryList(QStringList() << QStringLiteral("renderD*"), QDir::System);
    return !renderNodes.isEmpty();
}

bool GpuInfo::detectOpenCL()
{
    // Check for OpenCL via clinfo
    QProcess clInfo;
    clInfo.start(QStringLiteral("clinfo"), QStringList() << QStringLiteral("--list"));
    if (clInfo.waitForFinished(5000)) {
        QString output = QString::fromUtf8(clInfo.readAllStandardOutput());
        return output.contains(QStringLiteral("Device"));
    }
    return false;
}

bool GpuInfo::detectMetal()
{
#ifdef Q_OS_MACOS
    // On macOS, Metal is always available on supported hardware
    // Check for Apple GPU
    QProcess systemProfiler;
    systemProfiler.start(QStringLiteral("system_profiler"), QStringList() << QStringLiteral("SPDisplaysDataType"));
    if (systemProfiler.waitForFinished(5000)) {
        QString output = QString::fromUtf8(systemProfiler.readAllStandardOutput());
        return output.contains(QStringLiteral("Metal"));
    }
#endif
    return false;
}

bool GpuInfo::gpuAvailable() const
{
    return m_hasCuda || m_hasVulkan || m_hasOpenCL || m_hasMetal;
}

QStringList GpuInfo::availableBackends() const
{
    QStringList backends;
    backends.append(QStringLiteral("CPU"));

    if (m_hasCuda) {
        backends.append(QStringLiteral("CUDA"));
    }
    if (m_hasVulkan) {
        backends.append(QStringLiteral("Vulkan"));
    }
    if (m_hasOpenCL) {
        backends.append(QStringLiteral("OpenCL"));
    }
    if (m_hasMetal) {
        backends.append(QStringLiteral("Metal"));
    }

    return backends;
}

int GpuInfo::gpuCount() const
{
    return m_gpuCount;
}

QStringList GpuInfo::gpuNames() const
{
    return m_gpuNames;
}

bool GpuInfo::hasBackend(Backend backend) const
{
    switch (backend) {
    case Backend::CPU:
        return true;
    case Backend::CUDA:
        return m_hasCuda;
    case Backend::Vulkan:
        return m_hasVulkan;
    case Backend::OpenCL:
        return m_hasOpenCL;
    case Backend::Metal:
        return m_hasMetal;
    }
    return false;
}

QString GpuInfo::recommendedBackend() const
{
    // Priority: CUDA > Metal > Vulkan > OpenCL > CPU
    if (m_hasCuda) {
        return QStringLiteral("CUDA");
    }
    if (m_hasMetal) {
        return QStringLiteral("Metal");
    }
    if (m_hasVulkan) {
        return QStringLiteral("Vulkan");
    }
    if (m_hasOpenCL) {
        return QStringLiteral("OpenCL");
    }
    return QStringLiteral("CPU");
}

void GpuInfo::logInfo()
{
    GpuInfo info;

    fprintf(stderr, "\nDiktate GPU Info:\n");

    if (info.m_gpuCount > 0) {
        fprintf(stderr, "  GPUs detected: %d\n", info.m_gpuCount);
        for (int i = 0; i < info.m_gpuNames.size(); ++i) {
            fprintf(stderr, "    [%d] %s\n", i, info.m_gpuNames.at(i).toUtf8().constData());
        }
    } else {
        fprintf(stderr, "  GPUs detected: None (or detection failed)\n");
    }

    fprintf(stderr, "  Compiled backends:");
#ifdef GGML_USE_CUDA
    fprintf(stderr, " CUDA");
#endif
#ifdef GGML_USE_VULKAN
    fprintf(stderr, " Vulkan");
#endif
#ifdef GGML_USE_OPENCL
    fprintf(stderr, " OpenCL");
#endif
#ifdef GGML_USE_METAL
    fprintf(stderr, " Metal");
#endif
    fprintf(stderr, " CPU (always)\n");

    fprintf(stderr, "  Available at runtime: %s\n",
            info.availableBackends().join(QStringLiteral(", ")).toUtf8().constData());
    fprintf(stderr, "  Recommended: %s\n", info.recommendedBackend().toUtf8().constData());
}
