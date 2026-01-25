#ifndef VAD_H
#define VAD_H

#include <vector>
#include <cstdint>
#include <cstddef>

// Forward declare QByteArray to avoid including Qt headers
class QByteArray;

/**
 * Voice Activity Detection using WebRTC VAD (libfvad)
 * 
 * This provides industrial-grade voice activity detection that is much more
 * accurate than simple RMS threshold checking. It significantly reduces
 * the amount of silence sent to Whisper, improving transcription speed.
 */
class VAD
{
public:
    /**
     * VAD aggressiveness mode
     * Higher values are more aggressive about filtering out non-speech
     */
    enum class Mode {
        Quality = 0,      // Least aggressive, highest quality
        LowBitrate = 1,   // Balanced
        Aggressive = 2,   // More aggressive filtering
        VeryAggressive = 3 // Most aggressive, may cut speech
    };

    explicit VAD(int sampleRate = 16000, Mode mode = Mode::Aggressive);
    ~VAD();

    // Prevent copying (handle is not copyable)
    VAD(const VAD&) = delete;
    VAD& operator=(const VAD&) = delete;

    /**
     * Check if audio data contains speech.
     * Buffers incoming data and processes complete frames.
     * Uses hysteresis to avoid rapid toggling.
     * 
     * @param samples Pointer to int16 samples
     * @param sampleCount Number of samples
     * @return true if speech is detected in this chunk
     */
    bool isSpeech(const int16_t* samples, size_t sampleCount);

    /**
     * Convenience overload for QByteArray
     */
    bool isSpeech(const QByteArray& data);

    /**
     * Reset VAD state (call between recordings)
     */
    void reset();

    /**
     * Set aggressiveness mode
     */
    void setMode(Mode mode);

    /**
     * Check if VAD is properly initialized
     */
    bool isValid() const { return m_handle != nullptr; }

private:
    void* m_handle = nullptr;
    int m_sampleRate;
    Mode m_mode;
    
    // Frame size for WebRTC VAD (10ms, 20ms, or 30ms)
    // We use 10ms (160 samples at 16kHz) for faster response
    static constexpr size_t FRAME_SIZE_MS = 10;
    size_t m_frameSize;  // In samples

    // Smoothing parameters to avoid choppy detection
    // At 10ms per frame:
    // - 15 frames = 150ms to confirm speech start (filters out clicks/pops/noise)
    // - 50 frames = 500ms to confirm speech end (allows natural pauses)
    static constexpr size_t SPEECH_FRAMES_THRESHOLD = 15;   // Frames to confirm speech start
    static constexpr size_t SILENCE_FRAMES_THRESHOLD = 50;  // Frames to confirm speech end
    
    // Cooldown period after speech ends before we can detect new speech
    // This prevents rapid re-triggering from residual noise
    // 100 frames = 1000ms (1 second) cooldown
    static constexpr size_t COOLDOWN_FRAMES = 100;
    
    size_t m_consecutiveSpeechFrames = 0;
    size_t m_consecutiveSilenceFrames = 0;
    size_t m_cooldownFramesRemaining = 0;
    bool m_inSpeech = false;

    // Buffer for partial frames
    std::vector<int16_t> m_inputBuffer;
    size_t m_bufferUsed = 0;

    bool processFrame(const int16_t* frame);
};

#endif // VAD_H
