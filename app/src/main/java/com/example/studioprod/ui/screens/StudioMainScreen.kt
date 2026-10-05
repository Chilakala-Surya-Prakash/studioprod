package com.example.studioprod.ui.screens

import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.*
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.itemsIndexed
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.studioprod.ui.StudioUiState
import com.example.studioprod.ui.StudioViewModel
import com.example.studioprod.ui.TrackModel

// ─── Studio Color Tokens ──────────────────────────────────────────────────────
val StudioBackground = Color(0xFF0A0C10)
val StudioSurface    = Color(0xFF141720)
val StudioCard       = Color(0xFF1C1F2E)
val StudioCardBorder = Color(0xFF2A2D3E)
val StudioAccent     = Color(0xFF00E5FF)
val StudioRed        = Color(0xFFFF1744)
val StudioAmber      = Color(0xFFFFAB00)
val StudioGreen      = Color(0xFF00E676)
val StudioPurple     = Color(0xFFD500F9)
val StudioOrange     = Color(0xFFFF6D00)
val StudioTeal       = Color(0xFF1DE9B6)

// ─── Root Screen ──────────────────────────────────────────────────────────────
@Composable
fun StudioMainScreen(
    viewModel: StudioViewModel = viewModel()
) {
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()
    val context = LocalContext.current

    var showExportDialog by remember { mutableStateOf(false) }
    var showBpmDialog by remember { mutableStateOf(false) }
    var activePianoTab by remember { mutableStateOf(false) }
    var exportStems by remember { mutableStateOf(true) }

    val micPresets = listOf("Neumann U87", "Vintage Tube", "Ribbon Velvet", "Stage Dynamic", "Bypass")
    val noteNames = listOf("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")
    val scaleNames = listOf("Chromatic", "Major", "Minor", "Pentatonic", "Blues", "Dorian")
    val ampModels = listOf("Fender Clean", "Marshall Crunch", "High Gain Lead", "Bass Tube Amp")
    val delayModes = listOf("Stereo", "Ping-Pong", "Tape Flutter")
    val timeSignatures = listOf(4 to 4, 3 to 4, 6 to 8)

    val drumPads = listOf(
        "808 KICK"   to Color(0xFFFF5252),
        "SNARE"      to Color(0xFFFF4081),
        "HI-HAT"     to Color(0xFFE040FB),
        "CLAP"       to Color(0xFF7C4DFF),
        "DEEP BASS"  to Color(0xFF536DFE),
        "SYNTH LEAD" to Color(0xFF448AFF),
        "VOCAL CHOP" to Color(0xFF18FFFF),
        "CRASH"      to Color(0xFF64FFDA)
    )

    Surface(modifier = Modifier.fillMaxSize(), color = StudioBackground) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 14.dp, vertical = 12.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp)
        ) {

            // ── Header & Session Controls ───────────────────────────────────
            StudioHeader(
                bpm = uiState.bpm,
                currentBeat = uiState.currentBeat,
                beatsPerBar = uiState.beatsPerBar,
                isRecording = uiState.isRecording,
                metronomeEnabled = uiState.metronomeEnabled,
                onBpmClick = { showBpmDialog = true },
                onTapTempo = { viewModel.tapTempo() },
                onMetronomeClick = { viewModel.toggleMetronome() }
            )

            // ── Session Time Signature Bar ─────────────────────────────────
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    timeSignatures.forEach { (num, den) ->
                        val sel = uiState.beatsPerBar == num && uiState.beatUnit == den
                        FilterChip(
                            selected = sel,
                            onClick = { viewModel.setTimeSignature(num, den) },
                            label = { Text("$num/$den", fontSize = 10.sp, fontWeight = FontWeight.Bold) },
                            colors = FilterChipDefaults.filterChipColors(
                                selectedContainerColor = StudioAmber.copy(alpha = 0.25f),
                                selectedLabelColor = StudioAmber
                            )
                        )
                    }
                }

                // Monitoring Toggle
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Zero-Latency Monitor", color = Color.Gray, fontSize = 10.sp)
                    Spacer(Modifier.width(6.dp))
                    Switch(
                        checked = uiState.monitoringEnabled,
                        onCheckedChange = { viewModel.setMonitoringEnabled(it) },
                        colors = SwitchDefaults.colors(
                            checkedThumbColor = StudioAccent,
                            checkedTrackColor = StudioAccent.copy(alpha = 0.3f)
                        )
                    )
                }
            }

            // ── Mic Presets ────────────────────────────────────────────────
            SectionLabel("STUDIO MICROPHONE PROCESSOR  ·  ZERO TELEPHONE SOUND")
            LazyRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                itemsIndexed(micPresets) { idx, name ->
                    val sel = idx == uiState.selectedPreset
                    PresetChip(name, sel) {
                        viewModel.setMicPreset(idx)
                    }
                }
            }

            // ── Vocal DSP Channel Strip Card ───────────────────────────────
            StudioCard {
                Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.Tune, null, tint = StudioAmber, modifier = Modifier.size(18.dp))
                            Spacer(Modifier.width(8.dp))
                            Text("Vocal DSP  ·  32-Bit NDK C++", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 13.sp)
                        }
                        DspBadge("18dB Butterworth + IR")
                    }

                    DspSlider("Tube Warmth & Harmonics", uiState.tubeWarmth, StudioAmber, "${(uiState.tubeWarmth * 100).toInt()}%") {
                        viewModel.setDspParams(it, uiState.deEsser, uiState.airEqGainDb, uiState.reverbMix)
                    }

                    DspSlider("Dynamic De-Esser (4k-8k Notch)", uiState.deEsser, StudioAccent, "${(uiState.deEsser * 100).toInt()}%") {
                        viewModel.setDspParams(uiState.tubeWarmth, it, uiState.airEqGainDb, uiState.reverbMix)
                    }

                    Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                        Column(modifier = Modifier.weight(1f)) {
                            DspSlider("Air EQ (14 kHz)", uiState.airEqGainDb, StudioPurple, "${"%.1f".format(uiState.airEqGainDb)} dB", 0f..10f) {
                                viewModel.setDspParams(uiState.tubeWarmth, uiState.deEsser, it, uiState.reverbMix)
                            }
                        }
                        Column(modifier = Modifier.weight(1f)) {
                            DspSlider("Stereo Plate Reverb", uiState.reverbMix, StudioGreen, "${(uiState.reverbMix * 100).toInt()}%") {
                                viewModel.setDspParams(uiState.tubeWarmth, uiState.deEsser, uiState.airEqGainDb, it)
                            }
                        }
                    }

                    DspSlider("Formant Shift (Tone Color)", uiState.formantShift, StudioTeal,
                        when {
                            uiState.formantShift < -0.2f -> "Deep Chest Voice"
                            uiState.formantShift > 0.2f  -> "Bright Pop Shifter"
                            else -> "Natural"
                        }, -1.0f..1.0f
                    ) {
                        viewModel.setFormantShift(it)
                    }
                }
            }

            // ── Auto-Tune Card ─────────────────────────────────────────────
            StudioCard {
                Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.Piano, null, tint = StudioTeal, modifier = Modifier.size(18.dp))
                            Spacer(Modifier.width(8.dp))
                            Text("Vocal Pitch Correction  ·  Auto-Tune", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 13.sp)
                        }
                        DspBadge("Granular Overlap-Add")
                    }

                    DspSlider("Correction Strength", uiState.autoTuneAmount, StudioTeal,
                        when {
                            uiState.autoTuneAmount < 0.1f -> "OFF"
                            uiState.autoTuneAmount < 0.4f -> "Natural"
                            uiState.autoTuneAmount < 0.75f -> "Smooth"
                            else -> "Hard Tune ★"
                        }
                    ) {
                        viewModel.setAutoTuneParams(it, uiState.rootKey, uiState.scaleType)
                    }

                    // Root Key selector
                    Column {
                        Text("Root Key", color = Color.Gray, fontSize = 11.sp)
                        Spacer(Modifier.height(4.dp))
                        LazyRow(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                            itemsIndexed(noteNames) { idx, note ->
                                val sel = idx == uiState.rootKey
                                Box(
                                    modifier = Modifier
                                        .size(width = 32.dp, height = 28.dp)
                                        .clip(RoundedCornerShape(6.dp))
                                        .background(if (sel) StudioTeal else StudioCard)
                                        .border(1.dp, if (sel) StudioTeal else StudioCardBorder, RoundedCornerShape(6.dp))
                                        .clickable { viewModel.setAutoTuneParams(uiState.autoTuneAmount, idx, uiState.scaleType) },
                                    contentAlignment = Alignment.Center
                                ) {
                                    Text(note, color = if (sel) Color.Black else Color.LightGray, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                                }
                            }
                        }
                    }

                    // Scale type chips
                    LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        itemsIndexed(scaleNames) { idx, name ->
                            val sel = idx == uiState.scaleType
                            FilterChip(
                                selected = sel,
                                onClick = { viewModel.setAutoTuneParams(uiState.autoTuneAmount, uiState.rootKey, idx) },
                                label = { Text(name, fontSize = 10.sp) },
                                colors = FilterChipDefaults.filterChipColors(
                                    selectedContainerColor = StudioTeal.copy(alpha = 0.3f),
                                    selectedLabelColor = StudioTeal
                                )
                            )
                        }
                    }
                }
            }

            // ── Rack Effects: Guitar Amp & Stereo Delay ────────────────────
            SectionLabel("STUDIO RACK EFFECTS  ·  GUITAR AMP & STEREO DELAY")
            StudioCard {
                Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    // Amp header
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.GraphicEq, null, tint = StudioOrange, modifier = Modifier.size(18.dp))
                            Spacer(Modifier.width(8.dp))
                            Text("Guitar Amp Simulator", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 13.sp)
                        }
                        Switch(
                            checked = uiState.guitarAmpEnabled,
                            onCheckedChange = { viewModel.toggleGuitarAmp() },
                            colors = SwitchDefaults.colors(checkedThumbColor = StudioOrange, checkedTrackColor = StudioOrange.copy(alpha = 0.3f))
                        )
                    }

                    if (uiState.guitarAmpEnabled) {
                        LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            itemsIndexed(ampModels) { idx, modelName ->
                                val sel = idx == uiState.guitarAmpModel
                                FilterChip(
                                    selected = sel,
                                    onClick = { viewModel.setGuitarAmpModel(idx) },
                                    label = { Text(modelName, fontSize = 10.sp) },
                                    colors = FilterChipDefaults.filterChipColors(
                                        selectedContainerColor = StudioOrange.copy(alpha = 0.3f),
                                        selectedLabelColor = StudioOrange
                                    )
                                )
                            }
                        }
                        Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                            Column(modifier = Modifier.weight(1f)) {
                                DspSlider("Drive Gain", uiState.guitarAmpGain, StudioOrange, "${"%.1f".format(uiState.guitarAmpGain)}x", 0.5f..4.0f) {
                                    viewModel.setGuitarAmpParams(it, uiState.guitarAmpTone)
                                }
                            }
                            Column(modifier = Modifier.weight(1f)) {
                                DspSlider("Amp Tone", uiState.guitarAmpTone, StudioAmber, "${(uiState.guitarAmpTone * 100).toInt()}%") {
                                    viewModel.setGuitarAmpParams(uiState.guitarAmpGain, it)
                                }
                            }
                        }
                    }

                    HorizontalDivider(color = StudioCardBorder)

                    // Delay header
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.SurroundSound, null, tint = StudioPurple, modifier = Modifier.size(18.dp))
                            Spacer(Modifier.width(8.dp))
                            Text("BPM-Synced Stereo Delay", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 13.sp)
                        }
                        Switch(
                            checked = uiState.delayEnabled,
                            onCheckedChange = { viewModel.toggleDelay() },
                            colors = SwitchDefaults.colors(checkedThumbColor = StudioPurple, checkedTrackColor = StudioPurple.copy(alpha = 0.3f))
                        )
                    }

                    if (uiState.delayEnabled) {
                        LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            itemsIndexed(delayModes) { idx, modeName ->
                                val sel = idx == uiState.delayMode
                                FilterChip(
                                    selected = sel,
                                    onClick = { viewModel.setDelayMode(idx) },
                                    label = { Text(modeName, fontSize = 10.sp) },
                                    colors = FilterChipDefaults.filterChipColors(
                                        selectedContainerColor = StudioPurple.copy(alpha = 0.3f),
                                        selectedLabelColor = StudioPurple
                                    )
                                )
                            }
                        }
                        Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                            Column(modifier = Modifier.weight(1f)) {
                                DspSlider("Feedback", uiState.delayFeedback, StudioPurple, "${(uiState.delayFeedback * 100).toInt()}%") {
                                    viewModel.setDelayParams(it, uiState.delayWetMix)
                                }
                            }
                            Column(modifier = Modifier.weight(1f)) {
                                DspSlider("Wet Mix", uiState.delayWetMix, StudioAccent, "${(uiState.delayWetMix * 100).toInt()}%") {
                                    viewModel.setDelayParams(uiState.delayFeedback, it)
                                }
                            }
                        }
                    }
                }
            }

            // ── Transport Controls ─────────────────────────────────────────
            StudioCard {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceEvenly,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    // Record button
                    val recColor by animateColorAsState(
                        if (uiState.isRecording) StudioRed else Color(0xFF3A1520),
                        animationSpec = infiniteRepeatable(tween(600), RepeatMode.Reverse),
                        label = "rec"
                    )
                    FloatingActionButton(
                        onClick = { viewModel.toggleRecording() },
                        containerColor = if (uiState.isRecording) recColor else Color(0xFF3A1520),
                        contentColor = Color.White, modifier = Modifier.size(54.dp)
                    ) {
                        Icon(
                            if (uiState.isRecording) Icons.Default.Stop else Icons.Default.FiberManualRecord,
                            null, tint = StudioRed
                        )
                    }

                    // Play / Pause
                    FloatingActionButton(
                        onClick = { viewModel.togglePlayback() },
                        containerColor = StudioAccent, contentColor = Color.Black,
                        modifier = Modifier.size(64.dp)
                    ) {
                        Icon(
                            if (uiState.isPlaying) Icons.Default.Pause else Icons.Default.PlayArrow,
                            null, modifier = Modifier.size(32.dp)
                        )
                    }

                    // Stop
                    IconButton(onClick = { viewModel.stopAll() }) {
                        Icon(Icons.Default.Stop, null, tint = Color.White, modifier = Modifier.size(30.dp))
                    }

                    // Export Master
                    Button(
                        onClick = { showExportDialog = true },
                        colors = ButtonDefaults.buttonColors(containerColor = StudioGreen),
                        shape = RoundedCornerShape(10.dp)
                    ) {
                        Icon(Icons.Default.IosShare, null, tint = Color.Black, modifier = Modifier.size(16.dp))
                        Spacer(Modifier.width(6.dp))
                        Text("EXPORT", color = Color.Black, fontWeight = FontWeight.ExtraBold, fontSize = 11.sp)
                    }
                }
            }

            // ── Real-Time LUFS Master Meter ────────────────────────────────
            LufsMasterMeter(
                integratedLufs = uiState.integratedLufs,
                momentaryLufs = uiState.momentaryLufs
            )

            // ── Multi-Track Console with Waveforms ─────────────────────────
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                SectionLabel("MULTI-TRACK CONSOLE  ·  32-BIT MIX ENGINE")
                TextButton(onClick = { viewModel.addTrack() }) {
                    Icon(Icons.Default.Add, null, tint = StudioAccent, modifier = Modifier.size(14.dp))
                    Spacer(Modifier.width(4.dp))
                    Text("ADD TRACK", color = StudioAccent, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                }
            }

            uiState.tracks.forEach { track ->
                StudioTrackStrip(
                    track = track,
                    onUpdate = { viewModel.updateTrack(it) },
                    onDelete = { viewModel.deleteTrack(track.id) }
                )
            }

            // ── Virtual Studio Instruments ─────────────────────────────────
            SectionLabel("VIRTUAL STUDIO INSTRUMENTS")
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                FilterChip(
                    selected = !activePianoTab,
                    onClick = { activePianoTab = false },
                    label = { Text("Drum Pads (808 / Kit)") }
                )
                FilterChip(
                    selected = activePianoTab,
                    onClick = { activePianoTab = true },
                    label = { Text("Synthesizer & Piano") }
                )
            }

            if (!activePianoTab) {
                // Drum Grid
                LazyVerticalGrid(
                    columns = GridCells.Fixed(4),
                    modifier = Modifier.fillMaxWidth().height(190.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    itemsIndexed(drumPads) { idx, (padName, padColor) ->
                        var pressed by remember { mutableStateOf(false) }
                        val padBg by animateColorAsState(
                            if (pressed) padColor.copy(alpha = 0.7f) else padColor.copy(alpha = 0.20f),
                            label = "pad$idx"
                        )
                        Card(
                            colors = CardDefaults.cardColors(containerColor = padBg),
                            shape = RoundedCornerShape(12.dp),
                            border = BorderStroke(1.dp, padColor.copy(alpha = if (pressed) 1f else 0.4f)),
                            modifier = Modifier
                                .fillMaxSize()
                                .clickable(
                                    interactionSource = remember { MutableInteractionSource() },
                                    indication = null
                                ) {
                                    pressed = true
                                    viewModel.triggerDrumPad(idx, 0.95f)
                                }
                        ) {
                            Box(Modifier.fillMaxSize().padding(8.dp), contentAlignment = Alignment.Center) {
                                Text(padName, color = Color.White, fontWeight = FontWeight.Bold, fontSize = 10.sp, textAlign = TextAlign.Center)
                            }
                        }
                    }
                }
            } else {
                // Synthesizer & Piano Keys
                val waveforms = listOf("Sawtooth", "Square", "Triangle", "Sine", "FM Synth")
                StudioCard {
                    Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text("Subtractive & FM Synthesizer", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 13.sp)
                            DspBadge("Polyphonic ADSR")
                        }

                        LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            itemsIndexed(waveforms) { idx, waveName ->
                                val sel = idx == uiState.synthWaveform
                                FilterChip(
                                    selected = sel,
                                    onClick = { viewModel.setSynthWaveform(idx) },
                                    label = { Text(waveName, fontSize = 10.sp) },
                                    colors = FilterChipDefaults.filterChipColors(
                                        selectedContainerColor = StudioAccent.copy(alpha = 0.3f),
                                        selectedLabelColor = StudioAccent
                                    )
                                )
                            }
                        }

                        DspSlider("Filter Cutoff", uiState.synthCutoff, StudioAccent, "${uiState.synthCutoff.toInt()} Hz", 200.0f..12000.0f) {
                            viewModel.setSynthCutoff(it)
                        }

                        MiniPianoKeyboard(
                            baseOctave = 3,
                            onNoteOn = { note -> viewModel.noteOn(note) },
                            onNoteOff = { note -> viewModel.noteOff(note) }
                        )
                    }
                }
            }

            Spacer(Modifier.height(24.dp))
        }

        // Export Dialog
        if (showExportDialog) {
            AlertDialog(
                onDismissRequest = { showExportDialog = false },
                containerColor = StudioCard,
                titleContentColor = Color.White,
                textContentColor = Color.LightGray,
                title = {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(Icons.Default.Album, null, tint = StudioGreen, modifier = Modifier.size(22.dp))
                        Spacer(Modifier.width(8.dp))
                        Text("Export Studio Master", fontWeight = FontWeight.Bold)
                    }
                },
                text = {
                    Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                        ExportRow("Format", "24-bit / 48 kHz WAV (Uncompressed)")
                        ExportRow("Standard", "Spotify / Apple Music (–14 LUFS)")
                        ExportRow("Limiter", "True Peak Brickwall (–1.0 dBTP)")
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text("Export Stems (Separate WAVs)", color = Color.White, fontSize = 12.sp)
                            Checkbox(
                                checked = exportStems,
                                onCheckedChange = { exportStems = it },
                                colors = CheckboxDefaults.colors(checkedColor = StudioGreen)
                            )
                        }

                        HorizontalDivider(color = StudioCardBorder)

                        if (uiState.exportResultMessage != null) {
                            Text(
                                uiState.exportResultMessage!!,
                                color = StudioAccent,
                                fontSize = 12.sp,
                                fontWeight = FontWeight.Bold
                            )
                        } else if (uiState.exportInProgress) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                CircularProgressIndicator(color = StudioGreen, modifier = Modifier.size(18.dp))
                                Spacer(Modifier.width(10.dp))
                                Text("Rendering 24-bit audio...", color = Color.LightGray, fontSize = 12.sp)
                            }
                        }
                    }
                },
                confirmButton = {
                    Button(
                        onClick = {
                            viewModel.exportRelease(context, exportStems)
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = StudioGreen),
                        enabled = !uiState.exportInProgress
                    ) {
                        Text("RENDER & EXPORT", color = Color.Black, fontWeight = FontWeight.ExtraBold)
                    }
                },
                dismissButton = {
                    TextButton(onClick = {
                        viewModel.clearExportMessage()
                        showExportDialog = false
                    }) {
                        Text("Close", color = Color.Gray)
                    }
                }
            )
        }

        // BPM Dialog
        if (showBpmDialog) {
            var tempBpm by remember { mutableStateOf(uiState.bpm.toFloat()) }
            AlertDialog(
                onDismissRequest = { showBpmDialog = false },
                containerColor = StudioCard,
                titleContentColor = Color.White,
                textContentColor = Color.LightGray,
                title = { Text("Session BPM", fontWeight = FontWeight.Bold) },
                text = {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        Text("${tempBpm.toInt()} BPM", color = StudioAccent, fontWeight = FontWeight.Bold, fontSize = 28.sp)
                        Slider(
                            value = tempBpm,
                            onValueChange = { tempBpm = it },
                            valueRange = 40f..240f,
                            colors = SliderDefaults.colors(thumbColor = StudioAccent, activeTrackColor = StudioAccent)
                        )
                    }
                },
                confirmButton = {
                    Button(
                        onClick = {
                            viewModel.setBpm(tempBpm.toInt())
                            showBpmDialog = false
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = StudioAccent)
                    ) {
                        Text("APPLY", color = Color.Black, fontWeight = FontWeight.Bold)
                    }
                }
            )
        }
    }
}

