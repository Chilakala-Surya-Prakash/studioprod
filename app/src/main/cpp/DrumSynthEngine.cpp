#include "DrumSynthEngine.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

#ifndef M_PI_DRUM
#define M_PI_DRUM 3.14159265358979323846f
#endif

DrumSynthEngine::DrumSynthEngine() {}

void DrumSynthEngine::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;
}

float DrumSynthEngine::whiteNoise(float& state) {
    // Park-Miller LCG pseudo-random noise
    state = state * 1664525.0f + 1013904223.0f;
    union { float f; unsigned int i; } u;
    u.i = (static_cast<unsigned int>(state) >> 8) | 0x3F800000u;
    return u.f - 1.5f;  // range ~[-0.5, 0.5]
}

float DrumSynthEngine::processLP(float x, float& z, float cutoff) {
    float alpha = cutoff / (mSampleRate + cutoff);
    z = z + alpha * (x - z);
    return z;
}

float DrumSynthEngine::processHP(float x, float& z, float cutoff) {
    float alpha = cutoff / (mSampleRate + cutoff);
    float lp    = z + alpha * (x - z);
    z = lp;
    return x - lp;
}

void DrumSynthEngine::triggerPad(int padIndex, float velocity) {
    if (padIndex < 0 || padIndex >= NUM_PADS) return;
    PadVoice& v = mVoices[padIndex];

    v.active    = true;
    v.samplePos = 0;
    v.velocity  = std::clamp(velocity, 0.0f, 1.0f);
    v.type      = static_cast<PadType>(padIndex);
    v.phase     = 0.0f;
    v.phase2    = 0.0f;
    v.filterZ1  = 0.0f;
    v.filterZ2  = 0.0f;
    v.noiseState= static_cast<float>(padIndex * 12345 + 67890);

    switch (v.type) {
        case PAD_808_KICK:
            v.pitchHz    = 55.0f;
            v.pitchEnvStart = 220.0f;
            v.maxSamples = mSampleRate * 8 / 10;   // 800ms
            break;
        case PAD_SNARE:
            v.pitchHz    = 180.0f;
            v.maxSamples = mSampleRate * 3 / 10;   // 300ms
            break;
        case PAD_HIHAT_CLOSED:
            v.maxSamples = mSampleRate / 8;         // 125ms
            break;
        case PAD_CLAP:
            v.maxSamples = mSampleRate * 4 / 10;   // 400ms
            break;
        case PAD_DEEP_BASS:
            v.pitchHz    = 40.0f;
            v.pitchEnvStart = 120.0f;
            v.maxSamples = mSampleRate * 12 / 10;  // 1.2s
            break;
        case PAD_SYNTH_LEAD:
            v.pitchHz    = 440.0f;
            v.maxSamples = mSampleRate * 6 / 10;   // 600ms
            break;
        case PAD_VOCAL_CHOP:
            v.pitchHz    = 220.0f;
            v.maxSamples = mSampleRate * 5 / 10;   // 500ms
            break;
        case PAD_CRASH:
            v.maxSamples = mSampleRate * 2;         // 2s
            break;
    }
}

