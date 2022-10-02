#include "MainWindow.h"

namespace GuiApp
{
constexpr bool isMobile()
{
#if JUCE_IOS || JUCE_ANDROID
    return true;
#else
    return false;
#endif
}

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name, getBackgroundColour(), allButtons)
{
    setName("libGenisysTestBench");
    setUsingNativeTitleBar(true);
    setContentOwned(new MainComponent(), true);

    //TODO: Test mobile interface connecting to RPi Hotspot or similar
    if (isMobile())
        setFullScreen(true);
    else
    {
        getConstrainer()->setFixedAspectRatio(1.66667);
        auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        if (display)
        {
            juce::Rectangle<int> r = display->userArea;
                int x = r.getWidth();
                int y = r.getHeight();

            setResizeLimits (800,
                            480,
                            x,
                            y);
        }
        setResizable(true, true);
        centreWithSize(800, 480);
        //setFullScreen(true);
    }

    setVisible(true);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

juce::Colour MainWindow::getBackgroundColour()
{
    return juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
        ResizableWindow::backgroundColourId);
}

} // namespace GuiApp
