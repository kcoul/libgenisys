#include "LibGenisysImpl.h"

LibGenisysImpl::LibGenisysImpl()
{
    // RNNoise
    //Use the default model (must be done before registering the callback below)
    st = rnnoise_create(NULL);

//TODO: we probably only need the DEBUG_PATH and an INSTALL_PATH, but need to make a macOS installer to start with
//TODO: Use Packages as in the past: http://s.sudre.free.fr/Software/Packages/about.html

//TODO: Additionally, we can now use int
//STT_CreateModelFromBuffer(const char* aModelBuffer,
//                          unsigned int aBufferSize,
//                          ModelState** retval)
//TODO: to load a model and scorer embedded in the binary itself, which is probably the way to go

#if DEBUG_PATH
    int status = STT_CreateModel(TFLITE_PATH, &ctx);
#elseif BUNDLE_PATH
    CFURLRef tfliteUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(),
                                                  CFSTR("model.tflite"),
                                                 NULL, NULL);
    const char* tflitePtr = CFStringGetCStringPtr(CFURLGetString(tfliteUrlRef),kCFStringEncodingUTF8);
    int status = STT_CreateModel(tflitePtr, &ctx);
#else
    int status = STT_CreateModel("/usr/local/bin/model.tflite", &ctx);
#endif
    if (status != 0)
    {
        /*TODO: Post failure reason */
        return;
    }

#if DEBUG_PATH
    status = STT_EnableExternalScorer(ctx, SCORER_PATH);
#elseif BUNDLE_PATH
    CFURLRef scorerUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("coqui-stt-0.9.3-models.scorer"),
                                                 NULL, NULL);
    const char* scorerPtr = CFStringGetCStringPtr(CFURLGetString(scorerUrlRef),kCFStringEncodingUTF8);
    status = STT_EnableExternalScorer(ctx, scorerPtr);
#else
    status = STT_EnableExternalScorer(ctx, "/usr/local/bin/coqui-stt-0.9.3-models.scorer");
#endif
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
            status = STT_AddHotWord(ctx, word, boost);
            if (status != 0 || !boost_is_valid)
            {
                /*TODO: Report failure reason */
                return;
            }
        }
    }
}

LibGenisysImpl::~LibGenisysImpl()
{
    rnnoise_destroy(st);
}

LibGenisysStatus LibGenisysImpl::initialize(int expectedBlockSize, int sampleRate)
{
    if (sampleRate < 16000)
    {
        fprintf(stderr, "Warning: original sample rate (%d) is lower than %dkHz. "
                        "Up-sampling might produce erratic speech recognition.\n", targetSampleRate, (int)sampleRate);
    }
    if (sampleRate != 16000)
    {
        if (!inputResampler)
        {
            currentBlockSize = expectedBlockSize;
            const int resamplerMaxSamples = maxInputSampleRate * 2;
            inputResampler = std::make_unique<ResamplingFifo>(expectedBlockSize, 1, resamplerMaxSamples);
        } else if (currentBlockSize != expectedBlockSize)
        {
            currentBlockSize = expectedBlockSize;
            const int resamplerMaxSamples = maxInputSampleRate * 2;
            inputResampler->setSize(currentBlockSize, 1, resamplerMaxSamples);
        }
        if (currentInputSampleRate != sampleRate)
        {
            currentInputSampleRate = sampleRate;
            inputResampler->setResamplingRatio(currentInputSampleRate, targetSampleRate);
            inputResampler->reset();
        }
    } else { currentInputSampleRate = sampleRate; } //To bypass resampler at process() time

    return LibGenisysStatusOk;
}

std::string LibGenisysImpl::processFloat(float* buffer, int numSamples)
{
    return "Not yet implemented"; //TODO: rebuffer through resamplingFIFO
}

std::string LibGenisysImpl::processPath(std::string path)
{

    return "Not yet implemented"; //TODO: pass entire file through resamplingFIFO and then process it
}