float DrumSynthEngine::generateSample(PadVoice& v) {
    float t     = static_cast<float>(v.samplePos);
    float norm  = t / static_cast<float>(std::max(1, v.maxSamples));
    float env   = 0.0f;
    float sample= 0.0f;

    switch (v.type) {
        case PAD_808_KICK: {
            // Exponential pitch glide + sine body
            float pitchDecay = expf(-t * 15.0f / mSampleRate);
            float freq = v.pitchHz + (v.pitchEnvStart - v.pitchHz) * pitchDecay;
            v.phase += 2.0f * M_PI_DRUM * freq / mSampleRate;
            env     = expf(-t * 5.0f / mSampleRate);
            float click = expf(-t * 200.0f / mSampleRate) * 0.5f;  // punch transient
            sample  = (sinf(v.phase) * env + click) * v.velocity * 0.9f;
            break;
        }
        case PAD_SNARE: {
            // Sine body + noise burst
            v.phase += 2.0f * M_PI_DRUM * v.pitchHz / mSampleRate;
            env     = expf(-t * 18.0f / mSampleRate);
            float body  = sinf(v.phase) * env * 0.4f;
            float noise = whiteNoise(v.noiseState) * expf(-t * 12.0f / mSampleRate) * 0.6f;
            sample  = (body + noise) * v.velocity;
            break;
        }
        case PAD_HIHAT_CLOSED: {
            // Band-passed white noise (high-pass heavy)
            float n = whiteNoise(v.noiseState);
            n = processHP(n, v.filterZ1, 8000.0f);
            env = expf(-t * 50.0f / mSampleRate);
            sample = n * env * v.velocity * 0.5f;
            break;
        }
        case PAD_CLAP: {
            // Multiple noise bursts (comb effect)
            float n = whiteNoise(v.noiseState);
            // 3 burst impulses: t=0, t=10ms, t=18ms
            int t1 = static_cast<int>(0.010f * mSampleRate);
            int t2 = static_cast<int>(0.018f * mSampleRate);
            float burst = expf(-t * 40.0f / mSampleRate);
            if (v.samplePos >= t1 && v.samplePos < t1 + 800) burst += expf(-(t - t1) * 60.0f / mSampleRate) * 0.7f;
            if (v.samplePos >= t2 && v.samplePos < t2 + 1200) burst += expf(-(t - t2) * 35.0f / mSampleRate) * 0.9f;
            sample = n * burst * v.velocity * 0.6f;
            break;
        }
        case PAD_DEEP_BASS: {
            // Sub-bass sine with slight 2nd harmonic distortion
            float pitchDecay = expf(-t * 4.0f / mSampleRate);
            float freq = v.pitchHz + (v.pitchEnvStart - v.pitchHz) * pitchDecay;
            v.phase += 2.0f * M_PI_DRUM * freq / mSampleRate;
            v.phase2 += 2.0f * M_PI_DRUM * freq * 2.0f / mSampleRate;
            env = expf(-t * 3.0f / mSampleRate);
            float sub = sinf(v.phase) * 0.8f + sinf(v.phase2) * 0.15f;
            // Soft clip for sub warmth
            sub = tanhf(sub * 1.5f) / 1.5f;
            sample = sub * env * v.velocity * 0.85f;
            break;
        }
        case PAD_SYNTH_LEAD: {
            // Sawtooth-ish via multiple harmonics
            v.phase += 2.0f * M_PI_DRUM * v.pitchHz / mSampleRate;
            float saw = sinf(v.phase) + sinf(v.phase * 2.0f) * 0.5f + sinf(v.phase * 3.0f) * 0.25f;
            // Simple LP filter for tone
            saw = processLP(saw, v.filterZ1, 3000.0f * (1.0f - norm * 0.7f));
            env = expf(-t * 6.0f / mSampleRate);
            sample = (saw / 1.75f) * env * v.velocity * 0.7f;
            break;
        }
        case PAD_VOCAL_CHOP: {
            // Formant-filtered noise with pitch
            float n = whiteNoise(v.noiseState);
            v.phase += 2.0f * M_PI_DRUM * v.pitchHz / mSampleRate;
            float tone = sinf(v.phase) * 0.4f;
            n = processLP(n, v.filterZ1, 1200.0f);
            n = processHP(n, v.filterZ2, 300.0f);
            env = expf(-t * 8.0f / mSampleRate) * (1.0f - expf(-t * 60.0f / mSampleRate));
            sample = (tone + n * 0.6f) * env * v.velocity * 0.7f;
            break;
        }
        case PAD_CRASH: {
            // Long noise decay with high-pass
            float n = whiteNoise(v.noiseState);
            n = processHP(n, v.filterZ1, 4000.0f);
            // Shimmer: gentle modulation
            float shimmer = sinf(2.0f * M_PI_DRUM * 1800.0f * t / mSampleRate);
            env = expf(-t * 1.5f / mSampleRate);
            sample = (n + shimmer * 0.05f) * env * v.velocity * 0.45f;
            break;
        }
    }

    return std::clamp(sample, -1.0f, 1.0f);
}

void DrumSynthEngine::process(float* buffer, int numFrames, int numChannels) {
    for (int i = 0; i < numFrames; ++i) {
        float mixL = 0.0f, mixR = 0.0f;

        for (int p = 0; p < NUM_PADS; ++p) {
            PadVoice& v = mVoices[p];
            if (!v.active) continue;

            float s = generateSample(v);

            // Slight stereo spread per pad
            float panL = 1.0f - (p % 4) * 0.08f;
            float panR = 1.0f - (3 - p % 4) * 0.08f;
            mixL += s * panL;
            mixR += s * panR;

            v.samplePos++;
            if (v.samplePos >= v.maxSamples) v.active = false;
        }

        buffer[i * numChannels]     += mixL;
        if (numChannels > 1) buffer[i * numChannels + 1] += mixR;
    }
}
