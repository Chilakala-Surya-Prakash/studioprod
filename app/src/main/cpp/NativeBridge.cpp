#include <jni.h>
#include <memory>
#include <string>
#include <vector>
#include "StudioEngine.h"

static std::unique_ptr<StudioEngine> gEngine;

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_initNativeEngine(JNIEnv *env, jobject thiz) {
    if (!gEngine) {
        gEngine = std::make_unique<StudioEngine>();
        gEngine->startAudioEngine();
    }
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_stopNativeEngine(JNIEnv *env, jobject thiz) {
    if (gEngine) {
        gEngine->stopAudioEngine();
        gEngine.reset();
    }
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setBpm(JNIEnv *env, jobject thiz, jint bpm) {
    if (gEngine) gEngine->setBpm(bpm);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_tapTempo(JNIEnv *env, jobject thiz) {
    if (gEngine) gEngine->tapTempo();
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setTimeSignature(JNIEnv *env, jobject thiz, jint beatsPerBar, jint beatUnit) {
    if (gEngine) gEngine->setTimeSignature(beatsPerBar, beatUnit);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_startRecording(JNIEnv *env, jobject thiz) {
    if (gEngine) gEngine->startRecording();
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_stopRecording(JNIEnv *env, jobject thiz) {
    if (gEngine) gEngine->stopRecording();
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_startPlayback(JNIEnv *env, jobject thiz) {
    if (gEngine) gEngine->startPlayback();
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_stopPlayback(JNIEnv *env, jobject thiz) {
    if (gEngine) gEngine->stopPlayback();
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_seekToFrame(JNIEnv *env, jobject thiz, jlong frame) {
    if (gEngine) gEngine->seekToFrame(static_cast<size_t>(frame));
}

JNIEXPORT jlong JNICALL
Java_com_example_studioprod_NativeBridge_getPlaybackPosition(JNIEnv *env, jobject thiz) {
    return gEngine ? static_cast<jlong>(gEngine->getPlaybackPosition()) : 0;
}

JNIEXPORT jlong JNICALL
Java_com_example_studioprod_NativeBridge_getTotalFrames(JNIEnv *env, jobject thiz) {
    return gEngine ? static_cast<jlong>(gEngine->getTotalFrames()) : 0;
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setMonitoringEnabled(JNIEnv *env, jobject thiz, jboolean enabled) {
    if (gEngine) gEngine->setMonitoringEnabled(enabled);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setMonitoringLevel(JNIEnv *env, jobject thiz, jfloat level) {
    if (gEngine) gEngine->setMonitoringLevel(level);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setMicPreset(JNIEnv *env, jobject thiz, jint presetIndex) {
    if (gEngine) {
        gEngine->getMicDSP().setPreset(static_cast<MicPreset>(presetIndex));
    }
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setDSPParams(JNIEnv *env, jobject thiz,
                                                        jfloat tubeWarmth,
                                                        jfloat deEsserAmount,
                                                        jfloat airEqGainDb,
                                                        jfloat reverbMix) {
    if (gEngine) {
        DSPParams params = gEngine->getMicDSP().getParams();
        params.tubeWarmth = tubeWarmth;
        params.deEsserAmount = deEsserAmount;
        params.airEqGainDb = airEqGainDb;
        params.reverbMix = reverbMix;
        gEngine->getMicDSP().setParams(params);
    }
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setFormantShift(JNIEnv *env, jobject thiz, jfloat shift) {
    if (gEngine) {
        DSPParams params = gEngine->getMicDSP().getParams();
        params.formantShift = shift;
        gEngine->getMicDSP().setParams(params);
    }
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setAutoTuneParams(JNIEnv *env, jobject thiz, jfloat amount, jint rootKey, jint scaleType) {
    if (gEngine) {
        DSPParams params = gEngine->getMicDSP().getParams();
        params.autoTuneAmount = amount;
        params.rootKeyNote = rootKey;
        params.scaleType = static_cast<ScaleType>(scaleType);
        gEngine->getMicDSP().setParams(params);
    }
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_triggerDrumPad(JNIEnv *env, jobject thiz, jint padIndex, jfloat velocity) {
    if (gEngine) gEngine->triggerDrumPad(padIndex, velocity);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_noteOn(JNIEnv *env, jobject thiz, jint midiNote, jfloat velocity) {
    if (gEngine) gEngine->noteOn(midiNote, velocity);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_noteOff(JNIEnv *env, jobject thiz, jint midiNote) {
    if (gEngine) gEngine->noteOff(midiNote);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setSynthWaveform(JNIEnv *env, jobject thiz, jint waveformIndex) {
    if (gEngine) gEngine->setSynthWaveform(waveformIndex);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setSynthCutoff(JNIEnv *env, jobject thiz, jfloat cutoffHz) {
    if (gEngine) gEngine->setSynthCutoff(cutoffHz);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setMetronomeEnabled(JNIEnv *env, jobject thiz, jboolean enabled) {
    if (gEngine) gEngine->setMetronomeEnabled(enabled);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setMetronomeVolume(JNIEnv *env, jobject thiz, jfloat vol) {
    if (gEngine) gEngine->setMetronomeVolume(vol);
}

JNIEXPORT jint JNICALL
Java_com_example_studioprod_NativeBridge_getMetronomeBeat(JNIEnv *env, jobject thiz) {
    return gEngine ? gEngine->getMetronomeBeat() : 0;
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setGuitarAmpEnabled(JNIEnv *env, jobject thiz, jboolean enabled) {
    if (gEngine) gEngine->setGuitarAmpEnabled(enabled);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setGuitarAmpModel(JNIEnv *env, jobject thiz, jint model) {
    if (gEngine) gEngine->setGuitarAmpModel(model);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setGuitarAmpGain(JNIEnv *env, jobject thiz, jfloat gain) {
    if (gEngine) gEngine->setGuitarAmpGain(gain);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setGuitarAmpTone(JNIEnv *env, jobject thiz, jfloat tone) {
    if (gEngine) gEngine->setGuitarAmpTone(tone);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setDelayEnabled(JNIEnv *env, jobject thiz, jboolean enabled) {
    if (gEngine) gEngine->setDelayEnabled(enabled);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setDelayMode(JNIEnv *env, jobject thiz, jint mode) {
    if (gEngine) gEngine->setDelayMode(mode);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setDelayFeedback(JNIEnv *env, jobject thiz, jfloat feedback) {
    if (gEngine) gEngine->setDelayFeedback(feedback);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setDelayWetMix(JNIEnv *env, jobject thiz, jfloat wetMix) {
    if (gEngine) gEngine->setDelayWetMix(wetMix);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setEQBand(JNIEnv *env, jobject thiz, jint bandIndex, jint type, jfloat freq, jfloat gainDb, jfloat q, jboolean enabled) {
    if (gEngine) gEngine->setEQBand(bandIndex, type, freq, gainDb, q, enabled);
}

JNIEXPORT jint JNICALL
Java_com_example_studioprod_NativeBridge_createTrack(JNIEnv *env, jobject thiz, jstring name_) {
    if (!gEngine) return -1;
    const char *name = env->GetStringUTFChars(name_, nullptr);
    int trackId = gEngine->createTrack(std::string(name));
    env->ReleaseStringUTFChars(name_, name);
    return trackId;
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_deleteTrack(JNIEnv *env, jobject thiz, jint trackId) {
    if (gEngine) gEngine->deleteTrack(trackId);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_clearTrackBuffer(JNIEnv *env, jobject thiz, jint trackId) {
    if (gEngine) gEngine->clearTrackBuffer(trackId);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setTrackVolume(JNIEnv *env, jobject thiz, jint trackId, jfloat volume) {
    if (gEngine) gEngine->setTrackVolume(trackId, volume);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setTrackPan(JNIEnv *env, jobject thiz, jint trackId, jfloat pan) {
    if (gEngine) gEngine->setTrackPan(trackId, pan);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setTrackMute(JNIEnv *env, jobject thiz, jint trackId, jboolean mute) {
    if (gEngine) gEngine->setTrackMute(trackId, mute);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_setTrackSolo(JNIEnv *env, jobject thiz, jint trackId, jboolean solo) {
    if (gEngine) gEngine->setTrackSolo(trackId, solo);
}

JNIEXPORT void JNICALL
Java_com_example_studioprod_NativeBridge_armTrackForRecord(JNIEnv *env, jobject thiz, jint trackId, jboolean arm) {
    if (gEngine) gEngine->armTrackForRecord(trackId, arm);
}

JNIEXPORT jfloatArray JNICALL
Java_com_example_studioprod_NativeBridge_getTrackWaveform(JNIEnv *env, jobject thiz, jint trackId, jint numPoints) {
    if (!gEngine || numPoints <= 0) return env->NewFloatArray(0);
    std::vector<float> wf = gEngine->getTrackWaveform(trackId, numPoints);
    jfloatArray result = env->NewFloatArray(static_cast<jsize>(wf.size()));
    if (result && !wf.empty()) {
        env->SetFloatArrayRegion(result, 0, static_cast<jsize>(wf.size()), wf.data());
    }
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_example_studioprod_NativeBridge_trimTrack(JNIEnv *env, jobject thiz, jint trackId, jlong startFrame, jlong endFrame) {
    if (!gEngine) return JNI_FALSE;
    return gEngine->trimTrack(trackId, static_cast<size_t>(startFrame), static_cast<size_t>(endFrame)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_example_studioprod_NativeBridge_splitTrack(JNIEnv *env, jobject thiz, jint trackId, jlong splitFrame) {
    if (!gEngine) return JNI_FALSE;
    return gEngine->splitTrack(trackId, static_cast<size_t>(splitFrame)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_example_studioprod_NativeBridge_exportMasterWav(JNIEnv *env, jobject thiz, jstring filePath_, jboolean exportStems) {
    if (!gEngine) return JNI_FALSE;
    const char *filePath = env->GetStringUTFChars(filePath_, nullptr);
    bool success = gEngine->exportMasterWav(std::string(filePath), exportStems);
    env->ReleaseStringUTFChars(filePath_, filePath);
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jfloat JNICALL
Java_com_example_studioprod_NativeBridge_getMasterLufs(JNIEnv *env, jobject thiz) {
    return gEngine ? gEngine->getMasterLufs() : -60.0f;
}

JNIEXPORT jfloat JNICALL
Java_com_example_studioprod_NativeBridge_getMomentaryLufs(JNIEnv *env, jobject thiz) {
    return gEngine ? gEngine->getMomentaryLufs() : -60.0f;
}

}
