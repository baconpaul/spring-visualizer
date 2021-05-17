#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class SpringComponent   : public juce::AudioAppComponent,
        juce::Timer
{
public:
    //==============================================================================
    SpringComponent();
    ~SpringComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    void prepareToPlay(int, double) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &) override;

private:
    void timerCallback() override;

private:
    //==============================================================================
    // Your private member variables go here...

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpringComponent)
};
