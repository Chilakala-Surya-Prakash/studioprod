#ifndef GUITAR_AMP_SIM_H
#define GUITAR_AMP_SIM_H

#include <cmath>
#include <array>

enum AmpModel {
    AMP_CLEAN   = 0,  // Fender-style clean
    AMP_CRUNCH  = 1,  // Marshall-style crunch
    AMP_HEAVY   = 2,  // High-gain metal
    AMP_BASS    = 3   // Bass amp with low-mid scoop
};

class GuitarAmpSim {
public:
    GuitarAmpSim();
    void init(int sampleRate);

    void setModel(AmpModel model);
    void setGain(float gain)        { mInputGain = std::clamp(gain, 0.0f, 4.0f); }
    void setTone(float tone)        { mTone = std::clamp(tone, 0.0f, 1.0f); }
    void setOutputLevel(float lvl)  { mOutputLevel = std::clamp(lvl, 0.0f, 2.0f); }
    void setEnabled(bool en)        { mEnabled = en; }
    bool isEnabled() const          { return mEnabled; }

    void processBuffer(float* buffer, int numFrames, int numChannels);

private:
    int     mSampleRate  = 48000;
    AmpModel mModel      = AMP_CRUNCH;
    float   mInputGain   = 1.5f;
    float   mTone        = 0.6f;
    float   mOutputLevel = 0.8f;
    bool    mEnabled     = false;

    // Tone stack biquad (mid-scooped EQ)
    struct BiquadState { float z1=0,z2=0,x1=0,x2=0; };
    BiquadState mLowState[2];
    BiquadState mHighState[2];
    BiquadState mCabState[2];   // Cabinet IR approximation

    float processSample(float x, int ch);
    float waveshapeClean(float x);
    float waveshapeCrunch(float x);
    float waveshapeHeavy(float x);
    float waveshapeBass(float x);
    float processToneStack(float x, int ch);
    float processCabSim(float x, int ch);
};

#endif // GUITAR_AMP_SIM_H
