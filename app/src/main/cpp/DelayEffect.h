#ifndef DELAY_EFFECT_H
#define DELAY_EFFECT_H

#include <vector>
#include <cmath>
#include <algorithm>

enum DelayMode {
    DELAY_STEREO    = 0,  // Both channels same delay
    DELAY_PING_PONG = 1,  // L->R->L ping pong
    DELAY_TAPE      = 2   // Tape-style with wow/flutter
};

class DelayEffect {
public:
    DelayEffect();
    void init(int sampleRate);

    void setBpm(float bpm)           { mBpm = bpm; updateDelayTime(); }
    void setNoteDivision(int div)    { mNoteDivision = div; updateDelayTime(); }
    void setFeedback(float fb)       { mFeedback = std::clamp(fb, 0.0f, 0.95f); }
    void setWetMix(float wet)        { mWetMix = std::clamp(wet, 0.0f, 1.0f); }
    void setMode(DelayMode mode)     { mMode = mode; }
    void setEnabled(bool en)         { mEnabled = en; }
    bool isEnabled() const           { return mEnabled; }

    void processBuffer(float* buffer, int numFrames, int numChannels);

private:
    int  mSampleRate  = 48000;
    float mBpm        = 120.0f;
    int  mNoteDivision= 4;   // 4 = quarter note, 8 = eighth, etc.
    float mFeedback   = 0.4f;
    float mWetMix     = 0.30f;
    bool  mEnabled    = false;
    DelayMode mMode   = DELAY_PING_PONG;

    // Delay buffer (stereo)
    std::vector<float> mDelayBufL;
    std::vector<float> mDelayBufR;
    int  mWritePosL   = 0;
    int  mWritePosR   = 0;
    int  mDelaySamples= 0;

    // Tape wow/flutter state
    float mFlutterPhase = 0.0f;

    void updateDelayTime();
    float readDelay(const std::vector<float>& buf, int writePos, int delaySamples, float flutter = 0.0f);
};

#endif // DELAY_EFFECT_H
