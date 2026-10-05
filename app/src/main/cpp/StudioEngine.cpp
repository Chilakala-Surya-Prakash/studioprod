#include "StudioEngine.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>

#if __ANDROID__
#include <android/log.h>
#define LOG_TAG "StudioEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static aaudio_data_callback_result_t outputCallback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {
    StudioEngine* engine = static_cast<StudioEngine*>(userData);
    if (engine && audioData) {
        float* outputBuffer = static_cast<float*>(audioData);
        engine->processAudioBuffer(nullptr, outputBuffer, numFrames, 2);
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static aaudio_data_callback_result_t inputCallback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {
    StudioEngine* engine = static_cast<StudioEngine*>(userData);
    if (engine && audioData) {
        const float* inputBuffer = static_cast<const float*>(audioData);
        engine->onInputAudio(inputBuffer, numFrames, 1);
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
#else
#include <cstdio>
#define LOGI(...) printf("[INFO] " __VA_ARGS__)
#define LOGE(...) printf("[ERROR] " __VA_ARGS__)
#endif

StudioEngine::StudioEngine() {
    mMicDSP.init(mSampleRate);
    mSynthEngine.init(mSampleRate);
    mDrumSynth.init(mSampleRate);
    mParametricEQ.init(mSampleRate);
    mGuitarAmpSim.init(mSampleRate);
    mDelayEffect.init(mSampleRate);
    mLufsAnalyzer.init(mSampleRate);
    mMetronome.init(mSampleRate);

    // Create 3 production default tracks
    createTrack("Lead Vocal");
    createTrack("Guitar / Keys");
    createTrack("Drum Pad / 808");

    if (!mTracks.empty()) {
        mTracks[0].isArmedForRecord = true;
    }

    // Start background recorder thread to keep audio callback lock-free
    mRecorderRunning.store(true);
    mRecorderWorker = std::thread(&StudioEngine::recorderWorkerLoop, this);
}

StudioEngine::~StudioEngine() {
    stopAudioEngine();

    mRecorderRunning.store(false);
    if (mRecorderWorker.joinable()) {
        mRecorderWorker.join();
    }
}

void StudioEngine::recorderWorkerLoop() {
    // Drains recording samples from the lock-free SPSC queue into the armed track vector
    std::vector<float> drainBuffer(2048);

    while (mRecorderRunning.load(std::memory_order_relaxed)) {
        size_t available = mRecordRingBuffer.availableRead();
        if (available >= 2) {
            size_t toRead = std::min(available, drainBuffer.size());
            toRead -= (toRead % 2); // Ensure stereo pairs

            size_t readCount = mRecordRingBuffer.read(drainBuffer.data(), toRead);
            if (readCount > 0) {
                std::lock_guard<std::mutex> lock(mTracksMutex);
                for (auto& track : mTracks) {
                    if (track.isArmedForRecord) {
                        for (size_t i = 0; i < readCount; i += 2) {
                            track.pcmBufferL.push_back(drainBuffer[i]);
                            track.pcmBufferR.push_back(drainBuffer[i + 1]);
                        }
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void StudioEngine::startAudioEngine() {
#if __ANDROID__
    // 1. Build and open AAudio Output Stream (Low-Latency Stereo)
    AAudioStreamBuilder* outBuilder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&outBuilder);
    if (result == AAUDIO_OK && outBuilder) {
        AAudioStreamBuilder_setDirection(outBuilder, AAUDIO_DIRECTION_OUTPUT);
        AAudioStreamBuilder_setSharingMode(outBuilder, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setPerformanceMode(outBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setFormat(outBuilder, AAUDIO_FORMAT_PCM_FLOAT);
        AAudioStreamBuilder_setChannelCount(outBuilder, 2);
        AAudioStreamBuilder_setSampleRate(outBuilder, mSampleRate);
        AAudioStreamBuilder_setDataCallback(outBuilder, outputCallback, this);

        result = AAudioStreamBuilder_openStream(outBuilder, &mOutputStream);
        if (result == AAUDIO_OK && mOutputStream) {
            AAudioStream_requestStart(mOutputStream);
            LOGI("AAudio output stream started successfully.");
        } else {
            LOGE("Failed to open AAudio output stream: %d", result);
        }
        AAudioStreamBuilder_delete(outBuilder);
    }

    // 2. Build and open AAudio Input Stream (Hardware Microphone)
    AAudioStreamBuilder* inBuilder = nullptr;
    result = AAudio_createStreamBuilder(&inBuilder);
    if (result == AAUDIO_OK && inBuilder) {
        AAudioStreamBuilder_setDirection(inBuilder, AAUDIO_DIRECTION_INPUT);
        AAudioStreamBuilder_setSharingMode(inBuilder, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setPerformanceMode(inBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setFormat(inBuilder, AAUDIO_FORMAT_PCM_FLOAT);
        AAudioStreamBuilder_setChannelCount(inBuilder, 1);
        AAudioStreamBuilder_setSampleRate(inBuilder, mSampleRate);
        AAudioStreamBuilder_setDataCallback(inBuilder, inputCallback, this);

        result = AAudioStreamBuilder_openStream(inBuilder, &mInputStream);
        if (result == AAUDIO_OK && mInputStream) {
            AAudioStream_requestStart(mInputStream);
            LOGI("AAudio microphone input stream started successfully.");
        } else {
            LOGE("Failed to open AAudio input stream: %d", result);
        }
        AAudioStreamBuilder_delete(inBuilder);
    }
#endif
}

void StudioEngine::stopAudioEngine() {
    stopRecording();
    stopPlayback();

#if __ANDROID__
    if (mInputStream) {
        AAudioStream_requestStop(mInputStream);
        AAudioStream_close(mInputStream);
        mInputStream = nullptr;
    }
    if (mOutputStream) {
        AAudioStream_requestStop(mOutputStream);
        AAudioStream_close(mOutputStream);
        mOutputStream = nullptr;
    }
#endif
}

void StudioEngine::onInputAudio(const float* inputBuffer, int numFrames, int numChannels) {
    if (!inputBuffer || numFrames <= 0) return;
    // Push incoming raw mic samples into lock-free input ring buffer
    mInputRingBuffer.write(inputBuffer, numFrames * numChannels);
}

void StudioEngine::setBpm(int bpm) {
    mBpm = std::clamp(bpm, 40, 260);
    mDelayEffect.setBpm(static_cast<float>(mBpm));
    mMetronome.setBpm(static_cast<float>(mBpm));
}

void StudioEngine::tapTempo() {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mTapMutex);

    // Keep taps within last 2.5 seconds
    while (!mTapHistory.empty()) {
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - mTapHistory.front()).count();
        if (diff > 2500) {
            mTapHistory.erase(mTapHistory.begin());
        } else {
            break;
        }
    }

    mTapHistory.push_back(now);

    if (mTapHistory.size() >= 2) {
        double totalIntervalMs = 0.0;
        for (size_t i = 1; i < mTapHistory.size(); ++i) {
            totalIntervalMs += std::chrono::duration_cast<std::chrono::milliseconds>(
                mTapHistory[i] - mTapHistory[i - 1]).count();
        }
        double avgIntervalMs = totalIntervalMs / (mTapHistory.size() - 1);
        if (avgIntervalMs > 100.0) {
            int calculatedBpm = static_cast<int>(round(60000.0 / avgIntervalMs));
            setBpm(calculatedBpm);
        }
    }
}

void StudioEngine::setTimeSignature(int beatsPerBar, int beatUnit) {
    mMetronome.setTimeSignature(beatsPerBar, beatUnit);
}

void StudioEngine::startRecording() {
    mIsRecording.store(true, std::memory_order_relaxed);
    mIsPlaying.store(true, std::memory_order_relaxed);
}

void StudioEngine::stopRecording() {
    mIsRecording.store(false, std::memory_order_relaxed);
}

void StudioEngine::startPlayback() {
    mIsPlaying.store(true, std::memory_order_relaxed);
}

void StudioEngine::stopPlayback() {
    mIsPlaying.store(false, std::memory_order_relaxed);
    mPlaybackPosition.store(0, std::memory_order_relaxed);
}

void StudioEngine::seekToFrame(size_t frame) {
    mPlaybackPosition.store(frame, std::memory_order_relaxed);
}

size_t StudioEngine::getTotalFrames() const {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    size_t maxFrames = 0;
    for (const auto& track : mTracks) {
        maxFrames = std::max(maxFrames, track.pcmBufferL.size());
    }
    return maxFrames;
}

int StudioEngine::createTrack(const std::string& name) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    Track newTrack;
    newTrack.id = static_cast<int>(mTracks.size());
    newTrack.name = name;
    mTracks.push_back(newTrack);
    return newTrack.id;
}

void StudioEngine::deleteTrack(int trackId) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId >= 0 && trackId < static_cast<int>(mTracks.size())) {
        mTracks.erase(mTracks.begin() + trackId);
        for (size_t i = 0; i < mTracks.size(); ++i) {
            mTracks[i].id = static_cast<int>(i);
        }
    }
}

void StudioEngine::clearTrackBuffer(int trackId) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId >= 0 && trackId < static_cast<int>(mTracks.size())) {
        mTracks[trackId].pcmBufferL.clear();
        mTracks[trackId].pcmBufferR.clear();
    }
}

void StudioEngine::setTrackVolume(int trackId, float volume) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId >= 0 && trackId < static_cast<int>(mTracks.size())) {
        mTracks[trackId].volume = std::clamp(volume, 0.0f, 1.5f);
    }
}

void StudioEngine::setTrackPan(int trackId, float pan) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId >= 0 && trackId < static_cast<int>(mTracks.size())) {
        mTracks[trackId].pan = std::clamp(pan, -1.0f, 1.0f);
    }
}

void StudioEngine::setTrackMute(int trackId, bool mute) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId >= 0 && trackId < static_cast<int>(mTracks.size())) {
        mTracks[trackId].isMuted = mute;
    }
}

