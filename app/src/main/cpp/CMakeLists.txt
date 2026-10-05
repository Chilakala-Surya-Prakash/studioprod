cmake_minimum_required(VERSION 3.22.1)

project("studioprod_native")

add_library(
    studioprod_native
    SHARED
    StudioMicDSP.cpp
    DrumSynthEngine.cpp
    VirtualSynthEngine.cpp
    ParametricEQ.cpp
    GuitarAmpSim.cpp
    DelayEffect.cpp
    LufsAnalyzer.cpp
    Metronome.cpp
    StudioEngine.cpp
    NativeBridge.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    studioprod_native
    ${log-lib}
    aaudio
    OpenSLES
)
