#ifndef DRUM_SYNTH_ENGINE_H
#define DRUM_SYNTH_ENGINE_H

#include <vector>
#include <cmath>
#include <array>

// Each drum pad has a unique synthesis model:
// KICK: Sine with pitch envelope (808-style) + click transient
// SNARE: Sine body + white noise burst  
// HIHAT: Band-passed white noise
// CLAP: Multiple noise bursts with comb filter
// PERC: Metallic FM tone
// BASS: Sub sine with harmonic distortion
// VOCAL_CHOP: Formant filter noise
// CYMBAL: Band-limited noise with long decay

enum PadType {
    PAD_808_KICK    = 0,
    PAD_SNARE       = 1,
    PAD_HIHAT_CLOSED= 2,
    PAD_CLAP        = 3,
    PAD_DEEP_BASS   = 4,
    PAD_SYNTH_LEAD  = 5,
    PAD_VOCAL_CHOP  = 6,
    PAD_CRASH       = 7
};

class DrumSynthEngine {
public:
    static constexpr int NUM_PADS = 8;

    DrumSynthEngine();
    void init(int sampleRate);
    void triggerPad(int padIndex, float velocity);
    void process(float* buffer, int numFrames, int numChannels);

private:
    int mSampleRate = 48000;

    struct PadVoice {
        bool   active       = false;
        int    samplePos    = 0;
        float  velocity     = 0.9f;
        PadType type        = PAD_808_KICK;
        int    maxSamples   = 0;

        // Synthesis state variables
        float phase         = 0.0f;
        float phase2        = 0.0f;  // secondary oscillator
        float pitchHz       = 60.0f;
        float pitchEnvStart = 200.0f;
        float noiseState    = 0.0f;

        // Simple HP/LP filter state for noise
        float filterZ1      = 0.0f;
        float filterZ2      = 0.0f;
    };

    PadVoice mVoices[NUM_PADS];

    float generateSample(PadVoice& voice);
    float whiteNoise(float& state);
    float processLP(float x, float& z, float cutoff);
    float processHP(float x, float& z, float cutoff);
};

#endif // DRUM_SYNTH_ENGINE_H
