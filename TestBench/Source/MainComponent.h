#pragma once

#include "CommonHeader.h"
#include "LibGenisysAPI.h"
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
    //==============================================================================
    //GUI elements
    juce::ImageButton logoImageComponent;

    juce::ImageButton recordButton;
    void recordButtonClicked();
    juce::Label recordButtonLabel;
    void startRecording(const juce::File& file);
    void stop();

    juce::ImageButton stopButton;
    void stopButtonClicked();
    juce::Label stopButtonLabel;
    void stopRecordingAndConvert();

    juce::ImageButton playButton;
    void playButtonClicked();
    juce::Label playButtonLabel;

    juce::ToggleButton settingsButton;
    void settingsButtonClicked();

    juce::ImageButton quitCPanelButton;
    static void quitCPanelButtonClicked();
    juce::Label quitCPanelLabel;

    juce::ImageButton enablePassthroughButton;
    void enablePassthroughButtonClicked();
    juce::Label enablePassthroughLabel;

    juce::TextEditor textDisplay;

    juce::AudioDeviceSelectorComponent selector {
            deviceManager, 1, 1,
            2, 2,
            false, false,
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