// ─── Sub-Composables ──────────────────────────────────────────────────────────

@Composable
private fun StudioHeader(
    bpm: Int,
    currentBeat: Int,
    beatsPerBar: Int,
    isRecording: Boolean,
    metronomeEnabled: Boolean,
    onBpmClick: () -> Unit,
    onTapTempo: () -> Unit,
    onMetronomeClick: () -> Unit
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column {
            Text("StudioProd DAW", color = Color.White, fontWeight = FontWeight.ExtraBold, fontSize = 22.sp)
            Text("32-Bit NDK Engine  ·  AAudio Low-Latency", color = StudioAccent, fontSize = 11.sp)
        }

        Row(horizontalArrangement = Arrangement.spacedBy(6.dp), verticalAlignment = Alignment.CenterVertically) {
            // Metronome Click Chip with visual flash on beat 1 (accent)
            val flashColor = if (currentBeat == 0 && metronomeEnabled) StudioAmber else Color.Gray
            Surface(
                shape = RoundedCornerShape(20.dp),
                color = if (metronomeEnabled) StudioAmber.copy(alpha = 0.2f) else StudioCard,
                border = BorderStroke(1.dp, if (metronomeEnabled) flashColor else StudioCardBorder),
                modifier = Modifier.clickable(onClick = onMetronomeClick)
            ) {
                Row(Modifier.padding(horizontal = 10.dp, vertical = 7.dp), verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.AccessTime, null, tint = flashColor, modifier = Modifier.size(14.dp))
                    Spacer(Modifier.width(4.dp))
                    Text(if (metronomeEnabled) "CLICK ${currentBeat + 1}/$beatsPerBar" else "CLICK OFF", color = flashColor, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                }
            }

            // Tap Tempo Button
            Surface(
                shape = RoundedCornerShape(20.dp),
                color = StudioCard,
                border = BorderStroke(1.dp, StudioCardBorder),
                modifier = Modifier.clickable(onClick = onTapTempo)
            ) {
                Text("TAP", color = StudioAccent, fontSize = 10.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(horizontal = 8.dp, vertical = 7.dp))
            }

            // BPM Chip
            Surface(
                shape = RoundedCornerShape(20.dp),
                color = StudioCard,
                border = BorderStroke(1.dp, StudioAccent),
                modifier = Modifier.clickable(onClick = onBpmClick)
            ) {
                Row(Modifier.padding(horizontal = 10.dp, vertical = 7.dp), verticalAlignment = Alignment.CenterVertically) {
                    Text("$bpm BPM", color = Color.White, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                }
            }
        }
    }
}

