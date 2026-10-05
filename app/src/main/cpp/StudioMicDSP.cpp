#include "StudioMicDSP.h"
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ─── Biquad Filter Implementation ───────────────────────────────────────────

void Biquad::setPeaking(float freq, float gainDb, float q, float sampleRate) {
    float A = powf(10.0f, gainDb / 40.0f);
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn / (2.0f * q);

    float b0_raw = 1.0f + alpha * A;
    float b1_raw = -2.0f * cs;
    float b2_raw = 1.0f - alpha * A;
    float a0_raw = 1.0f + alpha / A;
    float a1_raw = -2.0f * cs;
    float a2_raw = 1.0f - alpha / A;

    b0 = b0_raw / a0_raw;
    b1 = b1_raw / a0_raw;
    b2 = b2_raw / a0_raw;
    a1 = a1_raw / a0_raw;
    a2 = a2_raw / a0_raw;
}

void Biquad::setHighShelf(float freq, float gainDb, float q, float sampleRate) {
    float A = powf(10.0f, gainDb / 40.0f);
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float beta = sqrtf(A) / q;

    float b0_raw = A * ((A + 1.0f) + (A - 1.0f) * cs + beta * sn);
    float b1_raw = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
    float b2_raw = A * ((A + 1.0f) + (A - 1.0f) * cs - beta * sn);
    float a0_raw = (A + 1.0f) - (A - 1.0f) * cs + beta * sn;
    float a1_raw = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
    float a2_raw = (A + 1.0f) - (A - 1.0f) * cs - beta * sn;

    b0 = b0_raw / a0_raw;
    b1 = b1_raw / a0_raw;
    b2 = b2_raw / a0_raw;
    a1 = a1_raw / a0_raw;
    a2 = a2_raw / a0_raw;
}

void Biquad::setLowShelf(float freq, float gainDb, float q, float sampleRate) {
    float A = powf(10.0f, gainDb / 40.0f);
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float beta = sqrtf(A) / q;

    float b0_raw = A * ((A + 1.0f) - (A - 1.0f) * cs + beta * sn);
    float b1_raw = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
    float b2_raw = A * ((A + 1.0f) - (A - 1.0f) * cs - beta * sn);
    float a0_raw = (A + 1.0f) + (A - 1.0f) * cs + beta * sn;
    float a1_raw = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
    float a2_raw = (A + 1.0f) + (A - 1.0f) * cs - beta * sn;

    b0 = b0_raw / a0_raw;
    b1 = b1_raw / a0_raw;
    b2 = b2_raw / a0_raw;
    a1 = a1_raw / a0_raw;
    a2 = a2_raw / a0_raw;
}

void Biquad::setBandpass(float freq, float q, float sampleRate) {
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn / (2.0f * q);

    float b0_raw = alpha;
    float b1_raw = 0.0f;
    float b2_raw = -alpha;
    float a0_raw = 1.0f + alpha;
    float a1_raw = -2.0f * cs;
    float a2_raw = 1.0f - alpha;

    b0 = b0_raw / a0_raw;
    b1 = b1_raw / a0_raw;
    b2 = b2_raw / a0_raw;
    a1 = a1_raw / a0_raw;
    a2 = a2_raw / a0_raw;
}

void Biquad::setNotch(float freq, float q, float sampleRate) {
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn / (2.0f * q);

    float b0_raw = 1.0f;
    float b1_raw = -2.0f * cs;
    float b2_raw = 1.0f;
    float a0_raw = 1.0f + alpha;
    float a1_raw = -2.0f * cs;
    float a2_raw = 1.0f - alpha;

    b0 = b0_raw / a0_raw;
    b1 = b1_raw / a0_raw;
    b2 = b2_raw / a0_raw;
    a1 = a1_raw / a0_raw;
    a2 = a2_raw / a0_raw;
}

void Biquad::setLowpass(float freq, float q, float sampleRate) {
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn / (2.0f * q);

    float b0_raw = (1.0f - cs) / 2.0f;
    float b1_raw = 1.0f - cs;
    float b2_raw = (1.0f - cs) / 2.0f;
    float a0_raw = 1.0f + alpha;
    float a1_raw = -2.0f * cs;
    float a2_raw = 1.0f - alpha;

    b0 = b0_raw / a0_raw;
    b1 = b1_raw / a0_raw;
    b2 = b2_raw / a0_raw;
    a1 = a1_raw / a0_raw;
    a2 = a2_raw / a0_raw;
}

