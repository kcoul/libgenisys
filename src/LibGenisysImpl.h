#pragma once

#include "deepspeech.h"
#include "gin/gin_resamplingfifo.h"
#include "LibGenisysAPI.h"
#include "wavio.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <string>

typedef struct {
    const char* string;
    double cpu_time_overall;
} ds_result;

struct meta_word {
    std::string word;
    float start_time;
    float duration;
};

typedef struct {
    char*  buffer;
    size_t buffer_size;
} ds_audio_buffer;

class LibGenisysImpl
{
public:
    LibGenisysImpl();
    ~LibGenisysImpl();
    LibGenisysStatus initialize(int expectedBlockSize, int sampleRate);
    std::string processFloat(float* buffer, int numSamples);
    std::string processNativeFloat(float* buffer, int numSamples);
    std::string processPath(std::string path);
    std::string processNativePath(std::string path);
private:
    //Resampler
    std::unique_ptr<ResamplingFifo> inputResampler;
    const int targetSampleRate = 16000;
    const int maxInputSampleRate = 96000;
    int currentInputSampleRate;
    int currentBlockSize;

    //DeepSpeech
    ModelState* ctx;

    ds_audio_buffer GetAudioBuffer(std::string path);
    std::string ProcessFile(ModelState* context, std::string path, bool show_times);
    ds_result LocalDsSTT(ModelState* aCtx, const short* aBuffer, size_t aBufferSize, bool extended_output, bool json_output);

    const char* hot_words = "genesis:5,open:3,close:3,pro:3,tools:3";
    //float max_boost = 20.0f; //See: https://deepspeech.readthedocs.io/en/master/HotWordBoosting-Examples.html
    /*
     * Overly positive boost values may cause a word following the boosted hot-word to be split into separate letters.
     * This problem is related to the scorer structure and currently only way to avoid it is to tune boost to a lower value.
     */

    bool extended_metadata = false;
    bool json_output = false;
    int json_candidate_transcripts = 3;
    int stream_size = 0;
    int extended_stream_size = 0;

    //==============================================================================
    char* CandidateTranscriptToString(const CandidateTranscript* transcript);
    std::vector<meta_word> CandidateTranscriptToWords(const CandidateTranscript* transcript);
    std::string CandidateTranscriptToJSON(const CandidateTranscript *transcript);
    char* MetadataToJSON(Metadata* result);
    std::vector<std::string> SplitStringOnDelim(std::string in_string, std::string delim);

    //Temp OS command methods
    void OpenProTools();
    void CloseProTools();
};

