#include "ParametricEQ.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI_EQ
#define M_PI_EQ 3.14159265358979323846f
#endif

ParametricEQ::ParametricEQ() {
    setDefaultBands();
}

void ParametricEQ::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;
    for (int i = 0; i < NUM_BANDS; ++i)
        computeCoefficients(i);
}

void ParametricEQ::setDefaultBands() {
    // Typical mastering / vocal EQ curve
    mBands[0] = { EQBand::HIGHPASS,  80.0f,   0.0f, 0.707f, true };  // HPF
    mBands[1] = { EQBand::LOWSHELF, 120.0f,  -1.5f, 0.707f, true };  // Sub cut
    mBands[2] = { EQBand::PEAK,     350.0f,  -2.0f, 1.2f,   true };  // Mud cut
    mBands[3] = { EQBand::PEAK,    1200.0f,   1.5f, 0.8f,   true };  // Presence
    mBands[4] = { EQBand::PEAK,    3500.0f,   2.0f, 0.9f,   true };  // Attack
    mBands[5] = { EQBand::PEAK,    8000.0f,   2.5f, 1.0f,   true };  // Air
    mBands[6] = { EQBand::HIGHSHELF,12000.0f, 3.0f, 0.707f, true };  // Shimmer
}

void ParametricEQ::setBand(int idx, const EQBand& band) {
    if (idx < 0 || idx >= NUM_BANDS) return;
    mBands[idx] = band;
    computeCoefficients(idx);
}

void ParametricEQ::computeCoefficients(int i) {
    const EQBand& b = mBands[i];
    BiquadCoeffs& c = mCoeffs[i];

    float omega = 2.0f * M_PI_EQ * b.frequency / static_cast<float>(mSampleRate);
    float cosw  = cosf(omega);
    float sinw  = sinf(omega);
    float alpha = sinw / (2.0f * b.q);
    float A     = powf(10.0f, b.gainDb / 40.0f);   // sqrt(10^(dB/20))
    float a0;

    switch (b.type) {
        case EQBand::PEAK: {
            float alphaA = alpha * A;
            float alphaDivA = alpha / A;
            a0   = 1.0f + alphaDivA;
            c.b0 = (1.0f + alphaA)  / a0;
            c.b1 = (-2.0f * cosw)   / a0;
            c.b2 = (1.0f - alphaA)  / a0;
            c.a1 = (-2.0f * cosw)   / a0;
            c.a2 = (1.0f - alphaDivA) / a0;
            break;
        }
        case EQBand::LOWSHELF: {
            float sqrtA = sqrtf(A);
            a0   = (A+1) + (A-1)*cosw + 2.0f*sqrtA*alpha;
            c.b0 = A * ((A+1) - (A-1)*cosw + 2.0f*sqrtA*alpha) / a0;
            c.b1 = 2.0f * A * ((A-1) - (A+1)*cosw)             / a0;
            c.b2 = A * ((A+1) - (A-1)*cosw - 2.0f*sqrtA*alpha) / a0;
            c.a1 = -2.0f * ((A-1) + (A+1)*cosw)                / a0;
            c.a2 = ((A+1) + (A-1)*cosw - 2.0f*sqrtA*alpha)     / a0;
            break;
        }
        case EQBand::HIGHSHELF: {
            float sqrtA = sqrtf(A);
            a0   = (A+1) - (A-1)*cosw + 2.0f*sqrtA*alpha;
            c.b0 = A * ((A+1) + (A-1)*cosw + 2.0f*sqrtA*alpha) / a0;
            c.b1 = -2.0f * A * ((A-1) + (A+1)*cosw)            / a0;
            c.b2 = A * ((A+1) + (A-1)*cosw - 2.0f*sqrtA*alpha) / a0;
            c.a1 = 2.0f * ((A-1) - (A+1)*cosw)                 / a0;
            c.a2 = ((A+1) - (A-1)*cosw - 2.0f*sqrtA*alpha)     / a0;
            break;
        }
        case EQBand::LOWPASS: {
            a0   = 1.0f + alpha;
            c.b0 = (1.0f - cosw) / 2.0f / a0;
            c.b1 = (1.0f - cosw) / a0;
            c.b2 = c.b0;
            c.a1 = (-2.0f * cosw) / a0;
            c.a2 = (1.0f - alpha) / a0;
            break;
        }
        case EQBand::HIGHPASS: {
            a0   = 1.0f + alpha;
            c.b0 = (1.0f + cosw) / 2.0f / a0;
            c.b1 = -(1.0f + cosw) / a0;
            c.b2 = c.b0;
            c.a1 = (-2.0f * cosw) / a0;
            c.a2 = (1.0f - alpha) / a0;
            break;
        }
    }
}

float ParametricEQ::processBiquad(float x, BiquadState& s, const BiquadCoeffs& c) {
    float y = c.b0*x + c.b1*s.x1 + c.b2*s.x2 - c.a1*s.y1 - c.a2*s.y2;
    s.x2 = s.x1; s.x1 = x;
    s.y2 = s.y1; s.y1 = y;
    return y;
}

void ParametricEQ::processBuffer(float* buffer, int numFrames, int numChannels) {
    for (int i = 0; i < numFrames; ++i) {
        for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
            float sample = buffer[i * numChannels + ch];
            for (int b = 0; b < NUM_BANDS; ++b) {
                if (mBands[b].enabled)
                    sample = processBiquad(sample, mState[b][ch], mCoeffs[b]);
            }
            buffer[i * numChannels + ch] = sample;
        }
    }
}
