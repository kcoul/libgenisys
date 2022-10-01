#include "LibGenisysImpl.h"

LibGenisysImpl::LibGenisysImpl()
{
    int status = DS_CreateModel(PBMM_PATH, &ctx);
    if (status != 0)
    {
        /*TODO: Post failure reason */
        return;
    }

    status = DS_EnableExternalScorer(ctx, SCORER_PATH);
    if (status != 0)
    {
        /*TODO: Report failure reason */
        return;
    }

    if (hot_words)
    {
        std::vector<std::string> hot_words_ = SplitStringOnDelim(hot_words, ",");
        for ( std::string hot_word_ : hot_words_ )
        {
            std::vector<std::string> pair_ = SplitStringOnDelim(hot_word_, ":");
            const char* word = (pair_[0]).c_str();
            // the strtof function will return 0 in case of non numeric characters
            // so, check the boost string before we turn it into a float
            bool boost_is_valid = (pair_[1].find_first_not_of("-.0123456789") == std::string::npos);
            float boost = strtof((pair_[1]).c_str(),0);
            status = DS_AddHotWord(ctx, word, boost);
            if (status != 0 || !boost_is_valid)
            {
                /*TODO: Report failure reason */
                return;
            }
        }
    }
}

LibGenisysStatus LibGenisysImpl::initialize(int sampleRate)
{
    if (sampleRate < 16000)
    {
        fprintf(stderr, "Warning: original sample rate (%d) is lower than %dkHz. "
                        "Up-sampling might produce erratic speech recognition.\n", targetSampleRate, (int)sampleRate);
    }

    const int resamplerBlockSize = sampleRate;
    const int resamplerMaxSamples = maxInputSampleRate * 2;

    if(!inputResampler)
        inputResampler = std::make_unique<ResamplingFifo>(resamplerBlockSize, 1, resamplerMaxSamples);
    else if (currentInputSampleRate != sampleRate)
    {
        currentInputSampleRate = sampleRate;
        inputResampler->setResamplingRatio(currentInputSampleRate, targetSampleRate);
        inputResampler->reset();
    }

    return LibGenisysStatusOk;
}

std::string LibGenisysImpl::processFloat(float* buffer, int numSamples)
{
    return "";
}

std::string LibGenisysImpl::processNativeFloat(float* buffer, int numSamples)
{
    return "";
}

std::string LibGenisysImpl::processPath(std::string path)
{

    return "";
}

std::string LibGenisysImpl::processNativePath(std::string path)
{
    return ProcessFile(ctx, path, true);
}

ds_audio_buffer LibGenisysImpl::GetAudioBuffer(std::string path)
{
    ds_audio_buffer res = {0};

    auto inputFile = WavIO::Reader(path);
    if (!inputFile.isOpen())
    {
        std::cerr << "Could not open input file: " << path << std::endl;
    }

    auto inputHeader = inputFile.getHeader();
    if (inputHeader.numberOfChannels != 1)
    {
        std::cerr << "Number of channels in input file has to be 1, it is: " << inputHeader.numberOfChannels << std::endl;
    }

    res.buffer_size = inputFile.getHeader().lengthInBytes;
    res.buffer = (char*)malloc(sizeof(char) * res.buffer_size);

    inputFile.readData(reinterpret_cast<unsigned char*>(res.buffer), res.buffer_size);
    return res;
}

std::string LibGenisysImpl::ProcessFile(ModelState* context, std::string path, bool show_times)
{
    ds_audio_buffer audio = GetAudioBuffer(path);

    // Pass audio to DeepSpeech
    // We take half of buffer_size because buffer is a char* while
    // LocalDsSTT() expected a short*
    ds_result result = LocalDsSTT(context,
                                  (const short*)audio.buffer,
                                  audio.buffer_size / 2,
                                  extended_metadata,
                                  json_output);
    free(audio.buffer);

    if (result.string)
    {
        printf("%s\n", result.string);
        return std::string(result.string);
        DS_FreeString((char*)result.string);
    }

    if (show_times) {
        printf("cpu_time_overall=%.05f\n",
               result.cpu_time_overall);
    }

    return "";
}