void Biquad::setHighpass(float freq, float q, float sampleRate) {
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn / (2.0f * q);

    float b0_raw = (1.0f + cs) / 2.0f;
    float b1_raw = -(1.0f + cs);
    float b2_raw = (1.0f + cs) / 2.0f;
    float a0_raw = 1.0f + alpha;
    float a1_raw = -2.0f * cs;
    float a2_raw = 1.0f - alpha;

    b0 = b0_raw / a0_raw;
    b1 = b1_raw / a0_raw;
    b2 = b2_raw / a0_raw;
    a1 = a1_raw / a0_raw;
    a2 = a2_raw / a0_raw;
}

// ─── Comb & All-Pass Filters ────────────────────────────────────────────────

void CombFilter::init(int size, float fb, float d) {
    bufferSize = std::max(size, 16);
    buffer.assign(bufferSize, 0.0f);
    bufferIdx = 0;
    feedback = fb;
    damp = d;
    filterStore = 0.0f;
}

inline float CombFilter::process(float in) {
    float output = buffer[bufferIdx];
    filterStore = (output * (1.0f - damp)) + (filterStore * damp);
    buffer[bufferIdx] = in + (filterStore * feedback);
    bufferIdx = (bufferIdx + 1) % bufferSize;
    return output;
}

void AllPassFilter::init(int size, float fb) {
    bufferSize = std::max(size, 16);
    buffer.assign(bufferSize, 0.0f);
    bufferIdx = 0;
    feedback = fb;
}

inline float AllPassFilter::process(float in) {
    float bufOut = buffer[bufferIdx];
    float out = -in + bufOut;
    buffer[bufferIdx] = in + (bufOut * feedback);
    bufferIdx = (bufferIdx + 1) % bufferSize;
    return out;
}

// ─── StudioMicDSP Implementation ────────────────────────────────────────────

StudioMicDSP::StudioMicDSP() {
    std::fill(mPitchDelayBuffer, mPitchDelayBuffer + PITCH_BUFFER_SIZE, 0.0f);
    init(48000);
}

StudioMicDSP::~StudioMicDSP() {}

void StudioMicDSP::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;

    // Initialize Schroeder Reverb delay lines (calibrated for warm studio vocal booth)
    int combTuningsL[4] = {
        static_cast<int>(mSampleRate * 0.0297f), // ~29.7ms
        static_cast<int>(mSampleRate * 0.0371f), // ~37.1ms
        static_cast<int>(mSampleRate * 0.0411f), // ~41.1ms
        static_cast<int>(mSampleRate * 0.0437f)  // ~43.7ms
    };
    int combTuningsR[4] = {
        static_cast<int>(mSampleRate * 0.0315f), // Stereo offset
        static_cast<int>(mSampleRate * 0.0354f),
        static_cast<int>(mSampleRate * 0.0423f),
        static_cast<int>(mSampleRate * 0.0456f)
    };

    for (int i = 0; i < 4; ++i) {
        mCombL[i].init(combTuningsL[i], 0.78f, 0.25f);
        mCombR[i].init(combTuningsR[i], 0.78f, 0.25f);
    }

    int allPassTuningsL[2] = {
        static_cast<int>(mSampleRate * 0.0051f), // 5.1ms
        static_cast<int>(mSampleRate * 0.0017f)  // 1.7ms
    };
    int allPassTuningsR[2] = {
        static_cast<int>(mSampleRate * 0.0057f),
        static_cast<int>(mSampleRate * 0.0019f)
    };

    for (int i = 0; i < 2; ++i) {
        mAllPassL[i].init(allPassTuningsL[i], 0.5f);
        mAllPassR[i].init(allPassTuningsR[i], 0.5f);
    }

    // Pitch shifter state
    mPitchWriteIndex = 0;
    mPitchTap1 = 0.0f;
    mPitchTap2 = mPitchWindowSize * 0.5f;

    setPreset(PRESET_NEUMANN_U87);
}

