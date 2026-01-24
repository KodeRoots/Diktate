#include "cpuinfo.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <thread>
#include <cstdio>

namespace CpuInfo {

// Static cache for CPU info
static Info s_cachedInfo;
static bool s_detected = false;

Info detect()
{
    if (s_detected) {
        return s_cachedInfo;
    }

    Info info;
    
    // Detect architecture at compile time
#if defined(__x86_64__) || defined(_M_X64)
    info.arch = Architecture::x86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    info.arch = Architecture::ARM64;
#elif defined(__arm__) || defined(_M_ARM)
    info.arch = Architecture::ARM32;
#else
    info.arch = Architecture::Unknown;
#endif

    // Get core count from hardware_concurrency
    info.coreCount = static_cast<int>(std::thread::hardware_concurrency());

    // Parse /proc/cpuinfo for features
    QFile cpuinfoFile(QStringLiteral("/proc/cpuinfo"));
    if (cpuinfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&cpuinfoFile);
        QString content = in.readAll();
        cpuinfoFile.close();

        // Extract model name
        QRegularExpression modelRx(QStringLiteral("model name\\s*:\\s*(.+)"));
        QRegularExpressionMatch modelMatch = modelRx.match(content);
        if (modelMatch.hasMatch()) {
            info.modelName = modelMatch.captured(1).trimmed();
        }

        // Extract flags/features line
        QRegularExpression flagsRx(QStringLiteral("(flags|Features)\\s*:\\s*(.+)"));
        QRegularExpressionMatch flagsMatch = flagsRx.match(content);
        if (flagsMatch.hasMatch()) {
            QString flags = flagsMatch.captured(2).toLower();

            // x86 features
            if (flags.contains(QStringLiteral("sse3"))) {
                info.features |= Feature::SSE3;
            }
            if (flags.contains(QStringLiteral("ssse3"))) {
                info.features |= Feature::SSSE3;
            }
            if (flags.contains(QStringLiteral("sse4_1"))) {
                info.features |= Feature::SSE4_1;
            }
            if (flags.contains(QStringLiteral("sse4_2"))) {
                info.features |= Feature::SSE4_2;
            }
            if (flags.contains(QStringLiteral(" avx "))) {
                info.features |= Feature::AVX;
            } else if (flags.contains(QStringLiteral("avx")) && 
                       !flags.contains(QStringLiteral("avx2")) &&
                       !flags.contains(QStringLiteral("avx512"))) {
                info.features |= Feature::AVX;
            }
            if (flags.contains(QStringLiteral("avx2"))) {
                info.features |= Feature::AVX2;
            }
            if (flags.contains(QStringLiteral("avx512f"))) {
                info.features |= Feature::AVX512F;
            }
            if (flags.contains(QStringLiteral("fma"))) {
                info.features |= Feature::FMA;
            }
            if (flags.contains(QStringLiteral("f16c"))) {
                info.features |= Feature::F16C;
            }

            // ARM features
            if (flags.contains(QStringLiteral("neon"))) {
                info.features |= Feature::NEON;
            }
            if (flags.contains(QStringLiteral("asimd"))) {
                info.features |= Feature::ASIMD;
            }
        }
    }

    s_cachedInfo = info;
    s_detected = true;
    return info;
}

QString featuresToString(unsigned int features)
{
    QStringList list;
    
    if (features & Feature::SSE3) list << QStringLiteral("SSE3");
    if (features & Feature::SSSE3) list << QStringLiteral("SSSE3");
    if (features & Feature::SSE4_1) list << QStringLiteral("SSE4.1");
    if (features & Feature::SSE4_2) list << QStringLiteral("SSE4.2");
    if (features & Feature::AVX) list << QStringLiteral("AVX");
    if (features & Feature::AVX2) list << QStringLiteral("AVX2");
    if (features & Feature::AVX512F) list << QStringLiteral("AVX-512");
    if (features & Feature::FMA) list << QStringLiteral("FMA");
    if (features & Feature::F16C) list << QStringLiteral("F16C");
    if (features & Feature::NEON) list << QStringLiteral("NEON");
    if (features & Feature::ASIMD) list << QStringLiteral("ASIMD");
    
    if (list.isEmpty()) {
        return QStringLiteral("none");
    }
    return list.join(QStringLiteral(", "));
}

QString archToString(Architecture arch)
{
    switch (arch) {
    case Architecture::x86_64:
        return QStringLiteral("x86_64");
    case Architecture::ARM32:
        return QStringLiteral("ARM32");
    case Architecture::ARM64:
        return QStringLiteral("ARM64");
    default:
        return QStringLiteral("unknown");
    }
}

void logInfo()
{
    Info info = detect();
    
    // Use fprintf for reliable output regardless of Qt logging config
    fprintf(stderr, "Diktate CPU Info:\n");
    fprintf(stderr, "  Architecture: %s\n", qPrintable(archToString(info.arch)));
    fprintf(stderr, "  Model: %s\n", qPrintable(info.modelName));
    fprintf(stderr, "  Cores: %d\n", info.coreCount);
    fprintf(stderr, "  SIMD Features: %s\n", qPrintable(featuresToString(info.features)));
    
    if (hasGoodSimdSupport()) {
        fprintf(stderr, "  Whisper.cpp: Good SIMD support detected (AVX2+FMA)\n");
    } else {
        fprintf(stderr, "  Whisper.cpp: Limited SIMD support - transcription may be slower\n");
    }
    fflush(stderr);
}

bool hasGoodSimdSupport()
{
    Info info = detect();
    
    if (info.arch == Architecture::x86_64) {
        // AVX2 + FMA is ideal for whisper.cpp on x86_64
        return (info.features & Feature::AVX2) && (info.features & Feature::FMA);
    } else if (info.arch == Architecture::ARM64) {
        // ASIMD (Advanced SIMD) is good for ARM64
        return (info.features & Feature::ASIMD) || (info.features & Feature::NEON);
    } else if (info.arch == Architecture::ARM32) {
        // NEON for ARM32
        return (info.features & Feature::NEON);
    }
    
    return false;
}

} // namespace CpuInfo
