#include "SpringComponent.h"

//==============================================================================
SpringComponent::SpringComponent()
{
    setSize (600, 400);
}

//==============================================================================
void SpringComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::Font (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void SpringComponent::resized()
{
    // This is called when the SpringComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
