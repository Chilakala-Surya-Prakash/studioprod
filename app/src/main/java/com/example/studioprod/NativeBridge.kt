package com.example.studioprod

interface StudioEngineBridge {
    fun initNativeEngine()
    fun stopNativeEngine()
    fun setBpm(bpm: Int)
    fun tapTempo()
    fun setTimeSignature(beatsPerBar: Int, beatUnit: Int)
    fun startRecording()
    fun stopRecording()
    fun startPlayback()
    fun stopPlayback()
    fun seekToFrame(frame: Long)
    fun getPlaybackPosition(): Long
    fun getTotalFrames(): Long
    fun setMonitoringEnabled(enabled: Boolean)
    fun setMonitoringLevel(level: Float)
    fun setMicPreset(presetIndex: Int)
    fun setDSPParams(tubeWarmth: Float, deEsserAmount: Float, airEqGainDb: Float, reverbMix: Float)
    fun setFormantShift(shift: Float)
    fun setAutoTuneParams(amount: Float, rootKey: Int, scaleType: Int)
    fun triggerDrumPad(padIndex: Int, velocity: Float)
    fun noteOn(midiNote: Int, velocity: Float)
    fun noteOff(midiNote: Int)
    fun setSynthWaveform(waveformIndex: Int)
    fun setSynthCutoff(cutoffHz: Float)
    fun setMetronomeEnabled(enabled: Boolean)
    fun setMetronomeVolume(vol: Float)
    fun getMetronomeBeat(): Int
    fun setGuitarAmpEnabled(enabled: Boolean)
    fun setGuitarAmpModel(model: Int)
    fun setGuitarAmpGain(gain: Float)
    fun setGuitarAmpTone(tone: Float)
    fun setDelayEnabled(enabled: Boolean)
    fun setDelayMode(mode: Int)
    fun setDelayFeedback(feedback: Float)
    fun setDelayWetMix(wetMix: Float)
    fun setEQBand(bandIndex: Int, type: Int, freq: Float, gainDb: Float, q: Float, enabled: Boolean)
    fun createTrack(name: String): Int
    fun deleteTrack(trackId: Int)
    fun clearTrackBuffer(trackId: Int)
    fun setTrackVolume(trackId: Int, volume: Float)
    fun setTrackPan(trackId: Int, pan: Float)
    fun setTrackMute(trackId: Int, mute: Boolean)
    fun setTrackSolo(trackId: Int, solo: Boolean)
    fun armTrackForRecord(trackId: Int, arm: Boolean)
    fun getTrackWaveform(trackId: Int, numPoints: Int): FloatArray
    fun trimTrack(trackId: Int, startFrame: Long, endFrame: Long): Boolean
    fun splitTrack(trackId: Int, splitFrame: Long): Boolean
    fun getMasterLufs(): Float
    fun getMomentaryLufs(): Float
    fun exportMasterWav(filePath: String, exportStems: Boolean): Boolean
}

object NativeBridge : StudioEngineBridge {
    init {
        try {
            System.loadLibrary("studioprod_native")
        } catch (e: UnsatisfiedLinkError) {
            e.printStackTrace()
        }
    }

    // Engine Lifecycle & Session
    external override fun initNativeEngine()
    external override fun stopNativeEngine()
    external override fun setBpm(bpm: Int)
    external override fun tapTempo()
    external override fun setTimeSignature(beatsPerBar: Int, beatUnit: Int)

    // Transport
    external override fun startRecording()
    external override fun stopRecording()
    external override fun startPlayback()
    external override fun stopPlayback()
    external override fun seekToFrame(frame: Long)
    external override fun getPlaybackPosition(): Long
    external override fun getTotalFrames(): Long

    // Monitoring
    external override fun setMonitoringEnabled(enabled: Boolean)
    external override fun setMonitoringLevel(level: Float)

    // Mic DSP & Pitch Correction
    external override fun setMicPreset(presetIndex: Int)
    external override fun setDSPParams(tubeWarmth: Float, deEsserAmount: Float, airEqGainDb: Float, reverbMix: Float)
    external override fun setFormantShift(shift: Float)
    external override fun setAutoTuneParams(amount: Float, rootKey: Int, scaleType: Int)

    // Virtual Instruments
    external override fun triggerDrumPad(padIndex: Int, velocity: Float)
    external override fun noteOn(midiNote: Int, velocity: Float)
    external override fun noteOff(midiNote: Int)
    external override fun setSynthWaveform(waveformIndex: Int)
    external override fun setSynthCutoff(cutoffHz: Float)

    // Metronome
    external override fun setMetronomeEnabled(enabled: Boolean)
    external override fun setMetronomeVolume(vol: Float)
    external override fun getMetronomeBeat(): Int

    // Guitar Amp Simulator
    external override fun setGuitarAmpEnabled(enabled: Boolean)
    external override fun setGuitarAmpModel(model: Int)
    external override fun setGuitarAmpGain(gain: Float)
    external override fun setGuitarAmpTone(tone: Float)

    // Stereo Ping-Pong / Tape Delay
    external override fun setDelayEnabled(enabled: Boolean)
    external override fun setDelayMode(mode: Int)
    external override fun setDelayFeedback(feedback: Float)
    external override fun setDelayWetMix(wetMix: Float)

    // 7-Band Parametric EQ
    external override fun setEQBand(bandIndex: Int, type: Int, freq: Float, gainDb: Float, q: Float, enabled: Boolean)

    // Dynamic Track Management
    external override fun createTrack(name: String): Int
    external override fun deleteTrack(trackId: Int)
    external override fun clearTrackBuffer(trackId: Int)
    external override fun setTrackVolume(trackId: Int, volume: Float)
    external override fun setTrackPan(trackId: Int, pan: Float)
    external override fun setTrackMute(trackId: Int, mute: Boolean)
    external override fun setTrackSolo(trackId: Int, solo: Boolean)
    external override fun armTrackForRecord(trackId: Int, arm: Boolean)

    // Track Editing & Waveforms
    external override fun getTrackWaveform(trackId: Int, numPoints: Int): FloatArray
    external override fun trimTrack(trackId: Int, startFrame: Long, endFrame: Long): Boolean
    external override fun splitTrack(trackId: Int, splitFrame: Long): Boolean

    // Metering
    external override fun getMasterLufs(): Float
    external override fun getMomentaryLufs(): Float

    // Export
    external override fun exportMasterWav(filePath: String, exportStems: Boolean): Boolean
}