@Composable
private fun LufsMasterMeter(
    integratedLufs: Float,
    momentaryLufs: Float
) {
    StudioCard {
        Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("ITU-R BS.1770-4 MASTER LOUDNESS", color = Color.LightGray, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                    Text("Momentary: ${"%.1f".format(momentaryLufs)} LUFS", color = StudioAccent, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                    Text("Integrated: ${"%.1f".format(integratedLufs)} LUFS", color = StudioGreen, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                }
            }

            // Visual Meter Bar
            val normalized = ((momentaryLufs + 60f) / 60f).coerceIn(0f, 1f)
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(8.dp)
                    .clip(RoundedCornerShape(4.dp))
                    .background(Color(0xFF0F1118))
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .fillMaxWidth(normalized)
                        .clip(RoundedCornerShape(4.dp))
                        .background(
                            Brush.horizontalGradient(
                                listOf(StudioGreen, StudioAmber, StudioRed)
                            )
                        )
                )
            }
        }
    }
}

@Composable
private fun StudioTrackStrip(
    track: TrackModel,
    onUpdate: (TrackModel) -> Unit,
    onDelete: () -> Unit
) {
    StudioCard {
        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(modifier = Modifier.size(10.dp).clip(CircleShape).background(track.color))
                    Spacer(Modifier.width(8.dp))
                    Text(track.name, color = Color.White, fontWeight = FontWeight.Bold, fontSize = 13.sp)
                }

                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    // Arm button
                    ButtonPill("ARM", track.isArmed, StudioRed) {
                        onUpdate(track.copy(isArmed = !track.isArmed))
                    }
                    // Mute button
                    ButtonPill("M", track.isMuted, StudioAmber) {
                        onUpdate(track.copy(isMuted = !track.isMuted))
                    }
                    // Solo button
                    ButtonPill("S", track.isSolo, StudioGreen) {
                        onUpdate(track.copy(isSolo = !track.isSolo))
                    }
                    // Delete
                    IconButton(onClick = onDelete, modifier = Modifier.size(24.dp)) {
                        Icon(Icons.Default.Close, null, tint = Color.Gray, modifier = Modifier.size(14.dp))
                    }
                }
            }

            // Waveform Canvas
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(34.dp)
                    .clip(RoundedCornerShape(6.dp))
                    .background(Color(0xFF0A0C12))
            ) {
                Canvas(modifier = Modifier.fillMaxSize()) {
                    val pts = track.waveformPoints
                    if (pts.isNotEmpty()) {
                        val midY = size.height / 2f
                        val stepX = size.width / pts.size.toFloat()
                        val path = Path()

                        pts.forEachIndexed { i, peak ->
                            val x = i * stepX
                            val barH = peak * midY
                            drawLine(
                                color = track.color.copy(alpha = 0.85f),
                                start = Offset(x, midY - barH),
                                end = Offset(x, midY + barH),
                                strokeWidth = 2.5f
                            )
                        }
                    } else {
                        // Empty line
                        drawLine(
                            color = Color(0xFF202330),
                            start = Offset(0f, size.height / 2f),
                            end = Offset(size.width, size.height / 2f),
                            strokeWidth = 1f
                        )
                    }
                }
            }

            // Fader & Pan Row
            Row(horizontalArrangement = Arrangement.spacedBy(14.dp), verticalAlignment = Alignment.CenterVertically) {
                Column(modifier = Modifier.weight(1f)) {
                    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                        Text("Volume", color = Color.Gray, fontSize = 10.sp)
                        Text("${(track.volume * 100).toInt()}%", color = Color.White, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                    }
                    Slider(
                        value = track.volume,
                        onValueChange = { onUpdate(track.copy(volume = it)) },
                        valueRange = 0.0f..1.5f,
                        colors = SliderDefaults.colors(thumbColor = track.color, activeTrackColor = track.color)
                    )
                }

                Column(modifier = Modifier.weight(1f)) {
                    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                        Text("Pan", color = Color.Gray, fontSize = 10.sp)
                        Text(
                            when {
                                track.pan < -0.05f -> "L ${(track.pan * -100).toInt()}%"
                                track.pan > 0.05f  -> "R ${(track.pan * 100).toInt()}%"
                                else -> "C"
                            },
                            color = Color.White, fontSize = 10.sp, fontWeight = FontWeight.Bold
                        )
                    }
                    Slider(
                        value = track.pan,
                        onValueChange = { onUpdate(track.copy(pan = it)) },
                        valueRange = -1.0f..1.0f,
                        colors = SliderDefaults.colors(thumbColor = StudioAccent, activeTrackColor = StudioAccent)
                    )
                }
            }
        }
    }
}

