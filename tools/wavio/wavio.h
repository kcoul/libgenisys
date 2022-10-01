//
// Created by Kamil Kisiel on 2022-09-13.
//

#pragma once

#include <string>
#include <vector>

namespace WavIO
{

struct Header {
    int format;
    int numberOfChannels;
    int sampleRate;
    int bitsPerSample;
    unsigned int lengthInBytes;
};

class Reader
{
public:
    /// Construct a reader of a file at the given path. Callers should check the result of isOpen afterwards.
    explicit Reader(std::string pathToFile);
    ~Reader();

    /// @returns whether the file is open. If there was a failure this will return false.
    inline bool isOpen() { return handle != nullptr; };

    /// @returns a pointer to the header, or nullptr if it could not be read.
    Header getHeader() const;

    /// Reads data from the WAV file into a byte buffer.
    /// @param bufferPtr pointer to the buffer.
    /// @param bufferSize size of the buffer.
    /// @returns number of bytes read.
    int readData(unsigned char* bufferPtr, unsigned int bufferSize);

    /// Reads data from the WAV file into a byte buffer.
    /// @param buffer the buffer to read into.
    /// @returns the number of bytes read.
    int readData(std::vector<unsigned char>& buffer);

    /// Reads data from the WAV file into a sample buffer.
    /// @warning Currently only 16-bit samples are supported.
    /// @param buffer the buffer to read into.
    /// @returns the number of samples read.
    int readSamples(std::vector<float>& buffer);
private:
    void* handle;
    Header header;
    std::vector<unsigned char> tmpBuffer;
};

class Writer
{
public:
    /// Construct a writer of a file at the given path. Callers should check the result of isOpen afterwards.
    Writer(std::string pathToFile, const Header& header);
   ~Writer();

    /// @returns whether the file is open. If there was a failure this will return false.
    inline bool isOpen() { return handle != nullptr; };

    /// Writes data to the WAV file from a byte buffer.
    /// @param bufferPtr pointer to the buffer.
    /// @param bufferSize number of bytes to read from the buffer.
    void writeData(const unsigned char* bufferPtr, unsigned int numberOfBytes);

    /// Writes data to the WAV file from a byte buffer.
    /// @param buffer the buffer
    /// @param numberOfBytes number of bytes to read from the buffer.
    /// @pre numberOfBytes <= buffer.size()
    void writeData(const std::vector<unsigned char>& buffer, unsigned int numberOfBytes);

    /// Writes data to the WAV file from a sample buffer.
    /// The data is converted to the appropriate bit-depth.
    /// @warning Currently only 16-bit samples are supported.
    /// @param buffer the buffer, its entire contents are written.
    void writeSamples(const std::vector<float>& buffer, unsigned int numberOfSamples);
private:
    void* handle;
    std::vector<unsigned char> tmpBuffer;
};

}