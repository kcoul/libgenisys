#pragma once

#include "CommonHeader.h"
#include "LibGenisysAPI.h"
#include "Paths.h"
#include <gin_dsp/gin_dsp.h>

namespace GuiApp
{
class MainComponent  : public juce::AudioAppComponent,
                       private juce::MidiInputCallback,
                       private juce::MidiKeyboardStateListener
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
    void startRecording();
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
    void processAudioFile(juce::File file, bool deleteAfterRender);

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

    void loadAndRenderTestFile(juce::File testFile, bool deleteAfterRender);
    void trySetReSpeakerAsDevice();

    //MIDI
    juce::ComboBox midiInputList;                     // [2]
    juce::Label midiInputListLabel;
    int lastInputIndex = 0;                           // [3]
    bool isAddingFromMidiInput = false;               // [4]

    juce::MidiKeyboardState keyboardState;            // [5]
    juce::MidiKeyboardComponent keyboardComponent;    // [6]

    juce::TextEditor midiMessagesBox;
    double startTime;

    static juce::String getMidiMessageDescription (const juce::MidiMessage& m);
    void logMessage (const juce::String& m);
    void setMidiInput (int index);
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;
    void postMessageToList (const juce::MidiMessage& message, const juce::String& source);
    void addMessageToList (const juce::MidiMessage& message, const juce::String& source);

    // This is used to dispach an incoming message to the message thread
    class IncomingMessageCallback   : public juce::CallbackMessage
    {
    public:
        IncomingMessageCallback (MainComponent* o, const juce::MidiMessage& m, const juce::String& s)
                : owner (o), message (m), source (s)
        {}

        void messageCallback() override
        {
            if (owner != nullptr)
                owner->addMessageToList (message, source);
        }

        Component::SafePointer<MainComponent> owner;
        juce::MidiMessage message;
        juce::String source;
    };


    //Temp OS-level command methods
    void OpenLogicMac();
    void OpenProToolsMac();
    void CloseLogicMac();
    void CloseProToolsMac();

    void OpenAbletonMac();
    void CloseAbletonMac();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

} // namespace GuiApp