@Composable
private fun MiniPianoKeyboard(
    baseOctave: Int = 3,
    onNoteOn: (Int) -> Unit,
    onNoteOff: (Int) -> Unit
) {
    val whiteNotes = listOf(0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23)
    val startMidi = baseOctave * 12

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(110.dp)
            .clip(RoundedCornerShape(8.dp))
            .background(Color.Black)
            .padding(2.dp)
    ) {
        whiteNotes.forEach { offset ->
            val midiNote = startMidi + offset
            var isDown by remember { mutableStateOf(false) }

            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
                    .padding(1.dp)
                    .clip(RoundedCornerShape(bottomStart = 4.dp, bottomEnd = 4.dp))
                    .background(if (isDown) StudioAccent else Color(0xFFE8E8E8))
                    .clickable(
                        interactionSource = remember { MutableInteractionSource() },
                        indication = null
                    ) {
                        isDown = !isDown
                        if (isDown) onNoteOn(midiNote) else onNoteOff(midiNote)
                    },
                contentAlignment = Alignment.BottomCenter
            ) {
                Text(
                    text = "${midiNote}",
                    color = Color.DarkGray,
                    fontSize = 8.sp,
                    modifier = Modifier.padding(bottom = 4.dp)
                )
            }
        }
    }
}

// ─── UI Helper Composables ───────────────────────────────────────────────────

