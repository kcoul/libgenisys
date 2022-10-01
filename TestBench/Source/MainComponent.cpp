#include "MainComponent.h"
#include "BinaryHelper.h"
namespace GuiApp
{
//==============================================================================
MainComponent::MainComponent()
{
    backgroundThread.startThread();
    formatManager.registerBasicFormats();

    libGenisysInstance = LibGenisysCreate();

    //Initial size big enough for most Desktop and Laptop displays
    //TODO: Make resizable layout
    setSize (1600, 900);

    setOpaque(true);

//Selector Pane
    addAndMakeVisible(selector);
    selector.setVisible(false);

    quitCPanelLabel.setText("Quit CPanel", juce::NotificationType::dontSendNotification);
    quitCPanelLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(quitCPanelLabel);
    quitCPanelLabel.setVisible(false);

    quitCPanelButton.setButtonStyle(HackAudio::Button::ButtonStyle::Bar);
    quitCPanelButton.onClick = [] { quitCPanelButtonClicked(); };
    quitCPanelButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(quitCPanelButton);
    quitCPanelButton.setVisible(false);

    enablePassthroughLabel.setText("Enable Passthrough",
                                   juce::NotificationType::dontSendNotification);
    enablePassthroughLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(enablePassthroughLabel);
    enablePassthroughLabel.setVisible(false);

    enablePassthroughButton.setButtonStyle(HackAudio::Button::ButtonStyle::BarToggle);
    enablePassthroughButton.onClick = [this] { enablePassthroughButtonClicked(); };
    addAndMakeVisible(enablePassthroughButton);
    enablePassthroughButton.setVisible(false);

//Main Pane
    auto images = getBinaryDataImages();

    logoImageComponent.setImages(true, true, true,
                                 images.front(), 1.0f, juce::Colours::transparentWhite,
                                 images.front(), 1.0f, juce::Colours::transparentWhite,
                                 images.front(), 1.0f, juce::Colours::transparentWhite);
    logoImageComponent.onClick = [this] { settingsButtonClicked(); };
    addAndMakeVisible(logoImageComponent);

    recordButton.setButtonStyle(HackAudio::Button::ButtonStyle::Bar);
        recordButton.onClick = [this] { recordButtonClicked(); };
    addAndMakeVisible(recordButton);

    recordButtonLabel.setText("Record", juce::NotificationType::dontSendNotification);
    recordButtonLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(recordButtonLabel);

    stopButton.setButtonStyle(HackAudio::Button::ButtonStyle::Bar);
        stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);

    stopButtonLabel.setText("Stop", juce::NotificationType::dontSendNotification);
    stopButtonLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stopButtonLabel);

    playButton.setButtonStyle(HackAudio::Button::ButtonStyle::Bar);
        playButton.onClick = [this] { playButtonClicked(); };
    addAndMakeVisible(playButton);

    playButtonLabel.setText("Play", juce::NotificationType::dontSendNotification);
    playButtonLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(playButtonLabel);

    textDisplay.setText("The quick brown fox jumps over the lazy dog", juce::dontSendNotification);
    textDisplay.setMultiLine(true, true);
    textDisplay.setReadOnly(true);
    textDisplay.setColour(juce::TextEditor::backgroundColourId, quitCPanelButton.findColour(HackAudio::midgroundColourId));
    textDisplay.setFont(juce::Font(64));
    addAndMakeVisible(textDisplay);

    lnf.setColour(foleys::LevelMeter::lmBackgroundColour, quitCPanelButton.findColour(HackAudio::midgroundColourId));
    lnf.setColour (foleys::LevelMeter::lmMeterGradientLowColour, juce::Colours::green);
    lnf.setColour(foleys::LevelMeter::lmTicksColour,
                  quitCPanelButton.findColour(HackAudio::backgroundColourId));

    meter.setLookAndFeel (&lnf);
    meter.setMeterSource (&meterSource);
    addAndMakeVisible (meter);

#ifdef __linux__
    auto audioError = deviceManager.initialiseWithDefaultDevices(1, 2);
    jassert (audioError.isEmpty());

    auto setup = deviceManager.getAudioDeviceSetup();
    setup.bufferSize = 2048;
    deviceManager.setAudioDeviceSetup(setup, true);

    deviceManager.addAudioCallback (&audioSourcePlayer);
    audioSourcePlayer.setSource (this);
#else
    setAudioChannels(1,2);
#endif

    auto parentDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    lastRecording = parentDir.getChildFile("GenisysTestRecording.wav");
    loadAndRenderTestFile(lastRecording);
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

        auto result = LibGenisysProcessNativePath(libGenisysInstance, testFile.getFullPathName().toStdString());
        if (!result.empty())
            textDisplay.setText(juce::String(result), juce::dontSendNotification);
    }
}

void MainComponent::quitCPanelButtonClicked()
{
    juce::JUCEApplicationBase::quit();
}