void StudioMicDSP::setPreset(MicPreset preset) {
    mCurrentPreset = preset;
    switch (preset) {
        case PRESET_NEUMANN_U87:
            mParams.hpCutoffHz = 85.0f;
            mParams.tubeWarmth = 0.55f;
            mParams.deEsserAmount = 0.60f;
            mParams.airEqGainDb = 5.0f;
            mParams.compThresholdDb = -18.0f;
            mParams.compRatio = 4.0f;
            mParams.reverbMix = 0.22f;
            mParams.noiseGateThreshold = 0.012f;
            break;

        case PRESET_VINTAGE_TUBE:
            mParams.hpCutoffHz = 75.0f;
            mParams.tubeWarmth = 0.85f;
            mParams.deEsserAmount = 0.50f;
            mParams.airEqGainDb = 3.5f;
            mParams.compThresholdDb = -20.0f;
            mParams.compRatio = 6.0f;
            mParams.reverbMix = 0.32f;
            mParams.noiseGateThreshold = 0.015f;
            break;

        case PRESET_RIBBON_WARMTH:
            mParams.hpCutoffHz = 95.0f;
            mParams.tubeWarmth = 0.70f;
            mParams.deEsserAmount = 0.70f;
            mParams.airEqGainDb = 1.5f;
            mParams.compThresholdDb = -14.0f;
            mParams.compRatio = 3.0f;
            mParams.reverbMix = 0.18f;
            mParams.noiseGateThreshold = 0.010f;
            break;

        case PRESET_STAGE_DYNAMIC:
            mParams.hpCutoffHz = 110.0f;
            mParams.tubeWarmth = 0.35f;
            mParams.deEsserAmount = 0.45f;
            mParams.airEqGainDb = 2.5f;
            mParams.compThresholdDb = -12.0f;
            mParams.compRatio = 4.0f;
            mParams.reverbMix = 0.12f;
            mParams.noiseGateThreshold = 0.025f;
            break;

        case PRESET_BYPASS:
            mParams.hpCutoffHz = 20.0f;
            mParams.tubeWarmth = 0.0f;
            mParams.deEsserAmount = 0.0f;
            mParams.airEqGainDb = 0.0f;
            mParams.compThresholdDb = 0.0f;
            mParams.compRatio = 1.0f;
            mParams.reverbMix = 0.0f;
            mParams.noiseGateThreshold = 0.0f;
            break;
    }
    updateHPFFilter();
    updateAcousticIRFilters();
}

void StudioMicDSP::setParams(const DSPParams& params) {
    mParams = params;
    updateHPFFilter();
    updateAcousticIRFilters();
}

void StudioMicDSP::updateHPFFilter() {
    // 18 dB/oct Butterworth filter: Cascaded 1-pole high-pass + 2-pole Butterworth biquad
    float fc = std::clamp(mParams.hpCutoffHz, 20.0f, 300.0f);

    // 1-pole HP filter coefficient
    float omega = 2.0f * M_PI * fc / mSampleRate;
    mHp1Pole_a = expf(-omega);

    // 2-pole Butterworth biquad
    for (int ch = 0; ch < 2; ++ch) {
        mHpBiquad[ch].setHighpass(fc, 0.7071f, static_cast<float>(mSampleRate));
    }
}