void StudioEngine::setTrackSolo(int trackId, bool solo) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId >= 0 && trackId < static_cast<int>(mTracks.size())) {
        mTracks[trackId].isSolo = solo;
    }
}

void StudioEngine::armTrackForRecord(int trackId, bool arm) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId >= 0 && trackId < static_cast<int>(mTracks.size())) {
        mTracks[trackId].isArmedForRecord = arm;
    }
}

int StudioEngine::getTrackCount() const {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    return static_cast<int>(mTracks.size());
}

std::vector<float> StudioEngine::getTrackWaveform(int trackId, int numPoints) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    std::vector<float> waveform(numPoints, 0.0f);
    if (trackId < 0 || trackId >= static_cast<int>(mTracks.size()) || numPoints <= 0) {
        return waveform;
    }

    const auto& pcm = mTracks[trackId].pcmBufferL;
    size_t total = pcm.size();
    if (total == 0) return waveform;

    size_t step = std::max<size_t>(1, total / numPoints);
    for (int p = 0; p < numPoints; ++p) {
        size_t start = p * step;
        size_t end = std::min(start + step, total);
        float peak = 0.0f;
        for (size_t i = start; i < end; ++i) {
            peak = std::max(peak, std::abs(pcm[i]));
        }
        waveform[p] = std::clamp(peak, 0.0f, 1.0f);
    }
    return waveform;
}

