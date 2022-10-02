#include "MainComponent.h"
#include "BinaryHelper.h"
namespace GuiApp
{
//==============================================================================
MainComponent::MainComponent()
{
    powerButtonPath.loadPathFromData (powerButtonPathData, sizeof (powerButtonPathData));
    stopPath.loadPathFromData (stopPathData, sizeof (stopPathData));
    recordPath.loadPathFromData (recordPathData, sizeof (recordPathData));
    pausePath.loadPathFromData (pausePathData, sizeof (pausePathData));
    playPath.loadPathFromData (playPathData, sizeof (playPathData));
    playOutlinedPath.loadPathFromData (playOutlinedPathData, sizeof (playOutlinedPathData));
    stopOutlinedPath.loadPathFromData (stopOutlinedPathData, sizeof (stopOutlinedPathData));
    micPath.loadPathFromData (micPathData, sizeof (micPathData));
    monitorPath.loadPathFromData (monitorPathData, sizeof (monitorPathData));
    playOutlinedPath2.loadPathFromData (playOutlinedPathData2, sizeof (playOutlinedPathData2));
    midGray = juce::Colour(0xFF425568);
    midBlack = juce::Colour(0xFF100B28);

    backgroundThread.startThread();
    formatManager.registerBasicFormats();

    libGenisysInstance = LibGenisysCreate();

    //Initial size big enough for most Desktop and Laptop displays
    //TODO: Make resizable layout
    setSize (1600, 900);

    setOpaque(true);

    addAndMakeVisible(selector);

    monitorButton.setShape(monitorPath, true, false, true);
    monitorButton.onClick = [this] { monitorButtonClicked(); };
    addAndMakeVisible(monitorButton);

    auto images = getBinaryDataImages();

    recordButton.setShape(micPath, true, false, true);
    recordButton.onClick = [this] { recordButtonClicked(); };
    addAndMakeVisible(recordButton);

    recordButtonLabel.setText("Record", juce::NotificationType::dontSendNotification);
    recordButtonLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(recordButtonLabel);

    stopButton.setShape(stopOutlinedPath, true, false, true);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);

    stopButtonLabel.setText("Stop", juce::NotificationType::dontSendNotification);
    stopButtonLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stopButtonLabel);

    playButton.setShape(playOutlinedPath2, true, false, true);
    playButton.onClick = [this] { playButtonClicked(); };
    addAndMakeVisible(playButton);

    playButtonLabel.setText("Play", juce::NotificationType::dontSendNotification);
    playButtonLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(playButtonLabel);

    textDisplay.setText("The quick brown fox jumps over the lazy dog", juce::dontSendNotification);
    textDisplay.setMultiLine(true, true);
    textDisplay.setReadOnly(true);
    textDisplay.setColour(juce::TextEditor::backgroundColourId, midGray);
    textDisplay.setFont(juce::Font(64));
    textDisplay.setJustification(juce::Justification::centred);
    addAndMakeVisible(textDisplay);

    lnf.setColour(foleys::LevelMeter::lmBackgroundColour, midGray);
    lnf.setColour (foleys::LevelMeter::lmMeterGradientLowColour, juce::Colours::green);
    lnf.setColour(foleys::LevelMeter::lmTicksColour, midBlack);

    meter.setLookAndFeel (&lnf);
    meter.setMeterSource (&meterSource);
    addAndMakeVisible (meter);

    auto parentDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    lastRecording = parentDir.getChildFile("GenisysTestRecording.wav");
    loadAndRenderTestFile(lastRecording);

    for (auto device : deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(true))
        if (device.equalsIgnoreCase(respeakerUSBDevice))
        {
            auto setup = deviceManager.getAudioDeviceSetup();
            setup.inputDeviceName = respeakerUSBDevice;
            setup.outputDeviceName = respeakerUSBDevice;
            deviceManager.setAudioDeviceSetup(setup, true);
            break;
        }

    setAudioChannels(2,2);
}

void MainComponent::loadAndRenderTestFile(juce::File testFile)
{
    juce::AudioFormatReader* reader = formatManager.createReaderFor(lastRecording);
    if (reader)
    {
        lastRecordingSize = sizeof(unsigned char) * reader->lengthInSamples * 2;

        loadedAudioBuffer = juce::AudioBuffer<float>(1, reader->lengthInSamples);
        reader->read(&loadedAudioBuffer, 0, reader->lengthInSamples,
                     0, true, true);

        newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);   // [11]
        transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
        transportLoaded = true;

        auto result = LibGenisysProcessNativePath(libGenisysInstance, testFile.getFullPathName().toStdString());
        if (!result.empty())
            textDisplay.setText(juce::String(result), juce::dontSendNotification);
    }
}

void MainComponent::monitorButtonClicked()
{
    monitorButton.setToggleState(!monitorButton.getToggleState(), juce::dontSendNotification);
    enablePassthrough = monitorButton.getToggleState();
}

