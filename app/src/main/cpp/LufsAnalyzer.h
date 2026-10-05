#ifndef LUFS_ANALYZER_H
#define LUFS_ANALYZER_H

#include <vector>
#include <cmath>

// ITU-R BS.1770-4 Integrated Loudness (LUFS) Analyzer
// Implements K-weighting filter + mean square gating
class LufsAnalyzer {
public:
    LufsAnalyzer();
    void init(int sampleRate);
    void reset();

    // Feed audio samples — returns momentary loudness
    float processSamples(const float* buffer, int numFrames, int numChannels);

    float getMomentaryLufs() const  { return mMomentaryLufs; }
    float getIntegratedLufs() const { return mIntegratedLufs; }
    float getTruePeakDb() const     { return mTruePeakDb; }

private:
    int mSampleRate = 48000;

    // K-Weighting Stage 1: High-shelf pre-filter  (+4dB @ 1.5kHz)
    float ks1_b0=1, ks1_b1=0, ks1_b2=0, ks1_a1=0, ks1_a2=0;
    float ks1_x1[2]={}, ks1_x2[2]={}, ks1_y1[2]={}, ks1_y2[2]={};

    // K-Weighting Stage 2: High-pass filter (100 Hz)
    float ks2_b0=1, ks2_b1=0, ks2_b2=0, ks2_a1=0, ks2_a2=0;
    float ks2_x1[2]={}, ks2_x2[2]={}, ks2_y1[2]={}, ks2_y2[2]={};

    void computeKWeightingFilters();
    float applyKWeighting(float x, int ch);

    // Block-based analysis (400ms blocks, 75% overlap)
    static constexpr int BLOCK_FRAMES = 400 * 48; // 400ms at 48kHz – scaled per sample rate
    int  mBlockSize = 19200;
    std::vector<float> mBlockL;
    std::vector<float> mBlockR;
    int  mBlockPos = 0;

    // Loudness history for integrated gating
    std::vector<float> mBlockHistory;
    static constexpr float GATE_THRESHOLD_ABS = -70.0f;  // absolute gate
    static constexpr float GATE_THRESHOLD_REL = -10.0f;  // relative gate

    float mMomentaryLufs = -60.0f;
    float mIntegratedLufs= -60.0f;
    float mTruePeakDb    = -60.0f;

    void analyzeBlock();
    float loudnessFromMeanSquare(float msL, float msR);
};

#endif // LUFS_ANALYZER_H