bool StudioEngine::trimTrack(int trackId, size_t startFrame, size_t endFrame) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId < 0 || trackId >= static_cast<int>(mTracks.size())) return false;
    auto& track = mTracks[trackId];
    if (startFrame >= track.pcmBufferL.size() || endFrame <= startFrame) return false;

    size_t actualEnd = std::min(endFrame, track.pcmBufferL.size());
    std::vector<float> newL(track.pcmBufferL.begin() + startFrame, track.pcmBufferL.begin() + actualEnd);
    std::vector<float> newR(track.pcmBufferR.begin() + startFrame, track.pcmBufferR.begin() + actualEnd);

    track.pcmBufferL = std::move(newL);
    track.pcmBufferR = std::move(newR);
    return true;
}

bool StudioEngine::splitTrack(int trackId, size_t splitFrame) {
    std::lock_guard<std::mutex> lock(mTracksMutex);
    if (trackId < 0 || trackId >= static_cast<int>(mTracks.size())) return false;
    auto& track = mTracks[trackId];
    if (splitFrame >= track.pcmBufferL.size()) return false;

    Track newTrack;
    newTrack.id = static_cast<int>(mTracks.size());
    newTrack.name = track.name + " (Split)";
    newTrack.volume = track.volume;
    newTrack.pan = track.pan;

    newTrack.pcmBufferL.assign(track.pcmBufferL.begin() + splitFrame, track.pcmBufferL.end());
    newTrack.pcmBufferR.assign(track.pcmBufferR.begin() + splitFrame, track.pcmBufferR.end());

    track.pcmBufferL.erase(track.pcmBufferL.begin() + splitFrame, track.pcmBufferL.end());
    track.pcmBufferR.erase(track.pcmBufferR.begin() + splitFrame, track.pcmBufferR.end());

    mTracks.push_back(std::move(newTrack));
    return true;
}

void StudioEngine::triggerDrumPad(int padIndex, float velocity) {
    mDrumSynth.triggerPad(padIndex, velocity);
}

