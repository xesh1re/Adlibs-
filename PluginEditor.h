#pragma once

#include "PluginProcessor.h"
#include "Sakura.h"

class AdlibSakuraEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit AdlibSakuraEditor (AdlibSakuraProcessor&);
    ~AdlibSakuraEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;   // pétalos cayendo
    void resized() override;

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void drawBranch (juce::Graphics&, int w, int h);

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        juce::String descripcion;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Petal { float x, y, size, vx, vy, rot, vrot, phase, alpha; };
    void resetPetal (Petal&, bool anywhere);

    AdlibSakuraProcessor& processor;
    sakura::SakuraLookAndFeel laf;
    juce::TooltipWindow tooltip { this, 600 };

    std::vector<std::unique_ptr<Knob>> knobs;
    juce::Label descLabel;
    juce::Rectangle<float> descPanel;

    juce::Image branchImage;
    juce::Path petalPath = sakura::makePetal (1.f);
    std::vector<Petal> petals;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdlibSakuraEditor)
};
