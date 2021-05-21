#pragma once

#include <JuceHeader.h>
#include <vector>
#include <thread>
#include <random>

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
    // juce::Image offscreen, offscreenForCalc;

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
    juce::ColourGradient backgroundShade;

    juce::MidiFile padFile, epFile;
    struct dot
    {
        double x, y, a, vx, vy;
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

    juce::Typeface::Ptr typeFace;

    std::minstd_rand rng{1234};
    std::uniform_real_distribution<float> z01d{0.f, 1.f}, zmp1d{-1.f, 1.f};
    std::uniform_int_distribution<int> uid{0, RAND_MAX};

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringComponent)
};
