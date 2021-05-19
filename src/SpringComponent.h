#pragma once

#include <JuceHeader.h>
#include <vector>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class SpringComponent : public juce::AudioAppComponent, public juce::Timer, public juce::KeyListener
{
  public:
    //==============================================================================
    SpringComponent();
    ~SpringComponent() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void paintForReal(juce::Graphics &);
    juce::Image offscreen;

    void resized() override;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    double priorTime;

    void prepareToPlay(int, double) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &) override;

    bool keyPressed(const KeyPress &key, Component *originatingComponent) override;
    void mouseUp(const MouseEvent &event) override;

  public:
    juce::MidiFile padFile, epFile;
    struct dot
    {
        double x, y, a;
        bool square;
    };
    std::vector<dot> dots;

    void timerCallback() override;

    void pushNextSampleIntoFifo(float sample) noexcept
    {
        // if the fifo contains enough data, set a flag to say
        // that the next line should now be rendered..
        if (fifoIndex == fftSize)
        {
            if (!nextFFTBlockReady)
            {
                zeromem(fftData, sizeof(fftData));
                memcpy(fftData, fifo, sizeof(fifo));
                nextFFTBlockReady = true;
            }

            fifoIndex = 0;
        }

        fifo[fifoIndex++] = sample;
    }
    enum
    {
        fftOrder = 11,
        fftSize = 1 << fftOrder
    };
    juce::dsp::FFT forwardFFT;

    static constexpr int meshSize = 48;
    std::array<std::array<float, meshSize>, meshSize> mesh, meshPrior, lx, ly, ly2;

    float fifo[fftSize];
    float fftData[2 * fftSize];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    float fftOut[fftSize];

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringComponent)
};