@Composable
private fun StudioCard(content: @Composable () -> Unit) {
    Card(
        colors = CardDefaults.cardColors(containerColor = StudioCard),
        border = BorderStroke(1.dp, StudioCardBorder),
        shape = RoundedCornerShape(14.dp),
        modifier = Modifier.fillMaxWidth()
    ) {
        Box(modifier = Modifier.padding(14.dp)) {
            content()
        }
    }
}

@Composable
private fun SectionLabel(title: String) {
    Text(
        text = title,
        color = Color(0xFF6B728E),
        fontSize = 10.sp,
        fontWeight = FontWeight.ExtraBold,
        letterSpacing = 1.sp
    )
}

@Composable
private fun PresetChip(name: String, selected: Boolean, onClick: () -> Unit) {
    Surface(
        shape = RoundedCornerShape(20.dp),
        color = if (selected) StudioAccent else StudioCard,
        border = BorderStroke(1.dp, if (selected) StudioAccent else StudioCardBorder),
        modifier = Modifier.clickable(onClick = onClick)
    ) {
        Text(
            text = name,
            color = if (selected) Color.Black else Color.White,
            fontWeight = FontWeight.Bold,
            fontSize = 11.sp,
            modifier = Modifier.padding(horizontal = 14.dp, vertical = 7.dp)
        )
    }
}

