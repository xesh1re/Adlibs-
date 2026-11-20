#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// Adlib Sakura - cadena de efectos pensada para adlibs:
//   Entrada -> Distorsión -> Tono -> Lo-Fi -> Doble (chorus) -> Eco -> Reverb -> Salida
//==============================================================================
class AdlibSakuraProcessor : public juce::AudioProcessor
{
public:
    AdlibSakuraProcessor();
    ~AdlibSakuraProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Adlib Sakura"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    double sr = 44100.0;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> echo;
    juce::Reverb reverb;

    float toneState[2]   { 0.f, 0.f };
    float echoFbState[2] { 0.f, 0.f };

    juce::SmoothedValue<float> sIn, sDrive, sEchoTime, sEchoMix, sOut;

    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdlibSakuraProcessor)
};