void StudioMicDSP::updateAcousticIRFilters() {
    float sr = static_cast<float>(mSampleRate);

    // Acoustic IR Calibration Filters based on selected microphone target
    for (int ch = 0; ch < 2; ++ch) {
        switch (mCurrentPreset) {
            case PRESET_NEUMANN_U87:
                // Neumann U87 profile: Gentle proximity warmth at 180Hz, presence lift at 3.5kHz, air sparkle at 11kHz
                mIrFilter1[ch].setLowShelf(180.0f, 2.0f, 0.71f, sr);
                mIrFilter2[ch].setPeaking(3500.0f, 2.2f, 1.2f, sr);
                mIrFilter3[ch].setHighShelf(11000.0f, 3.5f, 0.8f, sr);
                break;

            case PRESET_VINTAGE_TUBE:
                // Sony C800G / Telefunken 251: Rich low chest warmth at 120Hz, smoothed harsh midrange at 4kHz, tube top lift
                mIrFilter1[ch].setLowShelf(120.0f, 3.2f, 0.65f, sr);
                mIrFilter2[ch].setPeaking(3800.0f, -1.8f, 1.5f, sr);
                mIrFilter3[ch].setHighShelf(12500.0f, 2.8f, 0.75f, sr);
                break;

            case PRESET_RIBBON_WARMTH:
                // Coles 4038 / Royer R121: Velvety low-mids at 250Hz, ultra-smooth top roll-off
                mIrFilter1[ch].setPeaking(250.0f, 2.8f, 0.9f, sr);
                mIrFilter2[ch].setPeaking(2800.0f, 1.0f, 1.0f, sr);
                mIrFilter3[ch].setHighShelf(9500.0f, -3.0f, 0.71f, sr);
                break;

            case PRESET_STAGE_DYNAMIC:
                // Shure SM7B: Vocal presence boost at 4.8kHz, mid clarity boost
                mIrFilter1[ch].setPeaking(150.0f, 1.5f, 0.8f, sr);
                mIrFilter2[ch].setPeaking(4800.0f, 4.5f, 1.4f, sr);
                mIrFilter3[ch].setHighShelf(8000.0f, -1.5f, 0.71f, sr);
                break;

            case PRESET_BYPASS:
            default:
                mIrFilter1[ch].setPeaking(1000.0f, 0.0f, 1.0f, sr);
                mIrFilter2[ch].setPeaking(1000.0f, 0.0f, 1.0f, sr);
                mIrFilter3[ch].setPeaking(1000.0f, 0.0f, 1.0f, sr);
                break;
        }

        // De-Esser: Sidechain bandpass @ 6.5kHz (Q=2.0) and notch filter
        mDeEsserSidechain[ch].setBandpass(6500.0f, 2.0f, sr);
        mDeEsserNotch[ch].setNotch(6500.0f, 2.5f, sr);

        // Multi-band Crossover: Low (< 250Hz) and High (> 4000Hz)
        mCrossoverLp[ch].setLowpass(250.0f, 0.7071f, sr);
        mCrossoverHp[ch].setHighpass(4000.0f, 0.7071f, sr);

        // Air EQ High-Shelf @ 14kHz
        mAirEq[ch].setHighShelf(14000.0f, mParams.airEqGainDb, 0.7071f, sr);

        // Formant Shift Filter: shifts formants up or down
        float formantFreq = 2500.0f * powf(2.0f, mParams.formantShift * 0.4f);
        mFormantFilter[ch].setPeaking(formantFreq, mParams.formantShift * 4.0f, 1.2f, sr);
    }
}

// ─── Pitch Detection & Quantization ─────────────────────────────────────────

float StudioMicDSP::detectPitchYin(const float* buffer, int numFrames) {
    if (numFrames < 256) return 440.0f;

    int halfFrames = numFrames / 2;
    std::vector<float> d(halfFrames, 0.0f);

    for (int tau = 1; tau < halfFrames; ++tau) {
        float sum = 0.0f;
        for (int i = 0; i < halfFrames; ++i) {
            float delta = buffer[i] - buffer[i + tau];
            sum += delta * delta;
        }
        d[tau] = sum;
    }

    d[0] = 1.0f;
    float runningSum = 0.0f;
    int bestTau = -1;
    float threshold = 0.15f;

    for (int tau = 1; tau < halfFrames; ++tau) {
        runningSum += d[tau];
        if (runningSum > 0.0f) {
            d[tau] *= static_cast<float>(tau) / runningSum;
        }
        if (d[tau] < threshold && bestTau == -1) {
            bestTau = tau;
            break;
        }
    }

    if (bestTau <= 0) return 440.0f;

    // Parabolic interpolation for fine tuning accuracy
    float x0 = (bestTau > 0) ? d[bestTau - 1] : d[bestTau];
    float x1 = d[bestTau];
    float x2 = (bestTau + 1 < halfFrames) ? d[bestTau + 1] : d[bestTau];
    float deltaTau = 0.0f;
    float denom = 2.0f * (x0 - 2.0f * x1 + x2);
    if (std::abs(denom) > 1e-6f) {
        deltaTau = (x0 - x2) / denom;
    }

    float finalTau = static_cast<float>(bestTau) + deltaTau;
    if (finalTau <= 0.0f) return 440.0f;

    return static_cast<float>(mSampleRate) / finalTau;
}

