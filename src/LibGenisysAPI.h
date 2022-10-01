#pragma once

#include <string>

#if _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

/**
 * LibGenisys result status
 */
typedef enum
{
    LibGenisysStatusNoStatus =
    -1, /**<  Init State held by the client to compare against other statuses initially*/
    LibGenisysStatusOk = 0, /**< Ok status. Represents success and no errors. */
    LibGenisysUninitialized, /** LibGenisys not yet initialized */
    LibGenisysInvalidSampleRate, /**< Invalid sample rate */
    LibGenisysInternalError /**< Internal error */
} LibGenisysStatus;

/**
 * Object instance type
 */
typedef void* LibGenisysInstance;

/**
 * Creates a library instance
 *
 * Once the instance is not used anymore it should be deallocated
 * by calling the LibGenisysDestroy function.
 *
 * @returns the library instance
 */
LibGenisysInstance EXPORT LibGenisysCreate();

/**
 * Initializes the library with a sample rate for sample rate conversion
 * @returns the result status
 */
LibGenisysStatus EXPORT LibGenisysInitialize(LibGenisysInstance instance,
                                             int expectedBlockSize,
                                             int sampleRate);

/**
 * Resamples an audio buffer and runs it through DeepSpeech
 *
 * @param instance the library instance
 * @param audioBuffer the audio buffer
 * @param numberOfSamples the number of samples
 *
 * @returns the interpreted text string, if any
 */
std::string EXPORT LibGenisysProcessFloat(LibGenisysInstance instance,
                                          float* audioBuffer,
                                          int numberOfSamples);

/**
 * Runs a 16kHz audio buffer through DeepSpeech
 *
 * @param instance the library instance
 * @param audioBuffer the audio buffer
 * @param numberOfSamples the number of samples
 *
 * @returns the interpreted text string, if any
 */
std::string EXPORT LibGenisysProcessNativeFloat(LibGenisysInstance instance,
                                                float* nativeAudioBuffer,
                                                int numberOfSamples);

/**
 * Loads a file to audio buffer, resamples it and runs it through DeepSpeech
 *
 * @param instance the library instance
 * @param audioBuffer the audio buffer
 * @param numberOfSamples the number of samples
 *
 * @returns the interpreted text string, if any
 */
std::string EXPORT LibGenisysProcessPath(LibGenisysInstance instance,
                                         const char* audioFilePath);

/**
 * Loads a 16kHz file to audio buffer, and runs it through DeepSpeech
 *
 * @param instance the library instance
 * @param audioBuffer the audio buffer
 * @param numberOfSamples the number of samples
 *
 * @returns the interpreted text string, if any
 */
std::string EXPORT LibGenisysProcessNativePath(LibGenisysInstance instance,
                                               std::string nativeAudioFilePath);

/**
 * Destroys the library instance and deallocates the memory.
 *
 * @param instance the instance
 */
void EXPORT LibGenisysDestroy(LibGenisysInstance instance);