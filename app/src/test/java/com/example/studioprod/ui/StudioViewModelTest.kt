package com.example.studioprod.ui

import com.example.studioprod.StudioEngineBridge
import junit.framework.TestCase.assertEquals
import junit.framework.TestCase.assertFalse
import junit.framework.TestCase.assertTrue
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Before
import org.junit.Test

class FakeStudioEngineBridge : StudioEngineBridge {
    var isEngineRunning = false
    var currentBpm = 120
    var recording = false
    var playing = false
    var metronomeOn = false
    var guitarAmpOn = false
    var delayOn = false
    var currentMicPreset = 0
    var tracksCreated = mutableListOf<String>()
    var tracksDeleted = mutableListOf<Int>()

    override fun initNativeEngine() { isEngineRunning = true }
    override fun stopNativeEngine() { isEngineRunning = false }
    override fun setBpm(bpm: Int) { currentBpm = bpm }
    override fun tapTempo() { currentBpm = 128 }
    override fun setTimeSignature(beatsPerBar: Int, beatUnit: Int) {}
    override fun startRecording() { recording = true; playing = true }
    override fun stopRecording() { recording = false }
    override fun startPlayback() { playing = true }
    override fun stopPlayback() { playing = false }
    override fun seekToFrame(frame: Long) {}
    override fun getPlaybackPosition(): Long = 0L
    override fun getTotalFrames(): Long = 48000L
    override fun setMonitoringEnabled(enabled: Boolean) {}
    override fun setMonitoringLevel(level: Float) {}
    override fun setMicPreset(presetIndex: Int) { currentMicPreset = presetIndex }
    override fun setDSPParams(tubeWarmth: Float, deEsserAmount: Float, airEqGainDb: Float, reverbMix: Float) {}
    override fun setFormantShift(shift: Float) {}
    override fun setAutoTuneParams(amount: Float, rootKey: Int, scaleType: Int) {}
    override fun triggerDrumPad(padIndex: Int, velocity: Float) {}
    override fun noteOn(midiNote: Int, velocity: Float) {}
    override fun noteOff(midiNote: Int) {}
    override fun setSynthWaveform(waveformIndex: Int) {}
    override fun setSynthCutoff(cutoffHz: Float) {}
    override fun setMetronomeEnabled(enabled: Boolean) { metronomeOn = enabled }
    override fun setMetronomeVolume(vol: Float) {}
    override fun getMetronomeBeat(): Int = 0
    override fun setGuitarAmpEnabled(enabled: Boolean) { guitarAmpOn = enabled }
    override fun setGuitarAmpModel(model: Int) {}
    override fun setGuitarAmpGain(gain: Float) {}
    override fun setGuitarAmpTone(tone: Float) {}
    override fun setDelayEnabled(enabled: Boolean) { delayOn = enabled }
    override fun setDelayMode(mode: Int) {}
    override fun setDelayFeedback(feedback: Float) {}
    override fun setDelayWetMix(wetMix: Float) {}
    override fun setEQBand(bandIndex: Int, type: Int, freq: Float, gainDb: Float, q: Float, enabled: Boolean) {}
    override fun createTrack(name: String): Int {
        tracksCreated.add(name)
        return tracksCreated.size - 1
    }
    override fun deleteTrack(trackId: Int) { tracksDeleted.add(trackId) }
    override fun clearTrackBuffer(trackId: Int) {}
    override fun setTrackVolume(trackId: Int, volume: Float) {}
    override fun setTrackPan(trackId: Int, pan: Float) {}
    override fun setTrackMute(trackId: Int, mute: Boolean) {}
    override fun setTrackSolo(trackId: Int, solo: Boolean) {}
    override fun armTrackForRecord(trackId: Int, arm: Boolean) {}
    override fun getTrackWaveform(trackId: Int, numPoints: Int): FloatArray = FloatArray(numPoints) { 0.5f }
    override fun trimTrack(trackId: Int, startFrame: Long, endFrame: Long): Boolean = true
    override fun splitTrack(trackId: Int, splitFrame: Long): Boolean = true
    override fun getMasterLufs(): Float = -14.2f
    override fun getMomentaryLufs(): Float = -13.8f
    override fun exportMasterWav(filePath: String, exportStems: Boolean): Boolean = true
}