void MainComponent::enablePassthroughButtonClicked()
{
    enablePassthrough = enablePassthroughButton.getToggleState();
}

MainComponent::~MainComponent()
{
    inputResampler.release();
    shutdownAudio();
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

    inputResampler = std::make_unique<gin::ResamplingFifo>(resamplerBlockSize, 1, resamplerMaxSamples);

    if (currentBlockSize != samplesPerBlockExpected || currentSampleRate != sampleRate)
        meterSource.resize (1, static_cast<int>(sampleRate * 0.1 / samplesPerBlockExpected));

    if (currentBlockSize != samplesPerBlockExpected)
    {
        currentBlockSize = samplesPerBlockExpected;
        resamplerBuffer = std::make_unique<juce::AudioBuffer<float>>(1, currentBlockSize);
    }

    if (currentSampleRate != sampleRate)
    {
        currentSampleRate = sampleRate;
        inputResampler->setResamplingRatio(currentSampleRate, targetSampleRate);
        inputResampler->reset();

        LibGenisysInitialize(libGenisysInstance, sampleRate);
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
                inputResampler.get()->popAudioBuffer(*resamplerBuffer.get());

                const juce::ScopedLock sl (writerLock);

                if (activeWriter.load() != nullptr)
                {
                    activeWriter.load()->write(resamplerBuffer.get()->getArrayOfReadPointers(), currentBlockSize);
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
    g.fillAll (quitCPanelButton.findColour(HackAudio::midgroundColourId));
}

void MainComponent::resized()
{
    //Tic-tac-toe resizable layout (3x3)
    auto bounds = getBounds();

    int thirdOfWidth = bounds.getWidth()/3;
    int thirdOfWidthPlusOne = thirdOfWidth+1;

    int mainHeight = bounds.getHeight();
    int thirdOfMainHeight = mainHeight / 3;

    int labelHeight = thirdOfMainHeight / 5;

    int margin = 5;

    logoImageComponent.setBounds(thirdOfWidthPlusOne, 0, thirdOfWidth, thirdOfMainHeight);

    recordButton.setBounds(0,
                           0, thirdOfWidth,
                        thirdOfMainHeight - labelHeight);
    recordButtonLabel.setBounds(0,
                             thirdOfMainHeight - labelHeight, thirdOfWidth, labelHeight);

    stopButton.setBounds(0,
                         thirdOfMainHeight, thirdOfWidth,
                           thirdOfMainHeight - labelHeight);
    stopButtonLabel.setBounds(0, (2 * thirdOfMainHeight) - labelHeight, thirdOfWidth, labelHeight);

    playButton.setBounds(0,
                         (2*thirdOfMainHeight), thirdOfWidth,
                           thirdOfMainHeight - labelHeight);
    playButtonLabel.setBounds(0, mainHeight - labelHeight, thirdOfWidth, labelHeight);

    textDisplay.setBounds(thirdOfWidth + (margin * 2), thirdOfMainHeight + margin,
                          thirdOfWidth - (margin * 2), (thirdOfMainHeight * 2) - (margin * 2));

    meter.setBounds(thirdOfWidthPlusOne * 2, 0, thirdOfWidth, mainHeight);

    selector.setBounds(2 * thirdOfWidth, thirdOfMainHeight, thirdOfWidth, mainHeight - 150);

    quitCPanelButton.setBounds(getWidth()-(thirdOfWidth/2),
                               getHeight()-(thirdOfMainHeight/2), thirdOfWidth/2,
                               (thirdOfMainHeight - labelHeight)/2);
    quitCPanelLabel.setBounds(getWidth()-(thirdOfWidth/2),
                              getHeight()-(labelHeight/2), thirdOfWidth/2, labelHeight/2);

    enablePassthroughButton.setBounds(getWidth() - (thirdOfWidth/2),
                                      0, thirdOfWidth/2,
                                      (thirdOfMainHeight - labelHeight)/2);
    enablePassthroughLabel.setBounds(getWidth() - (thirdOfWidth/2),
                                     (thirdOfMainHeight - labelHeight)/2, thirdOfWidth/2, labelHeight/2);
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
    transportSource.start();
    currentlyPlaying = true;
    recordButton.setEnabled(false);
    stopButton.setEnabled(true);
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

void MainComponent::settingsButtonClicked()
{
    logoImageComponent.setToggleState(!logoImageComponent.getToggleState(), juce::dontSendNotification);
    if (logoImageComponent.getToggleState())
    {
        meter.setVisible(false);

        quitCPanelButton.setVisible(true);
        quitCPanelLabel.setVisible(true);
        enablePassthroughButton.setVisible(true);
        enablePassthroughLabel.setVisible(true);
        selector.setVisible(true);
    }
    else
    {
        meter.setVisible(true);

        quitCPanelButton.setVisible(false);
        quitCPanelLabel.setVisible(false);
        enablePassthroughButton.setVisible(false);
        enablePassthroughLabel.setVisible(false);
        selector.setVisible(false);
    }
}
} // namespace GuiApp