MainComponent::~MainComponent()
{
    shutdownAudio();
    LibGenisysDestroy(libGenisysInstance);
    meter.setLookAndFeel (nullptr);
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    meterSource.resize (1, static_cast<int>(sampleRate * 0.1 / samplesPerBlockExpected));

    if (sampleRate < targetSampleRate)
    {
        fprintf(stderr, "Warning: original sample rate (%d) is lower than %dkHz. "
                        "Up-sampling might produce erratic speech recognition.\n", targetSampleRate, (int)sampleRate);
    }

    const int resamplerBlockSize = samplesPerBlockExpected;
    const int resamplerMaxSamples = maxInputSampleRate * 2;

    if (currentBlockSize != samplesPerBlockExpected || currentSampleRate != sampleRate) {
        meterSource.resize(1, static_cast<int>(sampleRate * 0.1 / samplesPerBlockExpected));
        LibGenisysInitialize(libGenisysInstance, samplesPerBlockExpected, sampleRate);
    }

    if (currentBlockSize != samplesPerBlockExpected)
    {
        currentBlockSize = samplesPerBlockExpected;
        resamplerBuffer = std::make_unique<juce::AudioBuffer<float>>(1, currentBlockSize);

        if (!inputResampler)
            inputResampler = std::make_unique<gin::ResamplingFifo>(resamplerBlockSize, 1, resamplerMaxSamples);
        else
            inputResampler->setSize(resamplerBlockSize, 1, resamplerMaxSamples);
    }

    if (currentSampleRate != sampleRate)
    {
        currentSampleRate = sampleRate;
        inputResampler->setResamplingRatio(currentSampleRate, targetSampleRate);
        inputResampler->reset();
    }

    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);

    meterSource.resize (1, static_cast<int>(sampleRate * 0.1 / samplesPerBlockExpected));
}

    void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        meterSource.measureBlock (*bufferToFill.buffer);

        if (currentlyRecording)
        {
            inputResampler.get()->pushAudioBuffer(*bufferToFill.buffer);

            while (inputResampler.get()->samplesReady() >= currentBlockSize)
            {
                inputResampler.get()->popAudioBuffer(*resamplerBuffer);

                const juce::ScopedLock sl (writerLock);

                if (activeWriter.load() != nullptr)
                {
                    activeWriter.load()->write(resamplerBuffer->getArrayOfReadPointers(), currentBlockSize);
                }
            }
        }

        if (currentlyPlaying)
        {
            transportSource.getNextAudioBlock(bufferToFill);
            if (transportSource.hasStreamFinished())
            {
                juce::MessageManager::callAsync(
                        [=] ()
                        {
                            transportSource.setPosition(0);
                            recordButton.setEnabled(true);
                            stopButton.setEnabled(false);
                            playButton.setEnabled(true);
                        }
                );
            }
        }
        else if (!enablePassthrough)
            bufferToFill.clearActiveBufferRegion();
    }

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (midGray);
}

void MainComponent::resized()
{
    recordButton.setBounds(125,0, 100,100);
    stopButton.setBounds(275,0, 100,100);
    playButton.setBounds(425,0, 100,100);
    monitorButton.setBounds(665, 0, 100, 100);

    selector.setBounds(100, 125, getWidth()-100, getHeight()-200);

    textDisplay.setBounds(0, getHeight()-100,getWidth(), 100);

    meter.setBounds(0, 0, 100, getHeight()-100);
}

void MainComponent::recordButtonClicked()
{
    recordButton.setEnabled(false);
    playButton.setEnabled(false);
    stopButton.setEnabled(true);

    auto parentDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    lastRecording = parentDir.getNonexistentChildFile ("GenisysTestRecording", ".wav");

    if (lastRecording.existsAsFile())
        lastRecording.deleteFile();

    startRecording (lastRecording);
    currentlyRecording = true;
}

void MainComponent::stopButtonClicked()
{
    if (currentlyRecording)
    {
        stopRecordingAndConvert();
    }
    if (currentlyPlaying)
    {
        transportSource.stop();
        currentlyPlaying = false;
    }

    recordButton.setEnabled(true);
    stopButton.setEnabled(false);
    playButton.setEnabled(true);
}

void MainComponent::playButtonClicked()
{
    if (transportLoaded)
    {
        transportSource.start();
        currentlyPlaying = true;
        recordButton.setEnabled(false);
        stopButton.setEnabled(true);
    }
}

void MainComponent::startRecording(const juce::File& file)
{
    stop();

    if (currentSampleRate > 0)
    {
        file.deleteFile();

        if (auto fileStream = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream()))
        {
            juce::WavAudioFormat wavFormat;

            if (auto writer = wavFormat.createWriterFor (fileStream.get(), targetSampleRate, 1, 16, {}, 0))
            {
                fileStream.release();

                threadedWriter.reset (new juce::AudioFormatWriter::ThreadedWriter (writer, backgroundThread, 32768));

                const juce::ScopedLock sl (writerLock);
                activeWriter = threadedWriter.get();
            }
        }
    }
}

void MainComponent::stop()
{
    const juce::ScopedLock sl (writerLock);
    activeWriter = nullptr;
    threadedWriter.reset();
}

void MainComponent::stopRecordingAndConvert()
{
    currentlyRecording = false;
    stop();

    auto result = LibGenisysProcessNativePath(libGenisysInstance, lastRecording.getFullPathName().toStdString());
    if (!result.empty())
        textDisplay.setText(juce::String(result), juce::dontSendNotification);
}
} // namespace GuiApp