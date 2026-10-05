# StudioProd: Professional Studio & Music Production Suite for Android

[![Platform](https://img.shields.io/badge/Platform-Android-green.svg)](https://android.com)
[![Architecture](https://img.shields.io/badge/Architecture-C%2B%2B%20NDK%20%2B%20Jetpack%20Compose-blue.svg)](https://developer.android.com)
[![Engine](https://img.shields.io/badge/Engine-32--Bit%20Float%20AAudio-orange.svg)](https://developer.android.com/ndk/guides/audio/aaudio)
[![Download APK](https://img.shields.io/badge/Download-StudioProd%20APK-brightgreen.svg?logo=android)](https://github.com/Chilakala-Surya-Prakash/studioprod/releases/download/v1.0.0/StudioProd-v1.0.0.apk)
[![License](https://img.shields.io/badge/License-Apache%202.0-lightgrey.svg)](LICENSE)

> 🚀 **[Download StudioProd v1.0.0 APK](https://github.com/Chilakala-Surya-Prakash/studioprod/releases/download/v1.0.0/StudioProd-v1.0.0.apk)** (Direct Download)

**StudioProd** is a production-grade digital audio workstation (DAW) and studio vocal transformation suite engineered for Android. It eliminates the "telephone sound" typical of mobile MEMS microphones by utilizing a real-time 32-bit floating-point C++ DSP pipeline and AAudio low-latency drivers, paired with a modern Jetpack Compose dark studio interface.

---

## Key Features

### 1. Studio Vocal DSP Transformation Engine
* **18 dB/oct Butterworth High-Pass Filter**: Cascaded 1-pole and 2-pole Butterworth biquad removing handling noise and sub-bass rumble (<85 Hz).
* **Acoustic IR Calibration Filter Bank**: Calibrated multi-stage filter models emulating legendary studio microphones:
  * **Neumann U87**: Silky high-end air shimmer and gentle chest proximity warmth.
  * **Vintage Tube**: Telefunken/Sony C800G style tube saturation and smoothed midrange.
  * **Ribbon Velvet**: Warm body and rolled-off silky high frequencies.
  * **Stage Dynamic**: Shure SM7B presence boost and dual low-cut profile.
* **True Granular Overlap-Add Pitch Correction & Auto-Tune**: Synchronized dual-delay-line crossfading pitch shifter with YIN fundamental frequency detection. Quantizes vocal audio across Major, Minor, Pentatonic, Blues, Dorian, and Chromatic scales.
* **Formant Shifter**: Real-time vocal timbre modification (deep chest voice to bright modern pop).
* **Dynamic Vocal De-Esser**: Sidechain bandpass (6.5 kHz) with dynamic notch filter attenuation for sibilance damping.
* **Analogue Tube Preamp Warmth**: Asymmetric soft-clipping with 2nd and 3rd harmonic saturation.
* **Linkwitz-Riley Multi-Band Compressor**: 3-band crossover (Low < 250 Hz, Mid 250–4000 Hz, High > 4000 Hz) with independent RMS level detectors.
* **Stereo Plate Reverb Tank**: Schroeder-Moorer diffusion network with 4 parallel comb filters and 2 series all-pass diffusers per channel.

### 2. Multi-Track DAW Arranging & Console
* **Full-Duplex AAudio Backend**: Low-latency recording and playback with zero-latency headphone monitoring.
* **Lock-Free Thread Safety**: SPSC circular ring buffers (`LockFreeQueue.h`) with zero memory allocations or mutexes on the real-time audio thread.
* **Real-Time Waveform Visualizer**: Real-time peak extraction rendered via Compose `Canvas`.
* **Track Editing**: Non-destructive clip trimming and splitting.
* **Mix Console**: Per-track volume faders, equal-power pan knobs ($\cos / \sin$ law), mute, solo, and record arming.
* **Rack FX**: Guitar Amp Simulator with cabinet emulation, 7-Band Parametric EQ, and BPM-synced Stereo Delay.

### 3. Virtual Studio Instruments & External MIDI
* **8-Pad Drum Machine**: Synthesized 808 Kick, Snare, Hi-hat, Clap, Deep Bass, Synth Lead, Vocal Chop, and Crash.
* **Polyphonic Synthesizer**: Multi-waveform oscillator (Sawtooth, Square, Triangle, Sine, FM), ADSR envelopes, and resonant low-pass filter.
* **2-Octave Piano Keyboard**: Touch-responsive keys with MIDI Note-On/Off triggering.
* **External MIDI Controller Support**: Android MIDI API integration for plug-and-play USB and Bluetooth LE MIDI controllers.

### 4. Mastering & Release Studio
* **ITU-R BS.1770-4 LUFS Analyzer**: True Momentary and Integrated loudness metering.
* **Lookahead True-Peak Limiter**: Brickwall ceiling at –1.0 dBTP preventing inter-sample distortion.
* **Export Engine**: 24-bit / 48 kHz uncompressed master WAV rendering and individual stem export.

### 5. Android Platform & Architecture
* **Foreground Service**: `StudioAudioService` with `FOREGROUND_SERVICE_MICROPHONE` and `FOREGROUND_SERVICE_MEDIA_PLAYBACK` to keep the audio engine running continuously.
* **Audio Focus**: Lifecycle management with automatic pause/resume on incoming calls.
* **Clean Architecture**: `StudioViewModel` with Unidirectional Data Flow (`StateFlow`) and decoupled `StudioEngineBridge`.
* **ProGuard / R8 Hardening**: Native JNI preservation and compiler optimizations (`-O3 -ffast-math`).

---

## Tech Stack
* **UI**: Android Jetpack Compose, Material 3
* **Native Audio**: C++17, Android NDK, Google AAudio, OpenSL ES
* **Architecture**: MVVM, Kotlin Coroutines, StateFlow, Foreground Service
* **Build System**: Gradle Kotlin DSL (`build.gradle.kts`), CMake 3.22+


