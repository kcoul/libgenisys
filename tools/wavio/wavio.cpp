//
// Created by Kamil Kisiel on 2022-09-13.
//

#include "wavio.h"

#include "wavreader.h"
#include "wavwriter.h"

#include <algorithm>
#include <cassert>

namespace WavIO {

static void convertInt16ToFloat(const int16_t* inputBuffer, float* outputBuffer, int numberOfSamples)
{
    for (int i = 0; i < numberOfSamples; i++)
    {
        float sampleFloat = static_cast<float>(inputBuffer[i]) / static_cast<float>(INT16_MAX);
        outputBuffer[i] = std::clamp(sampleFloat, -1.0f, 1.0f);
    }
}

static void convertFloatToInt16(const float* inputBuffer, int16_t* outputBuffer, int numberOfSamples)
{
    for (int i = 0; i < numberOfSamples; i++)
    {
        outputBuffer[i] = static_cast<int16_t>(std::clamp(inputBuffer[i], -1.0f, 1.0f) * INT16_MAX);
    }
}

Reader::Reader(std::string pathToFile) {
    handle = wav_read_open(pathToFile.c_str());
    if (handle == nullptr) {
        return;
    }
    auto getHeaderResult = wav_get_header(
        handle, &header.format, &header.numberOfChannels, &header.sampleRate, &header.bitsPerSample, &header.lengthInBytes);
    if (!getHeaderResult) {
        wav_read_close(handle);
        handle = nullptr;
        return;
    }
    tmpBuffer.resize(256);
}

Reader::~Reader() {
    if (handle != nullptr) {
        wav_read_close(handle);
    }
}

Header Reader::getHeader() const
{
    return header;
}

int Reader::readData(unsigned char* bufferPtr, unsigned int bufferSize)
{
    return wav_read_data(handle, bufferPtr, bufferSize);
}

int Reader::readData(std::vector<unsigned char>& buffer)
{
    return readData(buffer.data(), buffer.size());
}

int Reader::readSamples(std::vector<float>& buffer)
{
    if (header.bitsPerSample != 16) {
       // bit depths other than 16 are not yet supported.
       assert(false);
    }

    auto bytesToRead = buffer.size() * sizeof (int16_t);
    if (tmpBuffer.size() < bytesToRead) {
        tmpBuffer.resize(bytesToRead);
    }

    auto numBytesRead = readData(tmpBuffer.data(), bytesToRead);
    int numSamplesRead = numBytesRead * 8 / header.bitsPerSample;
    convertInt16ToFloat((int16_t*)tmpBuffer.data(), buffer.data(), numSamplesRead);
    return numSamplesRead;
}

Writer::Writer(std::string pathToFile, const Header& header)
{
    handle = wav_write_open(pathToFile.c_str(), header.sampleRate, header.bitsPerSample, header.numberOfChannels, header.lengthInBytes);
    tmpBuffer.resize(256);
}

Writer::~Writer() {
    if (handle != nullptr) {
        wav_write_close(handle);
    }
}

void Writer::writeData(const unsigned char* bufferPtr, unsigned int bufferSize)
{
    wav_write_data(handle, bufferPtr, bufferSize);
}

void Writer::writeData(const std::vector<unsigned char>& buffer, unsigned int numberOfBytes)
{
    assert(numberOfBytes <= buffer.size());

    writeData(buffer.data(), numberOfBytes);
}

void Writer::writeSamples(const std::vector<float>& buffer, unsigned int numberOfSamples)
{
    assert(numberOfSamples <= buffer.size());

    unsigned int maxSamplesToWrite = tmpBuffer.size() / sizeof(float);
    while (numberOfSamples > 0) {
        auto samplesToWrite = std::min(numberOfSamples, maxSamplesToWrite);
        convertFloatToInt16(buffer.data(), (int16_t*)tmpBuffer.data(), samplesToWrite);
        writeData(tmpBuffer, samplesToWrite * sizeof(int16_t));
        numberOfSamples -= samplesToWrite;
    }
}

}