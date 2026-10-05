package com.example.studioprod.midi

import android.content.Context
import android.media.midi.MidiDevice
import android.media.midi.MidiDeviceInfo
import android.media.midi.MidiManager
import android.media.midi.MidiReceiver
import android.os.Handler
import android.os.Looper
import android.util.Log
import com.example.studioprod.NativeBridge

class StudioMidiManager(private val context: Context) {

    private val midiManager: MidiManager? =
        context.getSystemService(Context.MIDI_SERVICE) as? MidiManager

    private val openDevices = mutableListOf<MidiDevice>()

    fun initMidiSupport() {
        if (midiManager == null) {
            Log.w("StudioMidiManager", "MIDI service not available on this device.")
            return
        }

        // Listen for new MIDI devices (USB / BLE)
        midiManager.registerDeviceCallback(object : MidiManager.DeviceCallback() {
            override fun onDeviceAdded(device: MidiDeviceInfo) {
                openDevice(device)
            }

            override fun onDeviceRemoved(device: MidiDeviceInfo) {
                Log.i("StudioMidiManager", "MIDI device disconnected: ${device.properties.getString(MidiDeviceInfo.PROPERTY_NAME)}")
            }
        }, Handler(Looper.getMainLooper()))

        // Open any already-connected devices
        midiManager.devices.forEach { openDevice(it) }
    }

    private fun openDevice(deviceInfo: MidiDeviceInfo) {
        midiManager?.openDevice(deviceInfo, { device ->
            if (device != null) {
                openDevices.add(device)
                val portCount = deviceInfo.outputPortCount
                for (p in 0 until portCount) {
                    val outputPort = device.openOutputPort(p)
                    outputPort?.connect(object : MidiReceiver() {
                        override fun onSend(msg: ByteArray, offset: Int, count: Int, timestamp: Long) {
                            parseMidiMessage(msg, offset, count)
                        }
                    })
                }
                Log.i("StudioMidiManager", "Connected MIDI device: ${deviceInfo.properties.getString(MidiDeviceInfo.PROPERTY_NAME)}")
            }
        }, Handler(Looper.getMainLooper()))
    }

    private fun parseMidiMessage(msg: ByteArray, offset: Int, count: Int) {
        var i = offset
        while (i < offset + count) {
            val status = msg[i].toInt() and 0xFF
            val command = status and 0xF0

            if (command == 0x90 && i + 2 < offset + count) {
                // Note On
                val note = msg[i + 1].toInt() and 0x7F
                val velByte = msg[i + 2].toInt() and 0x7F
                val velocity = velByte / 127.0f
                if (velocity > 0.0f) {
                    NativeBridge.noteOn(note, velocity)
                } else {
                    NativeBridge.noteOff(note)
                }
                i += 3
            } else if (command == 0x80 && i + 2 < offset + count) {
                // Note Off
                val note = msg[i + 1].toInt() and 0x7F
                NativeBridge.noteOff(note)
                i += 3
            } else {
                i++
            }
        }
    }

    fun release() {
        openDevices.forEach {
            try { it.close() } catch (_: Exception) {}
        }
        openDevices.clear()
    }
}