ds_result LibGenisysImpl::LocalDsSTT(ModelState* aCtx, const short* aBuffer, size_t aBufferSize, bool extended_output, bool json_output)
{
    ds_result res = {0};

    clock_t ds_start_time = clock();

    // sphinx-doc: c_ref_inference_start
    if (extended_output)
    {
        Metadata *result = DS_SpeechToTextWithMetadata(aCtx, aBuffer, (unsigned int)aBufferSize, 1);
        res.string = CandidateTranscriptToString(&result->transcripts[0]);
        DS_FreeMetadata(result);
    }
    else if (json_output)
    {
        Metadata *result = DS_SpeechToTextWithMetadata(aCtx, aBuffer, (unsigned int)aBufferSize, json_candidate_transcripts);
        res.string = MetadataToJSON(result);
        DS_FreeMetadata(result);
    }
    else if (stream_size > 0)
    {
        StreamingState* ctx;
        int status = DS_CreateStream(aCtx, &ctx);

        if (status != DS_ERR_OK)
        {
            res.string = strdup("");
            return res;
        }

        size_t off = 0;
        const char *last = nullptr;
        const char *prev = nullptr;

        while (off < aBufferSize)
        {
            size_t cur = aBufferSize - off > stream_size ? stream_size : aBufferSize - off;
            DS_FeedAudioContent(ctx, aBuffer + off, (unsigned int)cur);
            off += cur;
            prev = last;
            const char* partial = DS_IntermediateDecode(ctx);

            if (last == nullptr || strcmp(last, partial))
            {
                printf("%s\n", partial);
                last = partial;
            }
            else
            {
                DS_FreeString((char *) partial);
            }

            if (prev != nullptr && prev != last)
            {
                DS_FreeString((char *) prev);
            }
        }

        if (last != nullptr)
        {
            DS_FreeString((char *) last);
        }

        res.string = DS_FinishStream(ctx);
    }
    else if (extended_stream_size > 0)
    {
        StreamingState* ctx;
        int status = DS_CreateStream(aCtx, &ctx);

        if (status != DS_ERR_OK)
        {
            res.string = strdup("");
            return res;
        }

        size_t off = 0;
        const char *last = nullptr;
        const char *prev = nullptr;

        while (off < aBufferSize)
        {
            size_t cur = aBufferSize - off > extended_stream_size ? extended_stream_size : aBufferSize - off;
            DS_FeedAudioContent(ctx, aBuffer + off, (unsigned int)cur);
            off += cur;
            prev = last;
            const Metadata* result = DS_IntermediateDecodeWithMetadata(ctx, 1);
            const char* partial = CandidateTranscriptToString(&result->transcripts[0]);

            if (last == nullptr || strcmp(last, partial))
            {
                printf("%s\n", partial);
                last = partial;
            }
            else
            {
                free((char *) partial);
            }

            if (prev != nullptr && prev != last)
            {
                free((char *) prev);
            }
            DS_FreeMetadata((Metadata *)result);
        }

        const Metadata* result = DS_FinishStreamWithMetadata(ctx, 1);
        res.string = CandidateTranscriptToString(&result->transcripts[0]);
        DS_FreeMetadata((Metadata *)result);
        free((char *) last);
    }
    else
    {
        res.string = DS_SpeechToText(aCtx, aBuffer, (unsigned int)aBufferSize);
    }
    // sphinx-doc: c_ref_inference_stop
    clock_t ds_end_infer = clock();

    res.cpu_time_overall = ((double) (ds_end_infer - ds_start_time)) / CLOCKS_PER_SEC;

    return res;
}

char* LibGenisysImpl::CandidateTranscriptToString(const CandidateTranscript* transcript)
{
    std::string retval = "";
    for (int i = 0; i < transcript->num_tokens; i++)
    {
        const TokenMetadata& token = transcript->tokens[i];
        retval += token.text;
    }
    return strdup(retval.c_str());
}

std::vector<meta_word> LibGenisysImpl::CandidateTranscriptToWords(const CandidateTranscript* transcript)
{
    std::vector<meta_word> word_list;
    std::string word = "";
    float word_start_time = 0;

    // Loop through each token
    for (int i = 0; i < transcript->num_tokens; i++)
    {
        const TokenMetadata& token = transcript->tokens[i];

        // Append token to word if it's not a space
        if (strcmp(token.text, u8" ") != 0)
        {
            // Log the start time of the new word
            if (word.length() == 0)
            {
                word_start_time = token.start_time;
            }
            word.append(token.text);
        }

        // Word boundary is either a space or the last token in the array
        if (strcmp(token.text, u8" ") == 0 || i == transcript->num_tokens-1)
        {
            float word_duration = token.start_time - word_start_time;

            if (word_duration < 0)
            {
                word_duration = 0;
            }

            meta_word w;
            w.word = word;
            w.start_time = word_start_time;
            w.duration = word_duration;
            word_list.push_back(w);

            // Reset
            word = "";
            word_start_time = 0;
        }
    }
    return word_list;
}

std::string LibGenisysImpl::CandidateTranscriptToJSON(const CandidateTranscript *transcript)
{
#ifdef WIN32
    return "";
#else
    std::ostringstream out_string;

    std::vector<meta_word> words = CandidateTranscriptToWords(transcript);

    out_string << R"("metadata":{"confidence":)" << transcript->confidence << R"(},"words":[)";

    for (int i = 0; i < words.size(); i++)
    {
        meta_word w = words[i];
        out_string << R"({"word":")" << w.word << R"(","time":)" << w.start_time << R"(,"duration":)" << w.duration << "}";

        if (i < words.size() - 1)
        {
            out_string << ",";
        }
    }

    out_string << "]";

    return out_string.str();
#endif
}

char* LibGenisysImpl::MetadataToJSON(Metadata* result)
{
#ifdef WIN32
    return "";
#else
    std::ostringstream out_string;
    out_string << "{\n";

    for (int j=0; j < result->num_transcripts; ++j)
    {
        const CandidateTranscript *transcript = &result->transcripts[j];

        if (j == 0)
        {
            out_string << CandidateTranscriptToJSON(transcript);

            if (result->num_transcripts > 1)
            {
                out_string << ",\n" << R"("alternatives")" << ":[\n";
            }
        }
        else
        {
            out_string << "{" << CandidateTranscriptToJSON(transcript) << "}";
            if (j < result->num_transcripts - 1)
            {
                out_string << ",\n";
            }
            else
            {
                out_string << "\n]";
            }
        }
    }

    out_string << "\n}\n";

    return strdup(out_string.str().c_str());
#endif
}

std::vector<std::string> LibGenisysImpl::SplitStringOnDelim(std::string in_string, std::string delim)
{
    std::vector<std::string> out_vector;
    char * tmp_str = new char[in_string.size() + 1];
    std::copy(in_string.begin(), in_string.end(), tmp_str);
    tmp_str[in_string.size()] = '\0';
    const char* token = strtok(tmp_str, delim.c_str());

    while( token != NULL )
    {
        out_vector.push_back(token);
        token = strtok(NULL, delim.c_str());
    }

    delete[] tmp_str;
    return out_vector;
}