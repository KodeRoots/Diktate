#ifndef CPUINFO_H
#define CPUINFO_H

#include <QString>
#include <QStringList>

/**
 * CPU feature detection utility
 * 
 * Detects available CPU SIMD features that can accelerate
 * whisper.cpp inference. Logs detected features for debugging.
 */
namespace CpuInfo {

/**
 * CPU feature flags relevant for whisper.cpp acceleration
 */
enum Feature : unsigned int {
    None = 0,
    SSE3 = 1 << 0,
    SSSE3 = 1 << 1,
    SSE4_1 = 1 << 2,
    SSE4_2 = 1 << 3,
    AVX = 1 << 4,
    AVX2 = 1 << 5,
    AVX512F = 1 << 6,
    FMA = 1 << 7,
    F16C = 1 << 8,
    // ARM features
    NEON = 1 << 9,
    ASIMD = 1 << 10,
};

/**
 * CPU architecture type
 */
enum class Architecture {
    Unknown,
    x86_64,
    ARM32,
    ARM64
};

/**
 * CPU information structure
 */
struct Info {
    Architecture arch = Architecture::Unknown;
    unsigned int features = Feature::None;
    int coreCount = 0;
    QString modelName;
};

/**
 * Detect CPU features by reading /proc/cpuinfo
 * Results are cached after first call.
 */
Info detect();

/**
 * Get a human-readable string of detected features
 */
QString featuresToString(unsigned int features);

/**
 * Get architecture name as string
 */
QString archToString(Architecture arch);

/**
 * Log detected CPU info to qDebug
 */
void logInfo();

/**
 * Check if specific feature is available
 */
inline bool hasFeature(unsigned int features, Feature feature) {
    return (features & feature) != 0;
}

/**
 * Check if CPU has good SIMD support for whisper.cpp
 * (AVX2+FMA on x86_64, NEON/ASIMD on ARM)
 */
bool hasGoodSimdSupport();

} // namespace CpuInfo

#endif // CPUINFO_H
