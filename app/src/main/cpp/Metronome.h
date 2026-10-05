#ifndef METRONOME_H
#define METRONOME_H

#include <functional>

class Metronome {
public:
    Metronome();
    void init(int sampleRate);

    void setBpm(float bpm);
    float getBpm() const { return mBpm; }

    void setTimeSignature(int beatsPerBar, int beatUnit);
    void setEnabled(bool enabled) { mEnabled = enabled; }
    bool isEnabled() const { return mEnabled; }

    void setClickVolume(float vol) { mClickVolume = vol; }

    // Renders click samples into output buffer (mixes in-place)
    void processBuffer(float* buffer, int numFrames, int numChannels);

    // Returns current beat index (0-based) for UI flash
    int  getCurrentBeat() const { return mCurrentBeat; }
    int  getBeatsPerBar() const { return mBeatsPerBar; }

    void reset();

private:
    int   mSampleRate   = 48000;
    float mBpm          = 120.0f;
    int   mBeatsPerBar  = 4;
    int   mBeatUnit     = 4;
    bool  mEnabled      = false;
    float mClickVolume  = 0.85f;

    // Playback state
    double mSamplesPerBeat   = 0.0;
    double mSampleCounter    = 0.0;
    int    mCurrentBeat      = 0;

    // Click synthesis state
    int    mClickSamplePos   = -1;  // -1 = idle
    bool   mIsAccentBeat     = false;
    static constexpr int CLICK_DURATION_SAMPLES = 1800;  // ~37ms at 48kHz

    void updateSamplesPerBeat();
    float generateClickSample(int pos, bool accent);
};

#endif // METRONOME_H
