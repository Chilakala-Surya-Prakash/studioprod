#include "Metronome.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI_MET
#define M_PI_MET 3.14159265358979323846f
#endif

Metronome::Metronome() {
    updateSamplesPerBeat();
}

void Metronome::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;
    updateSamplesPerBeat();
    reset();
}

void Metronome::setBpm(float bpm) {
    mBpm = std::clamp(bpm, 20.0f, 300.0f);
    updateSamplesPerBeat();
}

void Metronome::setTimeSignature(int beatsPerBar, int beatUnit) {
    mBeatsPerBar = std::max(1, beatsPerBar);
    mBeatUnit    = beatUnit;
}

void Metronome::updateSamplesPerBeat() {
    mSamplesPerBeat = (60.0 / static_cast<double>(mBpm)) * mSampleRate;
}

void Metronome::reset() {
    mSampleCounter  = 0.0;
    mCurrentBeat    = 0;
    mClickSamplePos = -1;
}

float Metronome::generateClickSample(int pos, bool accent) {
    if (pos < 0 || pos >= CLICK_DURATION_SAMPLES) return 0.0f;

    // Accent = higher pitched woodblock tone, regular = softer click
    float freq    = accent ? 1500.0f : 1000.0f;
    float env     = expf(-static_cast<float>(pos) / (mSampleRate * 0.008f));
    float tone    = sinf(2.0f * M_PI_MET * freq * pos / mSampleRate);
    float attack  = std::min(1.0f, pos * 8.0f / CLICK_DURATION_SAMPLES);
    return tone * env * attack * mClickVolume * (accent ? 1.0f : 0.75f);
}

void Metronome::processBuffer(float* buffer, int numFrames, int numChannels) {
    if (!mEnabled) return;

    for (int i = 0; i < numFrames; ++i) {
        // Check if we've crossed a beat boundary
        if (mSampleCounter >= mSamplesPerBeat) {
            mSampleCounter -= mSamplesPerBeat;
            mCurrentBeat = (mCurrentBeat + 1) % mBeatsPerBar;
            mClickSamplePos = 0;
            mIsAccentBeat   = (mCurrentBeat == 0);
        }

        float clickSample = 0.0f;
        if (mClickSamplePos >= 0 && mClickSamplePos < CLICK_DURATION_SAMPLES) {
            clickSample = generateClickSample(mClickSamplePos, mIsAccentBeat);
            mClickSamplePos++;
        } else if (mClickSamplePos >= CLICK_DURATION_SAMPLES) {
            mClickSamplePos = -1;
        }

        for (int ch = 0; ch < numChannels; ++ch) {
            buffer[i * numChannels + ch] += clickSample;
        }

        mSampleCounter += 1.0;
    }
}
