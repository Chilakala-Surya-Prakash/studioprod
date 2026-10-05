#include "LufsAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>

#ifndef M_PI_LUFS
#define M_PI_LUFS 3.14159265358979323846f
#endif

LufsAnalyzer::LufsAnalyzer() {}

void LufsAnalyzer::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;
    // Block size: 400ms
    mBlockSize = static_cast<int>(0.4 * mSampleRate);
    mBlockL.resize(mBlockSize, 0.0f);
    mBlockR.resize(mBlockSize, 0.0f);
    mBlockPos = 0;
    mBlockHistory.clear();
    computeKWeightingFilters();
    reset();
}

void LufsAnalyzer::computeKWeightingFilters() {
    float fs = static_cast<float>(mSampleRate);

    // Stage 1: High-shelf (+4dB pre-filter per BS.1770 Table 1)
    // Coefficients for 48kHz, scaled to other sample rates
    double Vh = 1.58486;
    double Vb = 1.52433;
    double pb = 0.99004745;
    double a0s1 = 1.0 + 0.45952 / fs;
    double b0s1 = (Vh + Vb * pb + Vb) / a0s1;
    double b1s1 = (-2.0 * Vh - 2.0 * Vb * pb) / a0s1;
    double b2s1 = (Vh - Vb * pb + Vb) / a0s1;
    // Simplified: use known 48kHz coefficients from BS.1770
    ks1_b0 =  1.53512485958697f;
    ks1_b1 = -2.69169618940638f;
    ks1_b2 =  1.19839281085285f;
    ks1_a1 = -1.69065929318241f;
    ks1_a2 =  0.73248077421585f;

    // Stage 2: High-pass at 38Hz (per BS.1770 Table 2)
    ks2_b0 =  1.0f;
    ks2_b1 = -2.0f;
    ks2_b2 =  1.0f;
    ks2_a1 = -1.99004745483398f;
    ks2_a2 =  0.99007225036621f;
}

float LufsAnalyzer::applyKWeighting(float x, int ch) {
    // Stage 1
    float y1 = ks1_b0*x + ks1_b1*ks1_x1[ch] + ks1_b2*ks1_x2[ch]
              - ks1_a1*ks1_y1[ch] - ks1_a2*ks1_y2[ch];
    ks1_x2[ch]=ks1_x1[ch]; ks1_x1[ch]=x;
    ks1_y2[ch]=ks1_y1[ch]; ks1_y1[ch]=y1;

    // Stage 2
    float y2 = ks2_b0*y1 + ks2_b1*ks2_x1[ch] + ks2_b2*ks2_x2[ch]
              - ks2_a1*ks2_y1[ch] - ks2_a2*ks2_y2[ch];
    ks2_x2[ch]=ks2_x1[ch]; ks2_x1[ch]=y1;
    ks2_y2[ch]=ks2_y1[ch]; ks2_y1[ch]=y2;
    return y2;
}

float LufsAnalyzer::loudnessFromMeanSquare(float msL, float msR) {
    float meanSquare = (msL + msR) / 2.0f;  // average channels
    if (meanSquare < 1e-10f) return -70.0f;
    return -0.691f + 10.0f * log10f(meanSquare);
}

void LufsAnalyzer::analyzeBlock() {
    float msL = 0.0f, msR = 0.0f;
    for (int i = 0; i < mBlockSize; ++i) {
        msL += mBlockL[i] * mBlockL[i];
        msR += mBlockR[i] * mBlockR[i];
    }
    msL /= mBlockSize;
    msR /= mBlockSize;

    float blockLoudness = loudnessFromMeanSquare(msL, msR);
    mMomentaryLufs = blockLoudness;

    // Only store blocks above absolute gate
    if (blockLoudness > GATE_THRESHOLD_ABS) {
        mBlockHistory.push_back(blockLoudness);
    }

    // Integrated LUFS with relative gate
    if (mBlockHistory.size() > 0) {
        float sumPow = 0.0f;
        int count = 0;
        for (float lb : mBlockHistory) {
            sumPow += powf(10.0f, lb / 10.0f);
            count++;
        }
        float meanPow = sumPow / count;
        float avgLufs = 10.0f * log10f(meanPow + 1e-10f) - 0.691f;
        float relGate = avgLufs + GATE_THRESHOLD_REL;

        // Re-compute with relative gate
        float sumPow2 = 0.0f;
        int count2 = 0;
        for (float lb : mBlockHistory) {
            if (lb > relGate) {
                sumPow2 += powf(10.0f, lb / 10.0f);
                count2++;
            }
        }
        if (count2 > 0) {
            mIntegratedLufs = 10.0f * log10f(sumPow2 / count2) - 0.691f;
        }
    }
}

float LufsAnalyzer::processSamples(const float* buffer, int numFrames, int numChannels) {
    float truePeakMax = 0.0f;

    for (int i = 0; i < numFrames; ++i) {
        float sL = buffer[i * numChannels];
        float sR = (numChannels > 1) ? buffer[i * numChannels + 1] : sL;

        // True peak tracking
        truePeakMax = std::max(truePeakMax, std::max(std::abs(sL), std::abs(sR)));

        // K-weighting
        mBlockL[mBlockPos] = applyKWeighting(sL, 0);
        mBlockR[mBlockPos] = applyKWeighting(sR, 1);

        mBlockPos++;
        if (mBlockPos >= mBlockSize) {
            analyzeBlock();
            // 75% overlap: shift block by 25%
            int overlap = mBlockSize / 4;
            for (int j = 0; j < mBlockSize - overlap; ++j) {
                mBlockL[j] = mBlockL[j + overlap];
                mBlockR[j] = mBlockR[j + overlap];
            }
            mBlockPos = mBlockSize - overlap;
        }
    }

    if (truePeakMax > 1e-6f) {
        mTruePeakDb = 20.0f * log10f(truePeakMax);
    }

    return mMomentaryLufs;
}

void LufsAnalyzer::reset() {
    mMomentaryLufs  = -60.0f;
    mIntegratedLufs = -60.0f;
    mTruePeakDb     = -60.0f;
    mBlockPos       = 0;
    mBlockHistory.clear();
    std::fill(ks1_x1, ks1_x1+2, 0.0f);
    std::fill(ks1_x2, ks1_x2+2, 0.0f);
    std::fill(ks1_y1, ks1_y1+2, 0.0f);
    std::fill(ks1_y2, ks1_y2+2, 0.0f);
    std::fill(ks2_x1, ks2_x1+2, 0.0f);
    std::fill(ks2_x2, ks2_x2+2, 0.0f);
    std::fill(ks2_y1, ks2_y1+2, 0.0f);
    std::fill(ks2_y2, ks2_y2+2, 0.0f);
}
