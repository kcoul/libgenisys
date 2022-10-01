#include "LibGenisysAPI.h"
#include "LibGenisysImpl.h"

LibGenisysInstance LibGenisysCreate()
{
    LibGenisysImpl* impl = new LibGenisysImpl();
    return impl;
}

LibGenisysStatus LibGenisysInitialize(LibGenisysInstance instance,
                                             int sampleRate)
{
    LibGenisysImpl* impl = (LibGenisysImpl*)instance;
    return impl->initialize(sampleRate);
}

std::string LibGenisysProcessFloat(LibGenisysInstance instance,
                                   float* audioBuffer,
                                   int numberOfSamples)
{
    LibGenisysImpl* impl = (LibGenisysImpl*)instance;
    return impl->processFloat(audioBuffer, numberOfSamples);
}

std::string LibGenisysProcessNativeFloat(LibGenisysInstance instance,
                                         float* nativeAudioBuffer,
                                         int numberOfSamples)
{
    LibGenisysImpl* impl = (LibGenisysImpl*)instance;
    return impl->processNativeFloat(nativeAudioBuffer, numberOfSamples);
}

std::string LibGenisysProcessPath(LibGenisysInstance instance,
                                  std::string audioFilePath)
{
    LibGenisysImpl* impl = (LibGenisysImpl*)instance;
    return impl->processPath(audioFilePath);
}

std::string LibGenisysProcessNativePath(LibGenisysInstance instance,
                                        std::string nativeAudioFilePath)
{
    LibGenisysImpl* impl = (LibGenisysImpl*)instance;
    return impl->processNativePath(nativeAudioFilePath);
}

void LibGenisysDestroy(LibGenisysInstance instance)
{
    LibGenisysImpl* impl = (LibGenisysImpl*)instance;
    delete impl;
}