float StudioMicDSP::quantizePitchToScale(float detectedPitchHz) {
    if (detectedPitchHz < 55.0f || detectedPitchHz > 1800.0f) return detectedPitchHz;

    float midiNoteExact = 69.0f + 12.0f * log2f(detectedPitchHz / 440.0f);
    int nearestMidi = static_cast<int>(roundf(midiNoteExact));

    int noteInOctave = (nearestMidi % 12 + 12) % 12;
    int root = (mParams.rootKeyNote % 12 + 12) % 12;
    int relativeNote = (noteInOctave - root + 12) % 12;

    bool isAllowed = false;
    switch (mParams.scaleType) {
        case SCALE_CHROMATIC:
            isAllowed = true;
            break;
        case SCALE_MAJOR: // Major: 0, 2, 4, 5, 7, 9, 11
            isAllowed = (relativeNote == 0 || relativeNote == 2 || relativeNote == 4 ||
                         relativeNote == 5 || relativeNote == 7 || relativeNote == 9 || relativeNote == 11);
            break;
        case SCALE_MINOR: // Natural Minor: 0, 2, 3, 5, 7, 8, 10
            isAllowed = (relativeNote == 0 || relativeNote == 2 || relativeNote == 3 ||
                         relativeNote == 5 || relativeNote == 7 || relativeNote == 8 || relativeNote == 10);
            break;
        case SCALE_PENTATONIC: // Major Pentatonic: 0, 2, 4, 7, 9
            isAllowed = (relativeNote == 0 || relativeNote == 2 || relativeNote == 4 ||
                         relativeNote == 7 || relativeNote == 9);
            break;
        case SCALE_BLUES: // Blues scale: 0, 3, 5, 6, 7, 10
            isAllowed = (relativeNote == 0 || relativeNote == 3 || relativeNote == 5 ||
                         relativeNote == 6 || relativeNote == 7 || relativeNote == 10);
            break;
        case SCALE_DORIAN: // Dorian: 0, 2, 3, 5, 7, 9, 10
            isAllowed = (relativeNote == 0 || relativeNote == 2 || relativeNote == 3 ||
                         relativeNote == 5 || relativeNote == 7 || relativeNote == 9 || relativeNote == 10);
            break;
    }

    int targetMidi = nearestMidi;
    if (!isAllowed) {
        // Search closest allowed note (either up or down by 1 semitone)
        int downRel = (relativeNote - 1 + 12) % 12;
        int upRel   = (relativeNote + 1) % 12;
        // Check scale membership for neighbors
        auto checkScale = [this](int r) -> bool {
            switch (mParams.scaleType) {
                case SCALE_MAJOR: return (r == 0 || r == 2 || r == 4 || r == 5 || r == 7 || r == 9 || r == 11);
                case SCALE_MINOR: return (r == 0 || r == 2 || r == 3 || r == 5 || r == 7 || r == 8 || r == 10);
                case SCALE_PENTATONIC: return (r == 0 || r == 2 || r == 4 || r == 7 || r == 9);
                case SCALE_BLUES: return (r == 0 || r == 3 || r == 5 || r == 6 || r == 7 || r == 10);
                case SCALE_DORIAN: return (r == 0 || r == 2 || r == 3 || r == 5 || r == 7 || r == 9 || r == 10);
                default: return true;
            }
        };

        if (checkScale(downRel)) {
            targetMidi = nearestMidi - 1;
        } else if (checkScale(upRel)) {
            targetMidi = nearestMidi + 1;
        }
    }

    float targetHz = 440.0f * powf(2.0f, (targetMidi - 69) / 12.0f);
    return detectedPitchHz * (1.0f - mParams.autoTuneAmount) + targetHz * mParams.autoTuneAmount;
}

// ─── True Granular Overlap-Add Pitch Shifter ────────────────────────────────

