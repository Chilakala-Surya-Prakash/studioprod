#ifndef PARAMETRIC_EQ_H
#define PARAMETRIC_EQ_H

#include <cmath>
#include <array>

#ifndef M_PI_EQ
#define M_PI_EQ 3.14159265358979323846f
#endif

struct EQBand {
    enum Type { LOWSHELF = 0, PEAK = 1, HIGHSHELF = 2, LOWPASS = 3, HIGHPASS = 4 };
    Type   type      = PEAK;
    float  frequency = 1000.0f;
    float  gainDb    = 0.0f;
    float  q         = 0.707f;
    bool   enabled   = true;
};

class ParametricEQ {
public:
    static constexpr int NUM_BANDS = 7;

    ParametricEQ();
    void init(int sampleRate);
    void setBand(int idx, const EQBand& band);
    EQBand getBand(int idx) const { return mBands[idx]; }
    void setDefaultBands();
    void processBuffer(float* buffer, int numFrames, int numChannels);

private:
    int mSampleRate = 48000;
    EQBand mBands[NUM_BANDS];

    struct BiquadCoeffs { float b0=1,b1=0,b2=0,a1=0,a2=0; };
    struct BiquadState  { float x1=0,x2=0,y1=0,y2=0;      };

    BiquadCoeffs mCoeffs[NUM_BANDS];
    BiquadState  mState[NUM_BANDS][2];   // [band][channel L/R]

    void  computeCoefficients(int bandIndex);
    float processBiquad(float x, BiquadState& s, const BiquadCoeffs& c);
};

#endif // PARAMETRIC_EQ_H
