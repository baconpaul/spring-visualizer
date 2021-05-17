#include "SpringComponent.h"

//==============================================================================
SpringComponent::SpringComponent() {
    setSize(600, 400);
    setAudioChannels(0,2);

    formatManager.registerBasicFormats();

    auto dir = std::string("/Users/paul/Desktop/Ambient Video Project/");
    auto file = juce::File(dir + "Audio.wav");                                        // [9]
    auto *reader = formatManager.createReaderFor(file);                    // [10]

    if (reader != nullptr) {
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader,true);
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource = std::move(newSource);
        transportSource.setPosition(26.0);
        priorTime = 26.0;
        transportSource.start();
    }
    startTimerHz(30);

    auto epf = juce::File(dir + "EP.mid");
    auto fis = std::make_unique<juce::FileInputStream>(epf);
    epFile.readFrom(*fis);
    epFile.convertTimestampTicksToSeconds();
}

SpringComponent::~SpringComponent() {
    shutdownAudio();
}

//==============================================================================
void SpringComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::Font (16.0f));
    g.setColour (juce::Colours::white);
    auto cp = transportSource.getCurrentPosition();
    auto p = std::to_string(cp);
    g.drawText (p, getLocalBounds(), juce::Justification::centred, true);

    auto tk = epFile.getTrack(0);
    auto it = tk->getNextIndexAtTime(priorTime);

    if (tk->getEventTime(it) <= transportSource.getCurrentPosition())
    {
        std::cout << "Event " << tk->getEventTime(it) << std::endl;
    }

    priorTime = transportSource.getCurrentPosition();
}

void SpringComponent::resized()
{
    // This is called when the SpringComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void SpringComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
}
void SpringComponent::releaseResources()
{
    transportSource.releaseResources();
}
void SpringComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &b) {
    if (readerSource.get() == nullptr)
    {
        b.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock (b);
}

void SpringComponent::timerCallback() {
    repaint();
}
