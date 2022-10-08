#include "MainComponent.h"
#include "BinaryHelper.h"
namespace GuiApp
{
//==============================================================================
MainComponent::MainComponent() : keyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
                                 startTime (juce::Time::getMillisecondCounterHiRes() * 0.001)
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
    setSize (800, 800);

    setOpaque(true);

    addAndMakeVisible(selector);

    monitorButton.setShape(monitorPath, true, false, true);
    monitorButton.onClick = [this] { monitorButtonClicked(); };
#if JUCE_DEBUG
    addAndMakeVisible(monitorButton);
#endif

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

    //textDisplay.setText("The quick brown fox jumps over the lazy dog", juce::dontSendNotification);
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


    addAndMakeVisible (midiInputListLabel);
    midiInputListLabel.setText ("MIDI Input:", juce::dontSendNotification);
    midiInputListLabel.attachToComponent (&midiInputList, true);

    addAndMakeVisible (midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable ("No MIDI Inputs Enabled");
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    juce::StringArray midiInputNames;

    for (auto input : midiInputs)
        midiInputNames.add (input.name);

    midiInputList.addItemList (midiInputNames, 1);
    midiInputList.onChange = [this] { setMidiInput (midiInputList.getSelectedItemIndex()); };

    // find the first enabled device and use that by default
    for (auto input : midiInputs)
    {
        if (deviceManager.isMidiInputDeviceEnabled (input.identifier))
        {
            setMidiInput (midiInputs.indexOf (input));
            break;
        }
    }

    // if no enabled devices were found just use the first one in the list
    if (midiInputList.getSelectedId() == 0)
        setMidiInput (0);

    addAndMakeVisible (keyboardComponent);
    keyboardState.addListener (this);

    addAndMakeVisible (midiMessagesBox);
    midiMessagesBox.setMultiLine (true);
    midiMessagesBox.setReturnKeyStartsNewLine (true);
    midiMessagesBox.setReadOnly (true);
    midiMessagesBox.setScrollbarsShown (true);
    midiMessagesBox.setCaretVisible (false);
    midiMessagesBox.setPopupMenuEnabled (true);
    midiMessagesBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x32ffffff));
    midiMessagesBox.setColour (juce::TextEditor::outlineColourId, juce::Colour (0x1c000000));
    midiMessagesBox.setColour (juce::TextEditor::shadowColourId, juce::Colour (0x16000000));

    auto parent = juce::File::getCurrentWorkingDirectory();
    testFile = parent.getChildFile("scripts").getChildFile("OpenProTools.wav");
    loadTestFile(testFile);

    setAudioChannels(2,2);

    trySetReSpeakerAsDevice();
    requestMicrophoneAccess();
}

void MainComponent::requestMicrophoneAccess()
{
    if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::Component::SafePointer<MainComponent> safeThis(this);

        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                                          [safeThis](bool granted) mutable {});
        return;
    }
}

void MainComponent::trySetReSpeakerAsDevice()
{
    for (auto device : deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(true))
        if (device.equalsIgnoreCase(respeakerUSBDevice))
        {
            auto setup = deviceManager.getAudioDeviceSetup();
            setup.inputDeviceName = respeakerUSBDevice;
            setup.outputDeviceName = respeakerUSBDevice;
            deviceManager.setAudioDeviceSetup(setup, true);
            break;
        }
}

void MainComponent::loadTestFile(juce::File testFile)
{
    juce::AudioFormatReader* reader = formatManager.createReaderFor(testFile);
    if (reader)
    {
        loadedAudioBuffer = juce::AudioBuffer<float>(1, reader->lengthInSamples);
        reader->read(&loadedAudioBuffer, 0, reader->lengthInSamples,
                     0, true, true);

        newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);   // [11]
        transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
        transportLoaded = true;
    }
}

void MainComponent::processAudioFile(juce::File file, bool deleteAfterRender)
{
    auto result = LibGenisysProcessNativePath(libGenisysInstance, file.getFullPathName().toStdString());
    if (!result.empty())
        textDisplay.setText(juce::String(result), juce::dontSendNotification);

    //Keep user's disk tidy unless we are purposely recording files to train the model
    //TODO: Develop system for batch recording and submitting audio files
    if (deleteAfterRender)
        file.deleteFile();

#ifdef __APPLE__
    if (result.find("genesis") != std::string::npos)
    {
        if (result.find("pro") != std::string::npos && result.find("tools") != std::string::npos)
        {
            if (result.find("open") != std::string::npos)
            {
                OpenProToolsMac();
            }
            else if (result.find("close") != std::string::npos || result.find("quit") != std::string::npos)
            {
                CloseProToolsMac();
            }
        }
        else if (result.find("live") != std::string::npos ||
                 result.find("life") != std::string::npos)
        {
            if (result.find("open") != std::string::npos)
            {
                OpenAbletonMac();
            }
            else if (result.find("close") != std::string::npos || result.find("quit") != std::string::npos)
            {
                CloseAbletonMac();
            }
        }
        else if (result.find("logic") != std::string::npos)
        {
            if (result.find("open") != std::string::npos)
            {
                OpenLogicMac();
            }
            else if (result.find("close") != std::string::npos || result.find("quit") != std::string::npos)
            {
                CloseLogicMac();
            }
        }
    }
#endif
}

