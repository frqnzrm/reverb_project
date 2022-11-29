#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel_frqz_rm.h"

class SlidersComponent       : public juce::Component
{
public:
    SlidersComponent();
    
    void resized() override;

    juce::Slider wetSlider;
    juce::Slider drySlider;
    juce::Slider dampeningSlider;
    juce::Slider roomsizeSlider;
    
private:
    juce::Label wetLabel;
    juce::Label dryLabel;
    juce::Label dampeningLabel;
    juce::Label roomsizeLabel;
};


class MainContentComponent   : public juce::Component
{
public:
    MainContentComponent();
    ~MainContentComponent() override;

    void paint (juce::Graphics&) override;

    void resized() override;
    
    SlidersComponent sliders;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    std::unique_ptr<juce::SliderParameterAttachment> wetSliderAttachement;
    std::unique_ptr<juce::SliderParameterAttachment> drySliderAttachement;
    std::unique_ptr<juce::SliderParameterAttachment> dampeningSliderAttachement;
    std::unique_ptr<juce::SliderParameterAttachment> roomsizeSliderAttachement;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;
    MainContentComponent main;
    LaF LookAndFeel_frqz_rm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
