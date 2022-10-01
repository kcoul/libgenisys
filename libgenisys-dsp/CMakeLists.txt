cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libGenisysDSP)

add_library(libGenisysDSP STATIC
        juce/juce_AbstractFifo.cpp
        juce/juce_AudioDataConverters.cpp
        juce/juce_DelayLine.cpp
        juce/juce_DryWetMixer.cpp
        juce/juce_FloatVectorOperations.cpp
        libsamplerate/samplerate.c
        libsamplerate/src_linear.c
        libsamplerate/src_sinc.c
        libsamplerate/src_zoh.c
        )

target_include_directories(libGenisysDSP PUBLIC ${CMAKE_CURRENT_LIST_DIR})