void MainComponent::monitorButtonClicked()
{
    monitorButton.setToggleState(!monitorButton.getToggleState(), juce::dontSendNotification);
    enablePassthrough = monitorButton.getToggleState();
}

MainComponent::~MainComponent()
{
    keyboardState.removeListener (this);
    deviceManager.removeMidiInputDeviceCallback (juce::MidiInput::getAvailableDevices()[midiInputList.getSelectedItemIndex()].identifier, this);

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

        if (currentlyRecordingCommandSample)
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

            if (!enablePassthrough)
                bufferToFill.clearActiveBufferRegion();
        }
        else if (currentlyPlayingCommandSample)
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

                            processAudioFile(testFile, false);

                            auto parent = juce::File::getCurrentWorkingDirectory();
                            if (testFile.getFileName() == "OpenProTools.wav")
                                testFile = parent.getChildFile("scripts").getChildFile("CloseProTools.wav");
                            else if (testFile.getFileName() == "CloseProTools.wav")
                                testFile = parent.getChildFile("scripts").getChildFile("OpenProTools.wav");
                            loadTestFile(testFile);
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
    auto margin = 8;
    recordButton.setBounds(125,0, 100,100);
    stopButton.setBounds(275,0, 100,100);
    playButton.setBounds(425,0, 100,100);
    monitorButton.setBounds(665, 0, 100, 100);

    selector.setBounds(100, 125, getWidth()-100, getHeight()-520);

    meter.setBounds(3, 0, 100, getHeight()-420);

    textDisplay.setBounds(margin, getHeight()-420,getWidth() - (margin*2), 100);

    auto area = getLocalBounds();
    area.removeFromTop(getHeight()-320);

    midiMessagesBox  .setBounds (area.removeFromTop(204).reduced (8));
    midiInputList    .setBounds (area.removeFromTop (36).removeFromRight (getWidth() - 150).reduced (8));
    keyboardComponent.setBounds (area.removeFromTop (80).reduced(8));

}

void MainComponent::recordButtonClicked()
{
    recordButton.setEnabled(false);
    playButton.setEnabled(false);
    stopButton.setEnabled(true);

    startRecording ();
    currentlyRecordingCommandSample = true;
}

void MainComponent::stopButtonClicked()
{
    if (currentlyRecordingCommandSample)
    {
        stopRecordingAndConvert();
    }
    if (currentlyPlayingCommandSample)
    {
        transportSource.stop();
        currentlyPlayingCommandSample = false;
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
        currentlyPlayingCommandSample = true;
        recordButton.setEnabled(false);
        stopButton.setEnabled(true);
    }
}

void MainComponent::startRecording()
{
    stop();

    lastRecording = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
                               .getChildFile("temp.wav");

    if (currentSampleRate > 0)
    {
        if (auto fileStream = std::unique_ptr<juce::FileOutputStream> (lastRecording.createOutputStream()))
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
    stop();
    currentlyRecordingCommandSample = false;
    processAudioFile(lastRecording, true);
}

juce::String MainComponent::getMidiMessageDescription (const juce::MidiMessage& m)
{
    if (m.isNoteOn())           return "Note on "          + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
    if (m.isNoteOff())          return "Note off "         + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
    if (m.isProgramChange())    return "Program change "   + juce::String (m.getProgramChangeNumber());
    if (m.isPitchWheel())       return "Pitch wheel "      + juce::String (m.getPitchWheelValue());
    if (m.isAftertouch())       return "After touch "      + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3) +  ": " + juce::String (m.getAfterTouchValue());
    if (m.isChannelPressure())  return "Channel pressure " + juce::String (m.getChannelPressureValue());
    if (m.isAllNotesOff())      return "All notes off";
    if (m.isAllSoundOff())      return "All sound off";
    if (m.isMetaEvent())        return "Meta event";

    if (m.isController())
    {
        juce::String name (juce::MidiMessage::getControllerName (m.getControllerNumber()));

        if (name.isEmpty())
            name = "[" + juce::String (m.getControllerNumber()) + "]";

        return "Controller " + name + ": " + juce::String (m.getControllerValue());
    }

    return juce::String::toHexString (m.getRawData(), m.getRawDataSize());
}

void MainComponent::logMessage (const juce::String& m)
{
    midiMessagesBox.moveCaretToEnd();
    midiMessagesBox.insertTextAtCaret (m + juce::newLine);
}

/** Starts listening to a MIDI input device, enabling it if necessary. */
void MainComponent::setMidiInput (int index)
{
    auto list = juce::MidiInput::getAvailableDevices();

    deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier, this);

    auto newInput = list[index];

    if (! deviceManager.isMidiInputDeviceEnabled (newInput.identifier))
        deviceManager.setMidiInputDeviceEnabled (newInput.identifier, true);

    deviceManager.addMidiInputDeviceCallback (newInput.identifier, this);
    midiInputList.setSelectedId (index + 1, juce::dontSendNotification);

    lastInputIndex = index;
}