float StudioMicDSP::processPitchShifter(float inSample, float ratio) {
    if (std::abs(ratio - 1.0f) < 0.005f) {
        return inSample; // Near unity, bypass
    }

    // Write input sample into circular pitch delay buffer
    mPitchDelayBuffer[mPitchWriteIndex] = inSample;

    float speed = 1.0f - ratio;
    mPitchTap1 += speed;
    mPitchTap2 += speed;

    // Wrap taps within pitch window
    while (mPitchTap1 >= mPitchWindowSize) mPitchTap1 -= mPitchWindowSize;
    while (mPitchTap1 < 0.0f)              mPitchTap1 += mPitchWindowSize;
    while (mPitchTap2 >= mPitchWindowSize) mPitchTap2 -= mPitchWindowSize;
    while (mPitchTap2 < 0.0f)              mPitchTap2 += mPitchWindowSize;

    // Triangular crossfading window
    float w1 = 1.0f - fabsf(2.0f * mPitchTap1 / mPitchWindowSize - 1.0f);
    float w2 = 1.0f - fabsf(2.0f * mPitchTap2 / mPitchWindowSize - 1.0f);

    // Read tap 1
    float readPos1 = static_cast<float>(mPitchWriteIndex) - mPitchTap1;
    while (readPos1 < 0.0f) readPos1 += static_cast<float>(PITCH_BUFFER_SIZE);
    int idx1 = static_cast<int>(readPos1) % PITCH_BUFFER_SIZE;
    float s1 = mPitchDelayBuffer[idx1];

    // Read tap 2
    float readPos2 = static_cast<float>(mPitchWriteIndex) - mPitchTap2;
    while (readPos2 < 0.0f) readPos2 += static_cast<float>(PITCH_BUFFER_SIZE);
    int idx2 = static_cast<int>(readPos2) % PITCH_BUFFER_SIZE;
    float s2 = mPitchDelayBuffer[idx2];

    mPitchWriteIndex = (mPitchWriteIndex + 1) % PITCH_BUFFER_SIZE;

    return s1 * w1 + s2 * w2;
}

// ─── Real-Time Audio Buffer Processing Pipeline ─────────────────────────────

void StudioMicDSP::processBuffer(float* buffer, int numFrames, int numChannels) {
    if (mCurrentPreset == PRESET_BYPASS) return;

    // Step A: Real-Time Pitch Detection & True Vocal Pitch Correction
    if (mParams.autoTuneAmount > 0.05f && numFrames >= 256) {
        float detectedHz = detectPitchYin(buffer, numFrames);
        float correctedHz = quantizePitchToScale(detectedHz);
        float ratio = std::clamp(correctedHz / std::max(55.0f, detectedHz), 0.75f, 1.35f);

        // Smooth pitch ratio transitions to avoid clicks
        mTargetShiftRatio = ratio;
        for (int i = 0; i < numFrames; ++i) {
            mCurrentShiftRatio = mCurrentShiftRatio * 0.995f + mTargetShiftRatio * 0.005f;
            for (int ch = 0; ch < numChannels; ++ch) {
                int idx = i * numChannels + ch;
                buffer[idx] = processPitchShifter(buffer[idx], mCurrentShiftRatio);
            }
        }
    }

    // Step B: Studio DSP Channel Strip Pipeline
    for (int i = 0; i < numFrames; ++i) {
        for (int ch = 0; ch < numChannels; ++ch) {
            int idx = i * numChannels + ch;
            buffer[idx] = processSample(buffer[idx], ch);
        }
    }
}