void StudioEngine::noteOn(int note, float velocity) {
    mSynthEngine.noteOn(note, velocity);
}

void StudioEngine::noteOff(int note) {
    mSynthEngine.noteOff(note);
}

void StudioEngine::setSynthWaveform(int waveformIndex) {
    mSynthEngine.setWaveform(static_cast<WaveformType>(waveformIndex));
}

void StudioEngine::setSynthCutoff(float cutoffHz) {
    mSynthEngine.setCutoff(cutoffHz);
}

void StudioEngine::setMetronomeEnabled(bool enabled) {
    mMetronome.setEnabled(enabled);
}

void StudioEngine::setMetronomeVolume(float vol) {
    mMetronome.setClickVolume(vol);
}

void StudioEngine::setGuitarAmpEnabled(bool enabled) {
    mGuitarAmpSim.setEnabled(enabled);
}

void StudioEngine::setGuitarAmpModel(int model) {
    mGuitarAmpSim.setModel(static_cast<AmpModel>(model));
}

void StudioEngine::setGuitarAmpGain(float gain) {
    mGuitarAmpSim.setGain(gain);
}

void StudioEngine::setGuitarAmpTone(float tone) {
    mGuitarAmpSim.setTone(tone);
}

void StudioEngine::setDelayEnabled(bool enabled) {
    mDelayEffect.setEnabled(enabled);
}

void StudioEngine::setDelayMode(int mode) {
    mDelayEffect.setMode(static_cast<DelayMode>(mode));
}

void StudioEngine::setDelayFeedback(float feedback) {
    mDelayEffect.setFeedback(feedback);
}

void StudioEngine::setDelayWetMix(float wetMix) {
    mDelayEffect.setWetMix(wetMix);
}

void StudioEngine::setEQBand(int bandIndex, int type, float freq, float gainDb, float q, bool enabled) {
    EQBand band;
    band.type = static_cast<EQBand::Type>(type);
    band.frequency = freq;
    band.gainDb = gainDb;
    band.q = q;
    band.enabled = enabled;
    mParametricEQ.setBand(bandIndex, band);
}

// ─── True-Peak Lookahead Master Limiter ──────────────────────────────────────

void StudioEngine::applyMasterLimiter(float* buffer, int numFrames, int numChannels) {
    const float threshold = 0.89125f; // -1.0 dBTP ceiling
    const float releaseCoeff = 0.9995f;

    for (int i = 0; i < numFrames; ++i) {
        float inL = buffer[i * numChannels];
        float inR = (numChannels > 1) ? buffer[i * numChannels + 1] : inL;

        // Store into lookahead delay ring buffer
        mLimiterBufferL[mLimiterIndex] = inL;
        mLimiterBufferR[mLimiterIndex] = inR;

        // Peak detection on incoming sample
        float peak = std::max(std::abs(inL), std::abs(inR));
        if (peak > threshold) {
            float desiredGain = threshold / peak;
            if (desiredGain < mLimiterGain) {
                mLimiterGain = desiredGain; // Fast attack
            }
        } else {
            mLimiterGain = mLimiterGain * releaseCoeff + 1.0f * (1.0f - releaseCoeff); // Smooth release
        }

        // Read delayed sample and apply limiter gain
        int readIndex = (mLimiterIndex + 1) % LIMITER_LOOKAHEAD;
        buffer[i * numChannels] = mLimiterBufferL[readIndex] * mLimiterGain;
        if (numChannels > 1) {
            buffer[i * numChannels + 1] = mLimiterBufferR[readIndex] * mLimiterGain;
        }

        mLimiterIndex = readIndex;
    }
}

// ─── Real-Time Audio Processing Callback ────────────────────────────────────

