#pragma once

#include "CommonHeader.h"
#include "LibGenisysAPI.h"
#include "Paths.h"
#include <gin_dsp/gin_dsp.h>

namespace GuiApp
{
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const juce::String respeakerUSBDevice = "ReSpeaker 4 Mic Array (UAC1.0)";
    //==============================================================================
    //GUI elements
    juce::Path powerButtonPath;
    juce::Path stopPath;
    juce::Path recordPath;
    juce::Path pausePath;
    juce::Path playPath;
    juce::Path playOutlinedPath;
    juce::Path stopOutlinedPath;
    juce::Path micPath;
    juce::Path monitorPath;
    juce::Path playOutlinedPath2;

    juce::Colour midGray;
    juce::Colour midBlack;

    juce::ImageButton logoImageComponent;

    juce::ShapeButton recordButton{"RecordButton",
                                   juce::Colours::white,
                                   juce::Colours::lightgrey,
                                   juce::Colours::grey};
    void recordButtonClicked();
    juce::Label recordButtonLabel;
    void startRecording(const juce::File& file);
    void stop();

    juce::ShapeButton stopButton{"StopButton",
                                 juce::Colours::white,
                                 juce::Colours::lightgrey,
                                 juce::Colours::grey};
    void stopButtonClicked();
    juce::Label stopButtonLabel;
    void stopRecordingAndConvert();

    juce::ShapeButton playButton{"PlayButton",
                                 juce::Colours::white,
                                 juce::Colours::lightgrey,
                                 juce::Colours::grey};
    void playButtonClicked();
    juce::Label playButtonLabel;

    juce::ShapeButton monitorButton{"PassthroughButton",
                                    juce::Colours::white,
                                    juce::Colours::lightgrey,
                                    juce::Colours::grey};
    void monitorButtonClicked();

    juce::TextEditor textDisplay;

    juce::AudioDeviceSelectorComponent selector {
            deviceManager, 1, 1,
            2, 2,
            true, true,
            true, false};

    foleys::LevelMeterLookAndFeel lnf;
    foleys::LevelMeter meter { foleys::LevelMeter::MeterFlags::Default };
    foleys::LevelMeterSource meterSource;

    //DSP
    std::unique_ptr<juce::AudioBuffer<float>> resamplerBuffer;
    std::unique_ptr<gin::ResamplingFifo> inputResampler;
    const int targetSampleRate = 16000;
    const int maxInputSampleRate = 96000;

    int currentBlockSize = 0;
    int currentSampleRate = 0;

    LibGenisysInstance libGenisysInstance;

    bool currentlyRecording = false;
    bool transportLoaded = false;
    bool currentlyPlaying = false;
    bool enablePassthrough = false;

    std::unique_ptr<juce::AudioFormatReaderSource> newSource;
    juce::AudioTransportSource transportSource;

    juce::AudioFormatManager formatManager;
    juce::File lastRecording;
    juce::AudioBuffer<float> loadedAudioBuffer;

    juce::TimeSliceThread backgroundThread { "Audio Recorder Thread" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    int lastRecordingSize;

    void loadAndRenderTestFile(juce::File testFile);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

} // namespace GuiApp
