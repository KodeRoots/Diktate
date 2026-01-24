#include "vad.h"
#include <QByteArray>
#include <QDebug>
#include <cstring>

// libfvad C API (standalone WebRTC VAD implementation)
#include <fvad.h>

VAD::VAD(int sampleRate, Mode mode)
    : m_sampleRate(sampleRate)
    , m_mode(mode)
{
    // Calculate frame size (10ms at given sample rate)
    // At 16kHz: 10ms = 160 samples
    // At 8kHz:  10ms = 80 samples
    m_frameSize = (sampleRate * FRAME_SIZE_MS) / 1000;
    
    m_handle = fvad_new();
    if (!m_handle) {
        qWarning() << "VAD: Failed to create libfvad instance";
        return;
    }
    
    // Set sample rate
    if (fvad_set_sample_rate(static_cast<Fvad*>(m_handle), sampleRate) != 0) {
        qWarning() << "VAD: Invalid sample rate" << sampleRate;
        fvad_free(static_cast<Fvad*>(m_handle));
        m_handle = nullptr;
        return;
    }
    
    setMode(mode);
    
    // Pre-allocate buffer for efficiency
    m_inputBuffer.resize(m_frameSize * 4);
    m_bufferUsed = 0;
    
    qDebug() << "VAD: Initialized with sample rate" << sampleRate 
             << "Hz, frame size" << m_frameSize << "samples (" << FRAME_SIZE_MS << "ms)";
}

VAD::~VAD()
{
    if (m_handle) {
        fvad_free(static_cast<Fvad*>(m_handle));
        m_handle = nullptr;
    }
}

void VAD::reset()
{
    if (m_handle) {
        fvad_reset(static_cast<Fvad*>(m_handle));
    }
    m_bufferUsed = 0;
    m_consecutiveSpeechFrames = 0;
    m_consecutiveSilenceFrames = 0;
    m_inSpeech = false;
}

void VAD::setMode(Mode mode)
{
    m_mode = mode;
    if (m_handle) {
        if (fvad_set_mode(static_cast<Fvad*>(m_handle), static_cast<int>(mode)) != 0) {
            qWarning() << "VAD: Failed to set mode" << static_cast<int>(mode);
        }
    }
}

bool VAD::processFrame(const int16_t* frame)
{
    if (!m_handle) {
        return true;  // If VAD not available, assume speech
    }
    
    // libfvad expects specific frame lengths:
    // At 16kHz: 160 (10ms), 320 (20ms), or 480 (30ms) samples
    // We pass m_frameSize which is 160 at 16kHz (10ms frames)
    int result = fvad_process(
        static_cast<Fvad*>(m_handle),
        frame,
        static_cast<size_t>(m_frameSize)  // Ensure correct type
    );
    
    if (result < 0) {
        // This should not happen if m_frameSize is correctly calculated
        // Debug: print actual values to diagnose
        static int errorCount = 0;
        if (errorCount < 5) {
            qWarning() << "VAD: Processing error - frame size" << m_frameSize 
                       << "sample rate" << m_sampleRate
                       << "expected" << (m_sampleRate * FRAME_SIZE_MS / 1000);
            errorCount++;
        }
        return true;  // Assume speech on error
    }
    
    return result == 1;
}

bool VAD::isSpeech(const int16_t* samples, size_t sampleCount)
{
    if (!m_handle) {
        return true;  // If VAD not available, assume speech (fallback to RMS)
    }
    
    if (sampleCount == 0) {
        return m_inSpeech;  // No new data, return current state
    }
    
    // Ensure buffer has enough space
    if (m_bufferUsed + sampleCount > m_inputBuffer.size()) {
        m_inputBuffer.resize(m_bufferUsed + sampleCount + m_frameSize);
    }
    
    // Append incoming samples to buffer
    std::memcpy(m_inputBuffer.data() + m_bufferUsed, samples, sampleCount * sizeof(int16_t));
    m_bufferUsed += sampleCount;
    
    // Process all complete frames
    bool anySpeechDetected = false;
    size_t processedSamples = 0;
    
    while (processedSamples + m_frameSize <= m_bufferUsed) {
        bool frameIsSpeech = processFrame(m_inputBuffer.data() + processedSamples);
        
        if (frameIsSpeech) {
            anySpeechDetected = true;
            m_consecutiveSpeechFrames++;
            m_consecutiveSilenceFrames = 0;
        } else {
            m_consecutiveSilenceFrames++;
            m_consecutiveSpeechFrames = 0;
        }
        
        processedSamples += m_frameSize;
    }
    
    // Move remaining unprocessed samples to the beginning of the buffer
    if (processedSamples > 0 && processedSamples < m_bufferUsed) {
        size_t remaining = m_bufferUsed - processedSamples;
        std::memmove(m_inputBuffer.data(), m_inputBuffer.data() + processedSamples, remaining * sizeof(int16_t));
        m_bufferUsed = remaining;
    } else if (processedSamples >= m_bufferUsed) {
        m_bufferUsed = 0;
    }
    
    // State machine with hysteresis:
    // - Need SPEECH_FRAMES_THRESHOLD consecutive speech frames to enter speech state
    // - Need SILENCE_FRAMES_THRESHOLD consecutive silence frames to leave speech state
    if (!m_inSpeech && m_consecutiveSpeechFrames >= SPEECH_FRAMES_THRESHOLD) {
        m_inSpeech = true;
        qDebug() << "VAD: Speech started";
    } else if (m_inSpeech && m_consecutiveSilenceFrames >= SILENCE_FRAMES_THRESHOLD) {
        m_inSpeech = false;
        qDebug() << "VAD: Speech ended";
    }
    
    // Return true if we're in speech state OR if any frame in this batch had speech
    // This ensures we don't miss the start of speech
    return m_inSpeech || anySpeechDetected;
}

bool VAD::isSpeech(const QByteArray& data)
{
    const int16_t* samples = reinterpret_cast<const int16_t*>(data.constData());
    size_t sampleCount = data.size() / sizeof(int16_t);
    return isSpeech(samples, sampleCount);
}
