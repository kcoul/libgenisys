/*
 ==============================================================================

 This file is part of the GIN library.
 Copyright (c) 2019 - Roland Rabien.

 ==============================================================================
 */

#pragma once

#include <assert.h>
#include <memory>

#include "gin_audiofifo.h"
#include "juce/juce_AudioDataConverters.h"
#include "juce/juce_AudioSampleBuffer.h"
#include "libsamplerate/samplerate.h"

/** ResamplingFifo - uses secret rabbit code
 */
class ResamplingFifo
{
public:
    ResamplingFifo() = delete;
    ResamplingFifo(int blockSz, int numCh = 2, int maxSamples = 44100)
    {
        impl = std::make_unique<Impl>();

        setSize(blockSz, numCh, maxSamples);
    }

    ~ResamplingFifo() { src_delete(impl->state); }

    void setSize(int blockSz, int numCh = 2, int maxSamples = 44100)
    {
        numChannels = numCh;
        blockSize = blockSz;

        int error = 0;
        impl->state = src_new(SRC_SINC_FASTEST, numChannels, &error);

        outputFifo.setSize(numChannels, maxSamples);

        ilInputBuffer.setSize(1, blockSize * numChannels);
        ilOutputBuffer.setSize(1, 4 * blockSize * numChannels);
        outputBuffer.setSize(numChannels, 4 * blockSize);
    }

    void setResamplingRatio(double inputRate, double outputRate)
    {
        ratio = float(std::max(0.0, outputRate / inputRate));
    }

    void setRatio(float r) { ratio = r; }

    void reset()
    {
        src_reset(impl->state);
        src_set_ratio(impl->state, ratio);

        outputFifo.reset();
    }

    int samplesReady() { return outputFifo.getNumReady(); }

    void pushAudioBuffer(const juce::AudioSampleBuffer& buffer)
    {
        if (buffer.getNumSamples() <= blockSize)
        {
            pushAudioBufferInt(buffer);
        }
        else
        {
            int todo = buffer.getNumSamples();
            int offset = 0;

            while (todo > 0)
            {
                int thisBlock = std::min(todo, blockSize);

                const juce::AudioSampleBuffer slice(
                    (float**)buffer.getArrayOfReadPointers(), buffer.getNumChannels(), offset, thisBlock);

                pushAudioBufferInt(slice);
                todo -= thisBlock;
                offset += thisBlock;
            }
        }
    }

    void popAudioBuffer(juce::AudioSampleBuffer& buffer) { outputFifo.read(buffer); }

private:
    void pushAudioBufferInt(const juce::AudioSampleBuffer& buffer)
    {
        assert(buffer.getNumSamples() <= blockSize);

        int todo = buffer.getNumSamples();
        int done = 0;

        juce::AudioDataConverters::interleaveSamples(
            buffer.getArrayOfReadPointers(), ilInputBuffer.getWritePointer(0), buffer.getNumSamples(), numChannels);

        SRC_DATA data;
        data.data_in = ilInputBuffer.getReadPointer(0);
        data.data_out = ilOutputBuffer.getWritePointer(0);
        data.output_frames = 4 * blockSize;
        data.src_ratio = ratio;
        data.end_of_input = 0;

        while (todo > 0)
        {
            data.input_frames = todo;
            data.input_frames_used = 0;
            data.output_frames_gen = 0;

            data.data_in = ilInputBuffer.getReadPointer(0) + done * numChannels;

            src_process(impl->state, &data);

            todo -= data.input_frames_used;
            done += data.input_frames_used;

            if (data.output_frames_gen > 0)
            {
                juce::AudioDataConverters::deinterleaveSamples(ilOutputBuffer.getReadPointer(0),
                                                               outputBuffer.getArrayOfWritePointers(),
                                                               int(data.output_frames_gen),
                                                               numChannels);

                outputFifo.write(outputBuffer, int(data.output_frames_gen));
            }
        }
    }

    struct Impl
    {
        SRC_STATE* state = nullptr;
    };
    std::unique_ptr<Impl> impl;

    int numChannels = 0, blockSize = 0;
    float ratio = 1.0f;
    AudioFifo outputFifo;
    juce::AudioSampleBuffer ilInputBuffer, ilOutputBuffer, outputBuffer;
};