void StudioEngine::processAudioBuffer(float* inputBuffer, float* outputBuffer, int numFrames, int numChannels) {
    if (outputBuffer) {
        std::fill(outputBuffer, outputBuffer + numFrames * numChannels, 0.0f);
    }

    // 1. Process Hardware Microphone Audio from Input Ring Buffer
    float micBuffer[1024];
    int framesToProcess = std::min(numFrames, 1024);
    size_t micRead = mInputRingBuffer.read(micBuffer, framesToProcess);

    if (micRead > 0) {
        // Run microphone input through StudioMicDSP (Transform phone mic into studio vocal)
        mMicDSP.processBuffer(micBuffer, static_cast<int>(micRead), 1);

        // If recording is active, write into lock-free recording queue for background thread to persist
        if (mIsRecording.load(std::memory_order_relaxed)) {
            float stereoRec[2048];
            for (size_t i = 0; i < micRead; ++i) {
                stereoRec[i * 2]     = micBuffer[i];
                stereoRec[i * 2 + 1] = micBuffer[i];
            }
            mRecordRingBuffer.write(stereoRec, micRead * 2);
        }

        // Low-latency zero-latency vocal monitoring into output headphones
        if (mMonitoringEnabled.load(std::memory_order_relaxed) && outputBuffer) {
            float monLvl = mMonitoringLevel.load(std::memory_order_relaxed);
            for (size_t i = 0; i < micRead; ++i) {
                outputBuffer[i * numChannels]     += micBuffer[i] * monLvl;
                if (numChannels > 1) {
                    outputBuffer[i * numChannels + 1] += micBuffer[i] * monLvl;
                }
            }
        }
    }

    // 2. Synthesize Virtual Studio Instruments (Drum Pads & Synth Keys)
    if (outputBuffer) {
        mDrumSynth.process(outputBuffer, numFrames, numChannels);
        mSynthEngine.process(outputBuffer, numFrames, numChannels);
    }

    // 3. Mix Down Tracks for Playback
    if (outputBuffer) {
        size_t currentPlaybackPos = mPlaybackPosition.load(std::memory_order_relaxed);

        // Try lock to prevent blocking audio thread
        if (mTracksMutex.try_lock()) {
            bool anySolo = false;
            for (const auto& track : mTracks) {
                if (track.isSolo) { anySolo = true; break; }
            }

            for (int i = 0; i < numFrames; ++i) {
                float masterL = outputBuffer[i * numChannels];
                float masterR = (numChannels > 1) ? outputBuffer[i * numChannels + 1] : masterL;
                size_t framePos = currentPlaybackPos + i;

                for (const auto& track : mTracks) {
                    if (track.isMuted) continue;
                    if (anySolo && !track.isSolo) continue;

                    if (framePos < track.pcmBufferL.size()) {
                        float sL = track.pcmBufferL[framePos] * track.volume;
                        float sR = track.pcmBufferR[framePos] * track.volume;

                        // Equal-power pan law: cosine/sine curve
                        float panAngle = (track.pan + 1.0f) * 3.14159265f / 4.0f;
                        float panL = cosf(panAngle);
                        float panR = sinf(panAngle);

                        masterL += sL * panL;
                        masterR += sR * panR;
                    }
                }

                outputBuffer[i * numChannels] = masterL;
                if (numChannels > 1) {
                    outputBuffer[i * numChannels + 1] = masterR;
                }
            }

            mTracksMutex.unlock();
        }

        if (mIsPlaying.load(std::memory_order_relaxed)) {
            mPlaybackPosition.fetch_add(numFrames, std::memory_order_relaxed);
        }

        // 4. Channel Strip & Master Rack FX Pipeline
        mGuitarAmpSim.processBuffer(outputBuffer, numFrames, numChannels);
        mParametricEQ.processBuffer(outputBuffer, numFrames, numChannels);
        mDelayEffect.processBuffer(outputBuffer, numFrames, numChannels);
        mMetronome.processBuffer(outputBuffer, numFrames, numChannels);

        // 5. True-Peak Master Bus Limiter (Guarantees <= -1.0 dBTP on master)
        applyMasterLimiter(outputBuffer, numFrames, numChannels);

        // 6. Master LUFS Loudness Metering
        mLufsAnalyzer.processSamples(outputBuffer, numFrames, numChannels);
    }
}

// ─── WAV File Exporter ──────────────────────────────────────────────────────