@Composable
private fun DspBadge(text: String) {
    Surface(
        shape = RoundedCornerShape(6.dp),
        color = Color(0xFF1E2333),
        border = BorderStroke(1.dp, Color(0xFF2C3248))
    ) {
        Text(text, color = StudioAccent, fontSize = 9.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(horizontal = 6.dp, vertical = 3.dp))
    }
}

@Composable
private fun DspSlider(
    label: String,
    value: Float,
    color: Color,
    display: String,
    range: ClosedFloatingPointRange<Float> = 0f..1f,
    onValueChange: (Float) -> Unit
) {
    Column {
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Text(label, color = Color.LightGray, fontSize = 11.sp)
            Text(display, color = color, fontSize = 11.sp, fontWeight = FontWeight.Bold)
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = range,
            colors = SliderDefaults.colors(thumbColor = color, activeTrackColor = color)
        )
    }
}

@Composable
private fun ButtonPill(text: String, active: Boolean, activeColor: Color, onClick: () -> Unit) {
    Surface(
        shape = RoundedCornerShape(4.dp),
        color = if (active) activeColor else Color(0xFF10131C),
        border = BorderStroke(1.dp, if (active) activeColor else StudioCardBorder),
        modifier = Modifier.clickable(onClick = onClick)
    ) {
        Text(
            text,
            color = if (active) Color.Black else Color.Gray,
            fontSize = 10.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(horizontal = 6.dp, vertical = 3.dp)
        )
    }
}

@Composable
private fun ExportRow(label: String, value: String) {
    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, color = Color.Gray, fontSize = 11.sp)
        Text(value, color = Color.White, fontSize = 11.sp, fontWeight = FontWeight.SemiBold)
    }
}
