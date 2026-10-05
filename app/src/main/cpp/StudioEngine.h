#ifndef STUDIO_ENGINE_H
#define STUDIO_ENGINE_H

#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#if __ANDROID__
#include <aaudio/AAudio.h>
#else
typedef void AAudioStream;
#endif

#include "StudioMicDSP.h"
#include "VirtualSynthEngine.h"
#include "DrumSynthEngine.h"
#include "ParametricEQ.h"
#include "GuitarAmpSim.h"
#include "DelayEffect.h"
#include "LufsAnalyzer.h"
#include "Metronome.h"
#include "LockFreeQueue.h"

struct Track {
    int id = 0;
    std::string name;
    float volume = 0.8f;  // 0.0 to 1.5
    float pan = 0.0f;     // -1.0 (Left) to +1.0 (Right)
    bool isMuted = false;
    bool isSolo = false;
    bool isArmedForRecord = false;
    std::vector<float> pcmBufferL;
    std::vector<float> pcmBufferR;
};

class StudioEngine {
public:
    StudioEngine();
    ~StudioEngine();

    void startAudioEngine();
    void stopAudioEngine();

    // Session & Tempo
    void setBpm(int bpm);
    int getBpm() const { return mBpm; }
    void tapTempo();
    void setTimeSignature(int beatsPerBar, int beatUnit);

    // Transport
    void startRecording();
    void stopRecording();
    bool isRecording() const { return mIsRecording.load(std::memory_order_relaxed); }

    void startPlayback();
    void stopPlayback();
    bool isPlaying() const { return mIsPlaying.load(std::memory_order_relaxed); }

    void seekToFrame(size_t frame);
    size_t getPlaybackPosition() const { return mPlaybackPosition.load(std::memory_order_relaxed); }
    size_t getTotalFrames() const;

    // Monitoring
    void setMonitoringEnabled(bool enabled) { mMonitoringEnabled.store(enabled, std::memory_order_relaxed); }
    bool isMonitoringEnabled() const { return mMonitoringEnabled.load(std::memory_order_relaxed); }
    void setMonitoringLevel(float level) { mMonitoringLevel.store(level, std::memory_order_relaxed); }

    // Dynamic Track Management
    int createTrack(const std::string& name);
    void deleteTrack(int trackId);
    void clearTrackBuffer(int trackId);
    void setTrackVolume(int trackId, float volume);
    void setTrackPan(int trackId, float pan);
    void setTrackMute(int trackId, bool mute);
    void setTrackSolo(int trackId, bool solo);
    void armTrackForRecord(int trackId, bool arm);
    int getTrackCount() const;

    // Track Editing & Waveform Visualization
    std::vector<float> getTrackWaveform(int trackId, int numPoints);
    bool trimTrack(int trackId, size_t startFrame, size_t endFrame);
    bool splitTrack(int trackId, size_t splitFrame);

    // Instrument Triggering
    void triggerDrumPad(int padIndex, float velocity);
    void noteOn(int note, float velocity);
    void noteOff(int note);
    void setSynthWaveform(int waveformIndex);
    void setSynthCutoff(float cutoffHz);

    // Metronome
    void setMetronomeEnabled(bool enabled);
    void setMetronomeVolume(float vol);
    bool isMetronomeEnabled() const { return mMetronome.isEnabled(); }
    int getMetronomeBeat() const { return mMetronome.getCurrentBeat(); }

    // FX Controls
    void setGuitarAmpEnabled(bool enabled);
    void setGuitarAmpModel(int model);
    void setGuitarAmpGain(float gain);
    void setGuitarAmpTone(float tone);

    void setDelayEnabled(bool enabled);
    void setDelayMode(int mode);
    void setDelayFeedback(float feedback);
    void setDelayWetMix(float wetMix);

    void setEQBand(int bandIndex, int type, float freq, float gainDb, float q, bool enabled);

    // DSP & Export
    StudioMicDSP& getMicDSP() { return mMicDSP; }
    VirtualSynthEngine& getSynth() { return mSynthEngine; }
    DrumSynthEngine& getDrumSynth() { return mDrumSynth; }
    LufsAnalyzer& getLufsAnalyzer() { return mLufsAnalyzer; }

    float getMasterLufs() const { return mLufsAnalyzer.getIntegratedLufs(); }
    float getMomentaryLufs() const { return mLufsAnalyzer.getMomentaryLufs(); }

    bool exportMasterWav(const std::string& filePath, bool exportStems);

    // Audio Callbacks
    void onInputAudio(const float* inputBuffer, int numFrames, int numChannels);
    void processAudioBuffer(float* inputBuffer, float* outputBuffer, int numFrames, int numChannels);

private:
    int mSampleRate = 48000;
    int mBpm = 120;
    std::atomic<bool> mIsRecording{false};
    std::atomic<bool> mIsPlaying{false};
    std::atomic<size_t> mPlaybackPosition{0};

    std::atomic<bool> mMonitoringEnabled{true};
    std::atomic<float> mMonitoringLevel{0.9f};

    AAudioStream* mInputStream = nullptr;
    AAudioStream* mOutputStream = nullptr;

    std::vector<Track> mTracks;
    mutable std::mutex mTracksMutex;

    // Lock-Free SPSC Queues for real-time audio safety
    SpscRingBuffer<float, 65536> mInputRingBuffer;
    SpscRingBuffer<float, 131072> mRecordRingBuffer;

    // Background recording thread to offload vector writes from the audio thread
    std::thread mRecorderWorker;
    std::atomic<bool> mRecorderRunning{false};
    void recorderWorkerLoop();

    // Tap tempo history
    std::vector<std::chrono::steady_clock::time_point> mTapHistory;
    std::mutex mTapMutex;

    // Subsystem DSP Engines
    StudioMicDSP       mMicDSP;
    VirtualSynthEngine mSynthEngine;
    DrumSynthEngine    mDrumSynth;
    ParametricEQ       mParametricEQ;
    GuitarAmpSim       mGuitarAmpSim;
    DelayEffect        mDelayEffect;
    LufsAnalyzer       mLufsAnalyzer;
    Metronome          mMetronome;

    // True-Peak Lookahead Master Limiter state
    static constexpr int LIMITER_LOOKAHEAD = 64;
    float mLimiterBufferL[LIMITER_LOOKAHEAD] = {0.0f};
    float mLimiterBufferR[LIMITER_LOOKAHEAD] = {0.0f};
    int mLimiterIndex = 0;
    float mLimiterGain = 1.0f;
    void applyMasterLimiter(float* buffer, int numFrames, int numChannels);
};

#endif // STUDIO_ENGINE_H