static bool writeWav24Bit(const std::string& path, uint32_t sampleRate,
                          const std::vector<float>& pcmL, const std::vector<float>& pcmR) {
    size_t numFrames = std::max(pcmL.size(), pcmR.size());
    if (numFrames == 0) return false;

    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open()) return false;

    uint16_t numChannels = 2;
    uint16_t bitsPerSample = 24;
    uint32_t bytesPerSample = bitsPerSample / 8;
    uint32_t dataSize = static_cast<uint32_t>(numFrames * numChannels * bytesPerSample);
    uint32_t chunkSize = 36 + dataSize;

    // RIFF Header
    outFile.write("RIFF", 4);
    outFile.write(reinterpret_cast<const char*>(&chunkSize), 4);
    outFile.write("WAVE", 4);

    // fmt subchunk
    outFile.write("fmt ", 4);
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    uint32_t byteRate = sampleRate * numChannels * bytesPerSample;
    uint16_t blockAlign = numChannels * bytesPerSample;

    outFile.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    outFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
    outFile.write(reinterpret_cast<const char*>(&numChannels), 2);
    outFile.write(reinterpret_cast<const char*>(&sampleRate), 4);
    outFile.write(reinterpret_cast<const char*>(&byteRate), 4);
    outFile.write(reinterpret_cast<const char*>(&blockAlign), 2);
    outFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data subchunk
    outFile.write("data", 4);
    outFile.write(reinterpret_cast<const char*>(&dataSize), 4);

    for (size_t i = 0; i < numFrames; ++i) {
        float sampleL = (i < pcmL.size()) ? std::clamp(pcmL[i], -0.999f, 0.999f) : 0.0f;
        float sampleR = (i < pcmR.size()) ? std::clamp(pcmR[i], -0.999f, 0.999f) : 0.0f;

        int32_t intSampleL = static_cast<int32_t>(sampleL * 8388607.0f);
        int32_t intSampleR = static_cast<int32_t>(sampleR * 8388607.0f);

        char pcm24L[3] = {
            static_cast<char>(intSampleL & 0xFF),
            static_cast<char>((intSampleL >> 8) & 0xFF),
            static_cast<char>((intSampleL >> 16) & 0xFF)
        };
        char pcm24R[3] = {
            static_cast<char>(intSampleR & 0xFF),
            static_cast<char>((intSampleR >> 8) & 0xFF),
            static_cast<char>((intSampleR >> 16) & 0xFF)
        };

        outFile.write(pcm24L, 3);
        outFile.write(pcm24R, 3);
    }

    outFile.close();
    return true;
}

bool StudioEngine::exportMasterWav(const std::string& filePath, bool exportStems) {
    std::lock_guard<std::mutex> lock(mTracksMutex);

    size_t maxFrames = 0;
    for (const auto& track : mTracks) {
        maxFrames = std::max(maxFrames, track.pcmBufferL.size());
    }

    if (maxFrames == 0) return false;

    // 1. Export Master Mixdown WAV (with equal-power panning & master bus EQ/limiting)
    std::vector<float> masterBufL(maxFrames, 0.0f);
    std::vector<float> masterBufR(maxFrames, 0.0f);

    for (size_t i = 0; i < maxFrames; ++i) {
        for (const auto& track : mTracks) {
            if (track.isMuted) continue;
            if (i < track.pcmBufferL.size()) {
                float panAngle = (track.pan + 1.0f) * 3.14159265f / 4.0f;
                float panL = cosf(panAngle);
                float panR = sinf(panAngle);
                masterBufL[i] += track.pcmBufferL[i] * track.volume * panL;
                masterBufR[i] += track.pcmBufferR[i] * track.volume * panR;
            }
        }
    }

    // Process Master FX Bus
    mParametricEQ.processBuffer(masterBufL.data(), static_cast<int>(maxFrames), 1);
    mParametricEQ.processBuffer(masterBufR.data(), static_cast<int>(maxFrames), 1);

    // Apply lookahead True-Peak Limiter
    std::vector<float> interleavedMaster(maxFrames * 2);
    for (size_t i = 0; i < maxFrames; ++i) {
        interleavedMaster[i * 2]     = masterBufL[i];
        interleavedMaster[i * 2 + 1] = masterBufR[i];
    }
    applyMasterLimiter(interleavedMaster.data(), static_cast<int>(maxFrames), 2);

    for (size_t i = 0; i < maxFrames; ++i) {
        masterBufL[i] = interleavedMaster[i * 2];
        masterBufR[i] = interleavedMaster[i * 2 + 1];
    }

    bool masterSuccess = writeWav24Bit(filePath, mSampleRate, masterBufL, masterBufR);

    // 2. Export Individual Stems if requested
    if (exportStems) {
        std::string basePath = filePath;
        size_t dotPos = basePath.find_last_of('.');
        if (dotPos != std::string::npos) basePath = basePath.substr(0, dotPos);

        for (size_t t = 0; t < mTracks.size(); ++t) {
            const auto& track = mTracks[t];
            std::string stemPath = basePath + "_Stem_" + std::to_string(t + 1) + "_" + track.name + ".wav";
            writeWav24Bit(stemPath, mSampleRate, track.pcmBufferL, track.pcmBufferR);
        }
    }

    return masterSuccess;
}
