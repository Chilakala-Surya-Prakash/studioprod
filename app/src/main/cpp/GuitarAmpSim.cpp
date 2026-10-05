#include "GuitarAmpSim.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI_AMP
#define M_PI_AMP 3.14159265358979323846f
#endif

GuitarAmpSim::GuitarAmpSim() {}

void GuitarAmpSim::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;
}

void GuitarAmpSim::setModel(AmpModel model) {
    mModel = model;
    switch (model) {
        case AMP_CLEAN:   mInputGain = 1.0f; mTone = 0.65f; break;
        case AMP_CRUNCH:  mInputGain = 2.0f; mTone = 0.60f; break;
        case AMP_HEAVY:   mInputGain = 3.5f; mTone = 0.45f; break;
        case AMP_BASS:    mInputGain = 1.8f; mTone = 0.35f; break;
    }
}

// Waveshaping transfer functions ─────────────────────────────────────────────

float GuitarAmpSim::waveshapeClean(float x) {
    // Fender-style: soft asymmetric clipping
    if (x > 0.8f)       return 0.8f + (x - 0.8f) * 0.15f;
    else if (x < -0.6f) return -0.6f + (x + 0.6f) * 0.1f;
    return x;
}

float GuitarAmpSim::waveshapeCrunch(float x) {
    // Marshall crunch: tanh with asymmetric bias
    float biased = x + 0.08f;  // small bias for even harmonics
    return tanhf(biased * 2.0f) * 0.7f;
}

float GuitarAmpSim::waveshapeHeavy(float x) {
    // Hard clip for high gain
    float driven = x * 4.0f;
    if (driven >  1.0f) driven =  1.0f - (driven - 1.0f) * 0.05f;
    if (driven < -1.0f) driven = -1.0f - (driven + 1.0f) * 0.05f;
    return tanhf(driven) * 0.6f;
}

float GuitarAmpSim::waveshapeBass(float x) {
    // Bass amp: warmer, preserve low-end with mild distortion
    return tanhf(x * 1.6f) * 0.75f + x * 0.1f;
}

// Tone stack ──────────────────────────────────────────────────────────────────

float GuitarAmpSim::processToneStack(float x, int ch) {
    // Simple shelving tone stack: cut highs when tone < 0.5, boost when > 0.5
    float cutoffLP = 800.0f + mTone * 6000.0f;  // 800–6800 Hz
    float alphaLP  = 2.0f * M_PI_AMP * cutoffLP / mSampleRate;
    float coef     = expf(-alphaLP);

    // First-order LP
    mLowState[ch].z1 = mLowState[ch].z1 * coef + x * (1.0f - coef);
    float lp = mLowState[ch].z1;
    float hp = x - lp;

    // Mix LP and HP based on tone knob
    return lp * (1.0f - mTone * 0.5f) + hp * (mTone * 0.8f);
}

// Cabinet Simulation (approximate 1×12 cabinet frequency response) ────────────

float GuitarAmpSim::processCabSim(float x, int ch) {
    // Low-pass for cabinet (removes harsh highs above 5 kHz)
    float cutoff = 5000.0f;
    float alpha  = 2.0f * M_PI_AMP * cutoff / mSampleRate;
    float coef   = expf(-alpha);
    mCabState[ch].z1 = mCabState[ch].z1 * coef + x * (1.0f - coef);

    // Add slight 300Hz resonance
    float res300Hz = 2.0f * M_PI_AMP * 300.0f / mSampleRate;
    float c300 = expf(-res300Hz);
    mCabState[ch].z2 = mCabState[ch].z2 * c300 + mCabState[ch].z1 * (1.0f - c300);
    return mCabState[ch].z1 * 0.9f + mCabState[ch].z2 * 0.1f;
}

float GuitarAmpSim::processSample(float x, int ch) {
    // 1. Input gain stage
    x *= mInputGain;

    // 2. Waveshaping distortion
    switch (mModel) {
        case AMP_CLEAN:   x = waveshapeClean(x);   break;
        case AMP_CRUNCH:  x = waveshapeCrunch(x);  break;
        case AMP_HEAVY:   x = waveshapeHeavy(x);   break;
        case AMP_BASS:    x = waveshapeBass(x);     break;
    }

    // 3. Tone stack
    x = processToneStack(x, ch);

    // 4. Cabinet simulation
    x = processCabSim(x, ch);

    // 5. Output level
    return x * mOutputLevel;
}

void GuitarAmpSim::processBuffer(float* buffer, int numFrames, int numChannels) {
    if (!mEnabled) return;
    for (int i = 0; i < numFrames; ++i) {
        buffer[i * numChannels] = processSample(buffer[i * numChannels], 0);
        if (numChannels > 1)
            buffer[i * numChannels + 1] = processSample(buffer[i * numChannels + 1], 1);
    }
}