// These methods handle callbacks from the midi device + on-screen keyboard..
void MainComponent::handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message)
{
    const juce::ScopedValueSetter<bool> scopedInputFlag (isAddingFromMidiInput, true);
    keyboardState.processNextMidiEvent (message);
    postMessageToList (message, source->getName());
}

void MainComponent::handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    if (! isAddingFromMidiInput)
    {
        auto m = juce::MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity);
        m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
        postMessageToList (m, "On-Screen Keyboard");
    }

    //C7, default Record/Arm button on Novation LaunchControl XL ||
    //default Stop/Solo/Mute button when in Custom Drum Mode on Novation Launchpad Mini
    if (midiNoteNumber == 108 || midiNoteNumber == 71) //C7, default Record/Arm button on Novation LaunchControl XL
    {
        juce::MessageManager::callAsync(
        [=] ()
        {
            recordButtonClicked();
        }
        );
    }
}

void MainComponent::handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/)
{
    if (! isAddingFromMidiInput)
    {
        auto m = juce::MidiMessage::noteOff (midiChannel, midiNoteNumber);
        m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
        postMessageToList (m, "On-Screen Keyboard");
    }

    //C7, default Record/Arm button on Novation LaunchControl XL ||
    //default Stop/Solo/Mute button when in Custom Drum Mode on Novation Launchpad Mini
    if (midiNoteNumber == 108 || midiNoteNumber == 71)
    {
        juce::MessageManager::callAsync(
        [=] ()
        {
            stopButtonClicked();
        }
        );
    }
}

void MainComponent::postMessageToList (const juce::MidiMessage& message, const juce::String& source)
{
    (new IncomingMessageCallback (this, message, source))->post();
}

void MainComponent::addMessageToList (const juce::MidiMessage& message, const juce::String& source)
{
    auto time = message.getTimeStamp() - startTime;

    auto hours   = ((int) (time / 3600.0)) % 24;
    auto minutes = ((int) (time / 60.0)) % 60;
    auto seconds = ((int) time) % 60;
    auto millis  = ((int) (time * 1000.0)) % 1000;

    auto timecode = juce::String::formatted ("%02d:%02d:%02d.%03d",
                                             hours,
                                             minutes,
                                             seconds,
                                             millis);

    auto description = getMidiMessageDescription (message);

    if (description == "fc")
    {
        //Stop Message. We'll use this to quit both this and the headless application, which won't have a GUI
        if (juce::JUCEApplicationBase::isStandaloneApp())
            juce::JUCEApplicationBase::quit();
    }

    juce::String midiMessageString (timecode + "  -  " + description + " (" + source + ")"); // [7]
    logMessage (midiMessageString);
}

void MainComponent::OpenAbletonMac()
{
    juce::File ableton11SuiteApp = juce::File("/Applications/Ableton Live 11 Suite.app");
    juce::File ableton11StandardApp = juce::File("/Applications/Ableton Live 11 Standard.app");
    juce::File ableton11IntroApp = juce::File("/Applications/Ableton Live 11 Intro.app");

    if (ableton11SuiteApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Ableton Live 11 Suite\" to activate'";
        system(cmd);
    }
    else if (ableton11StandardApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Ableton Live 11 Standard\" to activate'";
        system(cmd);
    }
    else if (ableton11IntroApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Ableton Live 11 Intro\" to activate'";
        system(cmd);
    }
}

void MainComponent::CloseAbletonMac()
{
    juce::File ableton11SuiteApp = juce::File("/Applications/Ableton Live 11 Suite.app");
    juce::File ableton11StandardApp = juce::File("/Applications/Ableton Live 11 Standard.app");
    juce::File ableton11IntroApp = juce::File("/Applications/Ableton Live 11 Intro.app");

    if (ableton11SuiteApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Ableton Live 11 Suite\" to quit'";
        system(cmd);
    }
    else if (ableton11StandardApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Ableton Live 11 Standard\" to quit'";
        system(cmd);
    }
    else if (ableton11IntroApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Ableton Live 11 Intro\" to quit'";
        system(cmd);
    }
}

void MainComponent::OpenLogicMac()
{
    juce::File logicApp = juce::File("/Applications/Logic Pro X.app");
    if (logicApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Logic Pro X\" to activate'";
        system(cmd);
    }
}

void MainComponent::CloseLogicMac()
{
    juce::File logicApp = juce::File("/Applications/Logic Pro X.app");
    if (logicApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Logic Pro X\" to quit'";
        system(cmd);
    }
}

void MainComponent::OpenProToolsMac()
{
    juce::File proToolsApp = juce::File("/Applications/Pro Tools.app");
    if (proToolsApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Pro Tools\" to activate'";
        system(cmd);
    }
}

void MainComponent::CloseProToolsMac()
{
    juce::File proToolsApp = juce::File("/Applications/Pro Tools.app");
    if (proToolsApp.exists())
    {
        const char *cmd = "osascript -e 'tell application \"Pro Tools\" to quit'";
        system(cmd);
    }
}

} // namespace GuiApp