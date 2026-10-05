#ifndef STUDIO_MIC_DSP_H
#define STUDIO_MIC_DSP_H

#include <vector>
#include <cmath>
#include <algorithm>

enum MicPreset {
    PRESET_NEUMANN_U87 = 0,
    PRESET_VINTAGE_TUBE = 1,
    PRESET_RIBBON_WARMTH = 2,
    PRESET_STAGE_DYNAMIC = 3,
    PRESET_BYPASS = 4
};

enum ScaleType {
    SCALE_CHROMATIC = 0,
    SCALE_MAJOR = 1,
    SCALE_MINOR = 2,
    SCALE_PENTATONIC = 3,
    SCALE_BLUES = 4,
    SCALE_DORIAN = 5
};

struct DSPParams {
    float hpCutoffHz = 85.0f;
    float tubeWarmth = 0.65f;          // 0.0 to 1.0 (Harmonic saturation)
    float deEsserAmount = 0.5f;        // 0.0 to 1.0 (Dynamic sibilance control)
    float airEqGainDb = 4.5f;          // High-shelf boost @ 14kHz
    float compThresholdDb = -18.0f;    // Multi-band compression threshold
    float compRatio = 4.0f;            // Compression ratio
    float reverbMix = 0.25f;           // 0.0 to 1.0 (Stereo plate reverb)
    float noiseGateThreshold = 0.015f; // Noise expander threshold
    float formantShift = 0.0f;         // -1.0 (Deeper) to +1.0 (Brighter pop)

    // Vocal Pitch Correction (Auto-Tune)
    float autoTuneAmount = 0.70f;      // 0.0 (Off) to 1.0 (Hard Auto-Tune)
    int rootKeyNote = 0;               // 0 = C, 1 = C#, ..., 11 = B
    ScaleType scaleType = SCALE_MAJOR;
};

// Second-order IIR Biquad filter
struct Biquad {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;

    void reset() {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    inline float process(float in) {
        float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = in;
        y2 = y1;
        y1 = out;
        return out;
    }

    void setPeaking(float freq, float gainDb, float q, float sampleRate);
    void setHighShelf(float freq, float gainDb, float q, float sampleRate);
    void setLowShelf(float freq, float gainDb, float q, float sampleRate);
    void setBandpass(float freq, float q, float sampleRate);
    void setNotch(float freq, float q, float sampleRate);
    void setLowpass(float freq, float q, float sampleRate);
    void setHighpass(float freq, float q, float sampleRate);
};

// Comb Filter for Schroeder Reverb
struct CombFilter {
    std::vector<float> buffer;
    int bufferSize = 0;
    int bufferIdx = 0;
    float feedback = 0.8f;
    float filterStore = 0.0f;
    float damp = 0.2f;

    void init(int size, float fb, float d);
    inline float process(float in);
};

// All-Pass Filter for Reverb Diffusion
struct AllPassFilter {
    std::vector<float> buffer;
    int bufferSize = 0;
    int bufferIdx = 0;
    float feedback = 0.5f;

    void init(int size, float fb = 0.5f);
    inline float process(float in);
};

class StudioMicDSP {
public:
    StudioMicDSP();
    ~StudioMicDSP();

    void init(int sampleRate);
    void setPreset(MicPreset preset);
    void setParams(const DSPParams& params);
    DSPParams getParams() const { return mParams; }

    // Real-time 32-bit float audio buffer processing (in-place)
    void processBuffer(float* buffer, int numFrames, int numChannels);

    // Fundamental pitch detection
    float detectPitchYin(const float* buffer, int numFrames);
    float quantizePitchToScale(float detectedPitchHz);

private:
    int mSampleRate = 48000;
    MicPreset mCurrentPreset = PRESET_NEUMANN_U87;
    DSPParams mParams;

    // 1. 18 dB/oct Butterworth High-Pass Filter (Cascaded 1-pole + 2-pole Biquad)
    float mHp1Pole_z = 0.0f;
    float mHp1Pole_a = 0.0f;
    Biquad mHpBiquad[2]; // Stereo HP biquad
    void updateHPFFilter();

    // 2. Acoustic IR Calibration Filter Bank
    // Compensates for mobile phone MEMS microphone resonance and emulates target mic frequency response
    Biquad mIrFilter1[2]; // Low warmth / proximity
    Biquad mIrFilter2[2]; // Mid resonance tame
    Biquad mIrFilter3[2]; // High air shimmer
    void updateAcousticIRFilters();

    // 3. Spectral Noise Expander / Gate
    float mExpanderEnvelope = 0.0f;

    // 4. Dynamic Vocal De-Esser (Bandpass sidechain + Dynamic Notch)
    Biquad mDeEsserSidechain[2];
    Biquad mDeEsserNotch[2];
    float mDeEsserEnv = 0.0f;

    // 5. Multi-Band Studio Compressor (Linkwitz-Riley 2nd order crossover)
    Biquad mCrossoverLp[2]; // Low band (< 250Hz)
    Biquad mCrossoverHp[2]; // High band (> 4000Hz)
    float mCompEnvLow = 0.0f;
    float mCompEnvMid = 0.0f;
    float mCompEnvHigh = 0.0f;

    // 6. Parametric Air EQ (High-Shelf @ 14kHz)
    Biquad mAirEq[2];

    // 7. Formant Shifter Filter
    Biquad mFormantFilter[2];

    // 8. Stereo Plate Reverb Tank (4 Comb Filters + 2 All-Pass Filters per channel)
    CombFilter mCombL[4];
    CombFilter mCombR[4];
    AllPassFilter mAllPassL[2];
    AllPassFilter mAllPassR[2];

    // 9. True Real-Time Granular / Overlap-Add Pitch Shifter
    static constexpr int PITCH_BUFFER_SIZE = 4096;
    float mPitchDelayBuffer[PITCH_BUFFER_SIZE];
    int mPitchWriteIndex = 0;
    float mPitchTap1 = 0.0f;
    float mPitchTap2 = 0.0f;
    float mPitchWindowSize = 1024.0f;
    float mCurrentShiftRatio = 1.0f;
    float mTargetShiftRatio = 1.0f;

    float processPitchShifter(float inSample, float ratio);

    // Single sample processing pipeline
    float processSample(float inSample, int channel);
};

#endif // STUDIO_MIC_DSP_H
