package com.example.studioprod.ui

import android.content.Context
import android.os.Environment
import androidx.compose.ui.graphics.Color
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.studioprod.NativeBridge
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

data class TrackModel(
    val id: Int,
    val name: String,
    val color: Color,
    val volume: Float = 0.8f,
    val pan: Float = 0.0f,
    val isMuted: Boolean = false,
    val isSolo: Boolean = false,
    val isArmed: Boolean = false,
    val waveformPoints: List<Float> = emptyList()
)

data class StudioUiState(
    val isRecording: Boolean = false,
    val isPlaying: Boolean = false,
    val bpm: Int = 120,
    val beatsPerBar: Int = 4,
    val beatUnit: Int = 4,
    val currentBeat: Int = 0,
    val momentaryLufs: Float = -60.0f,
    val integratedLufs: Float = -60.0f,
    val monitoringEnabled: Boolean = true,
    val monitoringLevel: Float = 0.9f,

    // Mic DSP
    val selectedPreset: Int = 0,
    val tubeWarmth: Float = 0.65f,
    val deEsser: Float = 0.50f,
    val airEqGainDb: Float = 4.5f,
    val reverbMix: Float = 0.25f,
    val formantShift: Float = 0.0f,

    // Auto-Tune
    val autoTuneAmount: Float = 0.70f,
    val rootKey: Int = 0,
    val scaleType: Int = 1,

    // Guitar Amp & Delay
    val guitarAmpEnabled: Boolean = false,
    val guitarAmpModel: Int = 1,
    val guitarAmpGain: Float = 2.0f,
    val guitarAmpTone: Float = 0.60f,

    val delayEnabled: Boolean = false,
    val delayMode: Int = 1,
    val delayFeedback: Float = 0.40f,
    val delayWetMix: Float = 0.30f,

    // Metronome
    val metronomeEnabled: Boolean = false,
    val metronomeVolume: Float = 0.85f,

    // Synth & Instruments
    val synthWaveform: Int = 1,
    val synthCutoff: Float = 3500.0f,

    // Tracks
    val tracks: List<TrackModel> = emptyList(),

    // Export Feedback
    val exportInProgress: Boolean = false,
    val exportResultMessage: String? = null
)

