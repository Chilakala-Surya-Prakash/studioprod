#include "VirtualSynthEngine.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI_SYNTH
#define M_PI_SYNTH 3.14159265358979323846f
#endif

VirtualSynthEngine::VirtualSynthEngine() {
    init(48000);
}

VirtualSynthEngine::~VirtualSynthEngine() {}

void VirtualSynthEngine::init(int sampleRate) {
    mSampleRate = sampleRate > 0 ? sampleRate : 48000;
    mVoices.resize(16);
}

void VirtualSynthEngine::noteOn(int midiNote, float velocity) {
    float freq = 440.0f * powf(2.0f, (midiNote - 69) / 12.0f);
    
    // Check if note is already playing (retrigger)
    for (auto& voice : mVoices) {
        if (voice.active && voice.note == midiNote) {
            voice.freq = freq;
            voice.velocity = std::clamp(velocity, 0.0f, 1.0f);
            voice.stage = STAGE_ATTACK;
            return;
        }
    }

    // Allocate an inactive voice
    for (auto& voice : mVoices) {
        if (!voice.active || voice.stage == STAGE_OFF) {
            voice.note = midiNote;
            voice.freq = freq;
            voice.velocity = std::clamp(velocity, 0.0f, 1.0f);
            voice.phase = 0.0f;
            voice.fmPhase = 0.0f;
            voice.envLevel = 0.0f;
            voice.stage = STAGE_ATTACK;
            voice.filterZ1 = 0.0f;
            voice.filterZ2 = 0.0f;
            voice.active = true;
            break;
        }
    }
}

void VirtualSynthEngine::noteOff(int midiNote) {
    for (auto& voice : mVoices) {
        if (voice.active && voice.note == midiNote) {
            voice.stage = STAGE_RELEASE;
        }
    }
}

float VirtualSynthEngine::generateOscillator(Voice& voice) {
    float normPhase = voice.phase / (2.0f * M_PI_SYNTH);
    float sample = 0.0f;

    switch (mParams.waveform) {
        case WAVE_SINE:
            sample = sinf(voice.phase);
            break;
        case WAVE_SAW:
            sample = 2.0f * normPhase - 1.0f;
            break;
        case WAVE_SQUARE:
            sample = (normPhase < 0.5f) ? 0.8f : -0.8f;
            break;
        case WAVE_TRIANGLE:
            sample = (normPhase < 0.5f) ? (4.0f * normPhase - 1.0f) : (3.0f - 4.0f * normPhase);
            break;
        case WAVE_FM: {
            float modFreq = voice.freq * mParams.fmRatio;
            voice.fmPhase += 2.0f * M_PI_SYNTH * modFreq / mSampleRate;
            if (voice.fmPhase >= 2.0f * M_PI_SYNTH) voice.fmPhase -= 2.0f * M_PI_SYNTH;

            float modulator = sinf(voice.fmPhase) * mParams.fmDepth;
            sample = sinf(voice.phase + modulator);
            break;
        }
    }

    // Advance main phase
    voice.phase += 2.0f * M_PI_SYNTH * voice.freq / mSampleRate;
    if (voice.phase >= 2.0f * M_PI_SYNTH) voice.phase -= 2.0f * M_PI_SYNTH;

    return sample;
}

float VirtualSynthEngine::processResonantLP(float input, Voice& voice, float cutoffHz, float Q) {
    float f = 2.0f * sinf(M_PI_SYNTH * cutoffHz / mSampleRate);
    float q = 1.0f / Q;

    voice.filterZ1 += f * (input - voice.filterZ1 - q * voice.filterZ2);
    voice.filterZ2 += f * voice.filterZ1;

    return voice.filterZ2;
}

void VirtualSynthEngine::process(float* buffer, int numFrames, int numChannels) {
    float attackRate  = 1.0f / std::max(0.001f, mParams.attackSec * mSampleRate);
    float decayRate   = (1.0f - mParams.sustainLvl) / std::max(0.001f, mParams.decaySec * mSampleRate);
    float releaseRate = mParams.sustainLvl / std::max(0.001f, mParams.releaseSec * mSampleRate);

    for (int i = 0; i < numFrames; ++i) {
        float mixL = 0.0f, mixR = 0.0f;

        for (auto& voice : mVoices) {
            if (!voice.active || voice.stage == STAGE_OFF) continue;

            // ADSR Envelope State Machine
            switch (voice.stage) {
                case STAGE_ATTACK:
                    voice.envLevel += attackRate;
                    if (voice.envLevel >= 1.0f) {
                        voice.envLevel = 1.0f;
                        voice.stage = STAGE_DECAY;
                    }
                    break;
                case STAGE_DECAY:
                    voice.envLevel -= decayRate;
                    if (voice.envLevel <= mParams.sustainLvl) {
                        voice.envLevel = mParams.sustainLvl;
                        voice.stage = STAGE_SUSTAIN;
                    }
                    break;
                case STAGE_SUSTAIN:
                    voice.envLevel = mParams.sustainLvl;
                    break;
                case STAGE_RELEASE:
                    voice.envLevel -= releaseRate;
                    if (voice.envLevel <= 0.0f) {
                        voice.envLevel = 0.0f;
                        voice.stage = STAGE_OFF;
                        voice.active = false;
                    }
                    break;
                case STAGE_OFF:
                    break;
            }

            if (voice.envLevel <= 0.0001f && voice.stage == STAGE_OFF) continue;

            float rawOsc = generateOscillator(voice);

            // Dynamically sweep filter cutoff with envelope
            float dynamicCutoff = mParams.cutoffHz * (0.3f + 0.7f * voice.envLevel);
            dynamicCutoff = std::clamp(dynamicCutoff, 100.0f, 18000.0f);

            float filtered = processResonantLP(rawOsc, voice, dynamicCutoff, mParams.resonance);
            float sample   = filtered * voice.envLevel * voice.velocity * 0.35f;

            mixL += sample;
            mixR += sample;
        }

        buffer[i * numChannels]     += mixL;
        if (numChannels > 1) buffer[i * numChannels + 1] += mixR;
    }
}