@OptIn(ExperimentalCoroutinesApi::class)
class StudioViewModelTest {

    private val testDispatcher = StandardTestDispatcher()
    private lateinit var fakeBridge: FakeStudioEngineBridge
    private lateinit var viewModel: StudioViewModel

    @Before
    fun setUp() {
        Dispatchers.setMain(testDispatcher)
        fakeBridge = FakeStudioEngineBridge()
        viewModel = StudioViewModel(fakeBridge)
    }

    @After
    fun tearDown() {
        Dispatchers.resetMain()
    }

    @Test
    fun initialState_hasDefaultTracksAndTempo() = runTest {
        val state = viewModel.uiState.value
        assertEquals(3, state.tracks.size)
        assertEquals("Lead Vocal", state.tracks[0].name)
        assertTrue(state.tracks[0].isArmed)
        assertEquals(120, state.bpm)
        assertFalse(state.isRecording)
        assertFalse(state.isPlaying)
    }

    @Test
    fun toggleRecording_startsRecordingAndPlayback() = runTest {
        viewModel.toggleRecording()
        val state = viewModel.uiState.value
        assertTrue(state.isRecording)
        assertTrue(state.isPlaying)
        assertTrue(fakeBridge.recording)
        assertTrue(fakeBridge.playing)

        viewModel.toggleRecording()
        assertFalse(viewModel.uiState.value.isRecording)
    }

    @Test
    fun togglePlayback_togglesPlayState() = runTest {
        viewModel.togglePlayback()
        assertTrue(viewModel.uiState.value.isPlaying)
        assertTrue(fakeBridge.playing)

        viewModel.togglePlayback()
        assertFalse(viewModel.uiState.value.isPlaying)
        assertFalse(fakeBridge.playing)
    }

    @Test
    fun setBpm_updatesUiStateAndBridge() = runTest {
        viewModel.setBpm(135)
        assertEquals(135, viewModel.uiState.value.bpm)
        assertEquals(135, fakeBridge.currentBpm)
    }

    @Test
    fun setMicPreset_updatesPresetInBridge() = runTest {
        viewModel.setMicPreset(1) // Vintage Tube
        assertEquals(1, viewModel.uiState.value.selectedPreset)
        assertEquals(1, fakeBridge.currentMicPreset)
    }

    @Test
    fun setDspParams_updatesAllParameters() = runTest {
        viewModel.setDspParams(0.85f, 0.40f, 6.0f, 0.35f)
        val state = viewModel.uiState.value
        assertEquals(0.85f, state.tubeWarmth)
        assertEquals(0.40f, state.deEsser)
        assertEquals(6.0f, state.airEqGainDb)
        assertEquals(0.35f, state.reverbMix)
    }

    @Test
    fun addAndDeleteTrack_modifiesTrackList() = runTest {
        viewModel.addTrack("Synth Lead 2")
        assertEquals(4, viewModel.uiState.value.tracks.size)
        assertEquals("Synth Lead 2", viewModel.uiState.value.tracks[3].name)

        viewModel.deleteTrack(1)
        val tracks = viewModel.uiState.value.tracks
        assertEquals(3, tracks.size)
        assertFalse(tracks.any { it.id == 1 })
    }

    @Test
    fun rackEffects_toggleProperly() = runTest {
        viewModel.toggleGuitarAmp()
        assertTrue(viewModel.uiState.value.guitarAmpEnabled)
        assertTrue(fakeBridge.guitarAmpOn)

        viewModel.toggleDelay()
        assertTrue(viewModel.uiState.value.delayEnabled)
        assertTrue(fakeBridge.delayOn)

        viewModel.toggleMetronome()
        assertTrue(viewModel.uiState.value.metronomeEnabled)
        assertTrue(fakeBridge.metronomeOn)
    }
}