float StudioMicDSP::processSample(float inSample, int channel) {
    float x = inSample;

    // 1. Spectral Downward Expander / Noise Gate
    // Provides transparent, gradual attenuation below threshold to remove room background noise
    float level = fabsf(x);
    mExpanderEnvelope = mExpanderEnvelope * 0.95f + level * 0.05f;
    if (mExpanderEnvelope < mParams.noiseGateThreshold) {
        float ratio = mExpanderEnvelope / std::max(1e-4f, mParams.noiseGateThreshold);
        x *= (ratio * ratio); // Soft downward expansion
    }

    // 2. 18 dB/oct Butterworth High-Pass Filter (removes handling thump and AC rumble)
    float hp1Out = x - mHp1Pole_z;
    mHp1Pole_z = mHp1Pole_z * mHp1Pole_a + x * (1.0f - mHp1Pole_a);
    x = mHpBiquad[channel].process(hp1Out);

    // 3. Acoustic IR Calibration Filter Bank
    // Shapes frequency response toward Neumann U87 / Vintage Tube / Ribbon / Dynamic
    x = mIrFilter1[channel].process(x);
    x = mIrFilter2[channel].process(x);
    x = mIrFilter3[channel].process(x);

    // 4. Dynamic Vocal De-Esser (Dynamic Notch Filter)
    if (mParams.deEsserAmount > 0.01f) {
        float sidechain = fabsf(mDeEsserSidechain[channel].process(x));
        mDeEsserEnv = mDeEsserEnv * 0.90f + sidechain * 0.10f;
        if (mDeEsserEnv > 0.15f) {
            float excess = (mDeEsserEnv - 0.15f) * mParams.deEsserAmount * 3.0f;
            float notchWet = std::clamp(excess, 0.0f, 0.85f);
            float notched = mDeEsserNotch[channel].process(x);
            x = x * (1.0f - notchWet) + notched * notchWet;
        }
    }

    // 5. Analogue Tube Preamp Emulation (Even & Odd Harmonic Saturation)
    if (mParams.tubeWarmth > 0.01f) {
        float drive = 1.0f + mParams.tubeWarmth * 2.2f;
        float driven = x * drive;

        // Asymmetric soft-clipping with 2nd order harmonic bias + 3rd order saturation
        driven += 0.15f * mParams.tubeWarmth * driven * driven; // 2nd harmonic warmth
        if (driven > 1.0f) driven = 1.0f;
        else if (driven < -1.0f) driven = -1.0f;
        else driven = driven - (driven * driven * driven) / 3.0f; // 3rd harmonic saturation

        x = driven / drive;
    }

    // 6. Multi-Band Studio Compressor (Linkwitz-Riley Low / Mid / High Split)
    if (mParams.compRatio > 1.05f) {
        float lowBand  = mCrossoverLp[channel].process(x);
        float highBand = mCrossoverHp[channel].process(x);
        float midBand  = x - lowBand - highBand;

        auto compressBand = [this](float bandSample, float& env, float ratioMultiplier) -> float {
            float envSample = fabsf(bandSample);
            env = env * 0.985f + envSample * 0.015f;
            float levelDb = 20.0f * log10f(std::max(1e-5f, env));
            if (levelDb > mParams.compThresholdDb) {
                float overDb = levelDb - mParams.compThresholdDb;
                float ratio = std::max(1.0f, mParams.compRatio * ratioMultiplier);
                float grDb = overDb * (1.0f - 1.0f / ratio);
                return bandSample * powf(10.0f, -grDb / 20.0f);
            }
            return bandSample;
        };

        float compLow  = compressBand(lowBand,  mCompEnvLow,  1.2f); // Tighter on bass
        float compMid  = compressBand(midBand,  mCompEnvMid,  1.0f);
        float compHigh = compressBand(highBand, mCompEnvHigh, 0.8f); // Gentle on highs

        x = compLow + compMid + compHigh;
    }

    // 7. Parametric Air EQ (High-Shelf Shimmer @ 14 kHz)
    if (mParams.airEqGainDb > 0.05f) {
        x = mAirEq[channel].process(x);
    }

    // 8. Formant Shifter
    if (std::abs(mParams.formantShift) > 0.05f) {
        x = mFormantFilter[channel].process(x);
    }

    // 9. Spatial Vocal Ambience (Stereo Plate Reverb Tank)
    if (mParams.reverbMix > 0.01f) {
        CombFilter* combs = (channel == 0) ? mCombL : mCombR;
        AllPassFilter* allPasses = (channel == 0) ? mAllPassL : mAllPassR;

        float combSum = 0.0f;
        for (int c = 0; c < 4; ++c) {
            combSum += combs[c].process(x);
        }
        combSum *= 0.25f;

        float wet = allPasses[0].process(combSum);
        wet = allPasses[1].process(wet);

        x = x * (1.0f - mParams.reverbMix) + wet * mParams.reverbMix;
    }

    return x;
}
