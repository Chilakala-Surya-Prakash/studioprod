#ifndef VIRTUAL_SYNTH_ENGINE_H
#define VIRTUAL_SYNTH_ENGINE_H

#include <vector>
#include <cmath>

enum WaveformType {
    WAVE_SINE = 0,
    WAVE_SAW = 1,
    WAVE_SQUARE = 2,
    WAVE_TRIANGLE = 3,
    WAVE_FM = 4
};

struct SynthParams {
    WaveformType waveform = WAVE_SAW;
    float attackSec  = 0.01f;
    float decaySec   = 0.20f;
    float sustainLvl = 0.70f;
    float releaseSec = 0.35f;
    float cutoffHz   = 2500.0f;
    float resonance  = 1.5f;
    float fmRatio    = 2.0f;
    float fmDepth    = 1.5f;
};

class VirtualSynthEngine {
public:
    VirtualSynthEngine();
    ~VirtualSynthEngine();

    void init(int sampleRate);
    void setParams(const SynthParams& params) { mParams = params; }
    SynthParams getParams() const { return mParams; }
    void setWaveform(WaveformType wave) { mParams.waveform = wave; }
    void setCutoff(float cutoffHz) { mParams.cutoffHz = cutoffHz; }

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void process(float* buffer, int numFrames, int numChannels);

private:
    int mSampleRate = 48000;
    SynthParams mParams;

    enum EnvelopeStage { STAGE_OFF, STAGE_ATTACK, STAGE_DECAY, STAGE_SUSTAIN, STAGE_RELEASE };

    struct Voice {
        int note = -1;
        float phase = 0.0f;
        float fmPhase = 0.0f;
        float freq = 440.0f;
        float velocity = 0.0f;
        float envLevel = 0.0f;
        EnvelopeStage stage = STAGE_OFF;
        bool active = false;

        // Resonant Lowpass State
        float filterZ1 = 0.0f;
        float filterZ2 = 0.0f;
    };

    std::vector<Voice> mVoices;

    float generateOscillator(Voice& voice);
    float processResonantLP(float input, Voice& voice, float cutoffHz, float Q);
};

#endif // VIRTUAL_SYNTH_ENGINE_H