class StudioViewModel(
    private val bridge: com.example.studioprod.StudioEngineBridge = NativeBridge
) : ViewModel() {

    private val _uiState = MutableStateFlow(StudioUiState())
    val uiState: StateFlow<StudioUiState> = _uiState.asStateFlow()

    private val trackColors = listOf(
        Color(0xFFD500F9), // Purple
        Color(0xFF00E676), // Green
        Color(0xFFFF6D00), // Orange
        Color(0xFF00E5FF), // Cyan
        Color(0xFFFF4081), // Pink
        Color(0xFFFFD600)  // Yellow
    )

    init {
        // Initialize default tracks
        val initialTracks = listOf(
            TrackModel(0, "Lead Vocal", trackColors[0], isArmed = true),
            TrackModel(1, "Guitar / Keys", trackColors[1]),
            TrackModel(2, "Drum Pad / 808", trackColors[2])
        )
        _uiState.update { it.copy(tracks = initialTracks) }

        // Start metering and status polling loop
        startMeteringLoop()
    }

    private fun startMeteringLoop() {
        viewModelScope.launch(Dispatchers.Default) {
            while (isActive) {
                val momLufs = bridge.getMomentaryLufs()
                val intLufs = bridge.getMasterLufs()
                val beat = bridge.getMetronomeBeat()

                _uiState.update { state ->
                    state.copy(
                        momentaryLufs = momLufs,
                        integratedLufs = intLufs,
                        currentBeat = beat
                    )
                }

                // If playing or recording, periodically refresh waveforms for armed tracks
                if (_uiState.value.isRecording || _uiState.value.isPlaying) {
                    refreshWaveforms()
                }

                delay(60) // ~16 FPS polling for smooth VU metering
            }
        }
    }

    private fun refreshWaveforms() {
        val updatedTracks = _uiState.value.tracks.map { track ->
            val wf = bridge.getTrackWaveform(track.id, 50).toList()
            track.copy(waveformPoints = wf)
        }
        _uiState.update { it.copy(tracks = updatedTracks) }
    }

    // ── Transport ────────────────────────────────────────────────────────────

    fun toggleRecording() {
        val nextRec = !_uiState.value.isRecording
        if (nextRec) {
            bridge.startRecording()
            _uiState.update { it.copy(isRecording = true, isPlaying = true) }
        } else {
            bridge.stopRecording()
            _uiState.update { it.copy(isRecording = false) }
            refreshWaveforms()
        }
    }

    fun togglePlayback() {
        val nextPlay = !_uiState.value.isPlaying
        if (nextPlay) {
            bridge.startPlayback()
            _uiState.update { it.copy(isPlaying = true) }
        } else {
            bridge.stopPlayback()
            _uiState.update { it.copy(isPlaying = false) }
        }
    }

    fun stopAll() {
        bridge.stopPlayback()
        bridge.stopRecording()
        _uiState.update { it.copy(isPlaying = false, isRecording = false) }
        refreshWaveforms()
    }

    // ── Session & Tempo ──────────────────────────────────────────────────────

    fun setBpm(bpm: Int) {
        val clamped = bpm.coerceIn(40, 260)
        bridge.setBpm(clamped)
        _uiState.update { it.copy(bpm = clamped) }
    }

    fun tapTempo() {
        bridge.tapTempo()
        // Read back new BPM
        // Note: BPM will also update internally in engine
    }

    fun setTimeSignature(beatsPerBar: Int, beatUnit: Int) {
        bridge.setTimeSignature(beatsPerBar, beatUnit)
        _uiState.update { it.copy(beatsPerBar = beatsPerBar, beatUnit = beatUnit) }
    }

    // ── Mic DSP & Pitch Correction ───────────────────────────────────────────

    fun setMicPreset(presetIndex: Int) {
        bridge.setMicPreset(presetIndex)
        _uiState.update { it.copy(selectedPreset = presetIndex) }
    }

    fun setDspParams(tubeWarmth: Float, deEsser: Float, airEq: Float, reverbMix: Float) {
        bridge.setDSPParams(tubeWarmth, deEsser, airEq, reverbMix)
        _uiState.update {
            it.copy(
                tubeWarmth = tubeWarmth,
                deEsser = deEsser,
                airEqGainDb = airEq,
                reverbMix = reverbMix
            )
        }
    }

    fun setFormantShift(shift: Float) {
        bridge.setFormantShift(shift)
        _uiState.update { it.copy(formantShift = shift) }
    }

    fun setAutoTuneParams(amount: Float, rootKey: Int, scaleType: Int) {
        bridge.setAutoTuneParams(amount, rootKey, scaleType)
        _uiState.update {
            it.copy(
                autoTuneAmount = amount,
                rootKey = rootKey,
                scaleType = scaleType
            )
        }
    }

    fun setMonitoringEnabled(enabled: Boolean) {
        bridge.setMonitoringEnabled(enabled)
        _uiState.update { it.copy(monitoringEnabled = enabled) }
    }

    fun setMonitoringLevel(level: Float) {
        bridge.setMonitoringLevel(level)
        _uiState.update { it.copy(monitoringLevel = level) }
    }

    // ── Metronome & Rack FX ──────────────────────────────────────────────────

    fun toggleMetronome() {
        val nextState = !_uiState.value.metronomeEnabled
        bridge.setMetronomeEnabled(nextState)
        _uiState.update { it.copy(metronomeEnabled = nextState) }
    }

    fun setMetronomeVolume(vol: Float) {
        bridge.setMetronomeVolume(vol)
        _uiState.update { it.copy(metronomeVolume = vol) }
    }

    fun toggleGuitarAmp() {
        val next = !_uiState.value.guitarAmpEnabled
        bridge.setGuitarAmpEnabled(next)
        _uiState.update { it.copy(guitarAmpEnabled = next) }
    }

    fun setGuitarAmpModel(model: Int) {
        bridge.setGuitarAmpModel(model)
        _uiState.update { it.copy(guitarAmpModel = model) }
    }

    fun setGuitarAmpParams(gain: Float, tone: Float) {
        bridge.setGuitarAmpGain(gain)
        bridge.setGuitarAmpTone(tone)
        _uiState.update { it.copy(guitarAmpGain = gain, guitarAmpTone = tone) }
    }

    fun toggleDelay() {
        val next = !_uiState.value.delayEnabled
        bridge.setDelayEnabled(next)
        _uiState.update { it.copy(delayEnabled = next) }
    }

    fun setDelayMode(mode: Int) {
        bridge.setDelayMode(mode)
        _uiState.update { it.copy(delayMode = mode) }
    }

    fun setDelayParams(feedback: Float, wetMix: Float) {
        bridge.setDelayFeedback(feedback)
        bridge.setDelayWetMix(wetMix)
        _uiState.update { it.copy(delayFeedback = feedback, delayWetMix = wetMix) }
    }

    // ── Virtual Instruments ──────────────────────────────────────────────────

    fun triggerDrumPad(index: Int, velocity: Float = 0.9f) {
        bridge.triggerDrumPad(index, velocity)
    }

    fun noteOn(midiNote: Int, velocity: Float = 0.85f) {
        bridge.noteOn(midiNote, velocity)
    }

    fun noteOff(midiNote: Int) {
        bridge.noteOff(midiNote)
    }

    fun setSynthWaveform(waveform: Int) {
        bridge.setSynthWaveform(waveform)
        _uiState.update { it.copy(synthWaveform = waveform) }
    }

    fun setSynthCutoff(cutoffHz: Float) {
        bridge.setSynthCutoff(cutoffHz)
        _uiState.update { it.copy(synthCutoff = cutoffHz) }
    }

    // ── Track Management & Editing ───────────────────────────────────────────

    fun addTrack(name: String? = null) {
        val currentTracks = _uiState.value.tracks
        val newId = currentTracks.size
        val trackName = name ?: "Track ${newId + 1}"
        val color = trackColors[newId % trackColors.size]

        bridge.createTrack(trackName)
        val newTrack = TrackModel(newId, trackName, color)
        _uiState.update { it.copy(tracks = currentTracks + newTrack) }
    }

    fun deleteTrack(trackId: Int) {
        bridge.deleteTrack(trackId)
        val filtered = _uiState.value.tracks.filter { it.id != trackId }
        _uiState.update { it.copy(tracks = filtered) }
    }

    fun updateTrack(updatedTrack: TrackModel) {
        bridge.setTrackVolume(updatedTrack.id, updatedTrack.volume)
        bridge.setTrackPan(updatedTrack.id, updatedTrack.pan)
        bridge.setTrackMute(updatedTrack.id, updatedTrack.isMuted)
        bridge.setTrackSolo(updatedTrack.id, updatedTrack.isSolo)
        bridge.armTrackForRecord(updatedTrack.id, updatedTrack.isArmed)

        _uiState.update { state ->
            val updated = state.tracks.map { if (it.id == updatedTrack.id) updatedTrack else it }
            state.copy(tracks = updated)
        }
    }

    fun trimTrack(trackId: Int, startFrame: Long, endFrame: Long) {
        bridge.trimTrack(trackId, startFrame, endFrame)
        refreshWaveforms()
    }

    fun splitTrack(trackId: Int, splitFrame: Long) {
        bridge.splitTrack(trackId, splitFrame)
        refreshWaveforms()
    }

    // ── Master Export ────────────────────────────────────────────────────────

    fun exportRelease(context: Context, exportStems: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            _uiState.update { it.copy(exportInProgress = true, exportResultMessage = null) }

            try {
                val outputDir = context.getExternalFilesDir(Environment.DIRECTORY_MUSIC)
                    ?: context.filesDir

                if (!outputDir.exists()) outputDir.mkdirs()

                val timeStamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault()).format(Date())
                val masterFile = File(outputDir, "StudioProd_Master_$timeStamp.wav")

                val success = bridge.exportMasterWav(masterFile.absolutePath, exportStems)

                if (success) {
                    val msg = if (exportStems) {
                        "Exported Master & Stems to:\n${masterFile.name}"
                    } else {
                        "Exported Master WAV to:\n${masterFile.name}"
                    }
                    _uiState.update {
                        it.copy(exportInProgress = false, exportResultMessage = msg)
                    }
                } else {
                    _uiState.update {
                        it.copy(exportInProgress = false, exportResultMessage = "Export failed (no recorded audio in session).")
                    }
                }
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(exportInProgress = false, exportResultMessage = "Export error: ${e.message}")
                }
            }
        }
    }

    fun clearExportMessage() {
        _uiState.update { it.copy(exportResultMessage = null) }
    }
}