std::string LibGenisysImpl::processNativeFloat(float* buffer, int numSamples)
{
    ds_audio_buffer res = {0};
    res.buffer_size = numSamples * sizeof(float);
    res.buffer = (char*)malloc(sizeof(char) * res.buffer_size);

    juce::AudioDataConverters::convertFloatToInt16LE(buffer, res.buffer, numSamples);

    ds_result result = LocalDsSTT(ctx,
                                  (const short*)res.buffer,
                                  res.buffer_size / 2,
                                  extended_metadata,
                                  json_output);
    free(res.buffer);

    if (result.string)
    {
        printf("%s\n", result.string);
        auto ret = std::string(result.string);

        //TODO: WIP: Optimize string massaging
        ret = std::regex_replace(ret, std::regex("^ +| +$|( ) +"), "$1");

        STT_FreeString((char*)result.string);
        return std::string(ret);
    }

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

ds_audio_buffer LibGenisysImpl::DeNoiseAudioBuffer(ds_audio_buffer &input)
{
    if (!denoisingBuffer)
        denoisingBuffer = std::make_unique<juce::AudioBuffer<float>>(1, input.buffer_size);
    else
        denoisingBuffer->setSize(1, input.buffer_size);

    juce::AudioDataConverters::convertInt16LEToFloat(input.buffer, denoisingBuffer->getWritePointer(0), input.buffer_size);

    rnnoise_process_frame(st, denoisingBuffer->getWritePointer(0), denoisingBuffer->getReadPointer(0));
    
    juce::AudioDataConverters::convertFloatToInt16LE(denoisingBuffer->getReadPointer(0), input.buffer, input.buffer_size);

    return input;
}

std::string LibGenisysImpl::ProcessFile(ModelState* context, std::string path, bool show_times)
{
    ds_audio_buffer audio = GetAudioBuffer(path);

    //DeNoiseAudioBuffer(audio);

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
        auto ret = std::string(result.string);

        //TODO: WIP: Optimize string massaging
        ret = std::regex_replace(ret, std::regex("^ +| +$|( ) +"), "$1");

        STT_FreeString((char*)result.string);
        return std::string(ret);
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
        Metadata *result = STT_SpeechToTextWithMetadata(aCtx, aBuffer, (unsigned int)aBufferSize, 1);
        res.string = CandidateTranscriptToString(&result->transcripts[0]);
        STT_FreeMetadata(result);
    }
    else if (json_output)
    {
        Metadata *result = STT_SpeechToTextWithMetadata(aCtx, aBuffer, (unsigned int)aBufferSize, json_candidate_transcripts);
        res.string = MetadataToJSON(result);
        STT_FreeMetadata(result);
    }
    else if (stream_size > 0)
    {
        StreamingState* ctx;
        int status = STT_CreateStream(aCtx, &ctx);

        if (status != STT_ERR_OK)
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
            STT_FeedAudioContent(ctx, aBuffer + off, (unsigned int)cur);
            off += cur;
            prev = last;
            const char* partial = STT_IntermediateDecode(ctx);

            if (last == nullptr || strcmp(last, partial))
            {
                printf("%s\n", partial);
                last = partial;
            }
            else
            {
                STT_FreeString((char *) partial);
            }

            if (prev != nullptr && prev != last)
            {
                STT_FreeString((char *) prev);
            }
        }

        if (last != nullptr)
        {
            STT_FreeString((char *) last);
        }

        res.string = STT_FinishStream(ctx);
    }
    else if (extended_stream_size > 0)
    {
        StreamingState* ctx;
        int status = STT_CreateStream(aCtx, &ctx);

        if (status != STT_ERR_OK)
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
            STT_FeedAudioContent(ctx, aBuffer + off, (unsigned int)cur);
            off += cur;
            prev = last;
            const Metadata* result = STT_IntermediateDecodeWithMetadata(ctx, 1);
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
            STT_FreeMetadata((Metadata *)result);
        }

        const Metadata* result = STT_FinishStreamWithMetadata(ctx, 1);
        res.string = CandidateTranscriptToString(&result->transcripts[0]);
        STT_FreeMetadata((Metadata *)result);
        free((char *) last);
    }
    else
    {
        res.string = STT_SpeechToText(aCtx, aBuffer, (unsigned int)aBufferSize);
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
}

char* LibGenisysImpl::MetadataToJSON(Metadata* result)
{
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