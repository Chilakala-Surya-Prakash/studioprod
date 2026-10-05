#include "DelayEffect.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI_DLY
#define M_PI_DLY 3.14159265358979323846f
#endif

DelayEffect::DelayEffect() {}

void DelayEffect::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;
    int maxDelaySamples = mSampleRate * 2;  // 2 second max
    mDelayBufL.assign(maxDelaySamples, 0.0f);
    mDelayBufR.assign(maxDelaySamples, 0.0f);
    mWritePosL = 0;
    mWritePosR = 0;
    updateDelayTime();
}

void DelayEffect::updateDelayTime() {
    // Quarter note delay at BPM, divided by note division factor
    double beatsPerSec = mBpm / 60.0;
    double secondsPerQuarter = 1.0 / beatsPerSec;
    double delaySeconds = secondsPerQuarter * (4.0 / mNoteDivision);
    mDelaySamples = static_cast<int>(delaySeconds * mSampleRate);
    mDelaySamples = std::clamp(mDelaySamples, 100, static_cast<int>(mDelayBufL.size()) - 1);
}

float DelayEffect::readDelay(const std::vector<float>& buf, int writePos, int delaySamples, float flutter) {
    int bufSize = static_cast<int>(buf.size());
    // Flutter: modulate read position slightly for tape effect
    float flutterOffset = flutter * 5.0f;  // max ±5 samples flutter
    float readPosF = static_cast<float>(writePos) - delaySamples + flutterOffset;
    while (readPosF < 0) readPosF += bufSize;
    while (readPosF >= bufSize) readPosF -= bufSize;

    // Linear interpolation for smooth read
    int   readI = static_cast<int>(readPosF);
    float frac  = readPosF - readI;
    int   nextI = (readI + 1) % bufSize;
    return buf[readI] * (1.0f - frac) + buf[nextI] * frac;
}

void DelayEffect::processBuffer(float* buffer, int numFrames, int numChannels) {
    if (!mEnabled) return;

    for (int i = 0; i < numFrames; ++i) {
        float inL = buffer[i * numChannels];
        float inR = (numChannels > 1) ? buffer[i * numChannels + 1] : inL;

        float flutter = 0.0f;
        if (mMode == DELAY_TAPE) {
            mFlutterPhase += 2.0f * M_PI_DLY * 0.7f / mSampleRate;  // 0.7 Hz flutter
            if (mFlutterPhase > 2.0f * M_PI_DLY) mFlutterPhase -= 2.0f * M_PI_DLY;
            flutter = sinf(mFlutterPhase) * 0.4f;
        }

        float delayedL = readDelay(mDelayBufL, mWritePosL, mDelaySamples, flutter);
        float delayedR = readDelay(mDelayBufR, mWritePosR, mDelaySamples, flutter);

        float feedL, feedR;
        if (mMode == DELAY_PING_PONG) {
            // Cross-feed: L writes to R's delay, R writes to L's delay
            feedL = inL + delayedR * mFeedback;
            feedR = inR + delayedL * mFeedback;
        } else {
            feedL = inL + delayedL * mFeedback;
            feedR = inR + delayedR * mFeedback;
        }

        // Soft clip feedback to prevent runaway
        feedL = tanhf(feedL * 0.9f);
        feedR = tanhf(feedR * 0.9f);

        mDelayBufL[mWritePosL] = feedL;
        mDelayBufR[mWritePosR] = feedR;

        mWritePosL = (mWritePosL + 1) % static_cast<int>(mDelayBufL.size());
        mWritePosR = (mWritePosR + 1) % static_cast<int>(mDelayBufR.size());

        float dryL = inL * (1.0f - mWetMix);
        float dryR = inR * (1.0f - mWetMix);

        buffer[i * numChannels]     = dryL + delayedL * mWetMix;
        if (numChannels > 1) buffer[i * numChannels + 1] = dryR + delayedR * mWetMix;
    }
}
