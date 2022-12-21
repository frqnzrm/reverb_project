#include "PluginProcessor.h"
#include "PluginEditor.h"

SlidersComponent::SlidersComponent()
{
    wetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    wetSlider.setSliderStyle(juce::Slider::Rotary);
    addAndMakeVisible(wetSlider);
    
    drySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    drySlider.setSliderStyle(juce::Slider::Rotary);
    addAndMakeVisible(drySlider);
    
    dampeningSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    dampeningSlider.setSliderStyle(juce::Slider::Rotary);
    addAndMakeVisible(dampeningSlider);
    
    roomsizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    roomsizeSlider.setSliderStyle(juce::Slider::Rotary);
    addAndMakeVisible(roomsizeSlider);
    
    addAndMakeVisible(wetLabel);
    wetLabel.setText("wet", juce::dontSendNotification);
    wetLabel.attachToComponent(&wetSlider, false);
    
    addAndMakeVisible(dryLabel);
    dryLabel.setText("dry", juce::dontSendNotification);
    dryLabel.attachToComponent(&drySlider, false);
    
    addAndMakeVisible(dampeningLabel);
    dampeningLabel.setText("dampening", juce::dontSendNotification);
    dampeningLabel.attachToComponent(&dampeningSlider, false);
    
    addAndMakeVisible(roomsizeLabel);
    roomsizeLabel.setText("roomsize", juce::dontSendNotification);
    roomsizeLabel.attachToComponent(&roomsizeSlider, false);
}

void SlidersComponent::resized()
{
    auto area = getLocalBounds();
    DBG(getLocalBounds().toString());
    area.removeFromTop(getHeight()/6);
    
    auto sliderWidth = getWidth()/4;
    
    wetSlider.setBounds(area.removeFromLeft(sliderWidth));
    drySlider.setBounds(area.removeFromLeft(sliderWidth));
    dampeningSlider.setBounds(area.removeFromLeft(sliderWidth));
    roomsizeSlider.setBounds(area.removeFromLeft(sliderWidth));
}

MainContentComponent::MainContentComponent()
{
    addAndMakeVisible (sliders);
}

MainContentComponent::~MainContentComponent()
{
    setLookAndFeel(nullptr);
}

void MainContentComponent::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainContentComponent::resized()
{
    auto area = getLocalBounds();
    
    sliders.setBounds (area.removeFromTop(getHeight()/2));
}


//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel(& LookAndFeel_frqz_rm);
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    addAndMakeVisible (main);
    setSize (400, 300);
    
    juce::AudioDeviceManager adm;
    const juce::OwnedArray<juce::AudioIODeviceType>& types = adm.getAvailableDeviceTypes();

    for (int i=0; i < types.size(); i++) {
        std::cout << types[i]->getTypeName() << "\n";
    }

    using Attachment     = juce::SliderParameterAttachment;
    
    auto& parameters = processorRef.parameters;
    
    wetSliderAttachement = std::make_unique<Attachment>(*parameters.getParameter(PARAM_WET_ID), main.sliders.wetSlider);
    drySliderAttachement = std::make_unique<Attachment>(*parameters.getParameter(PARAM_DRY_ID), main.sliders.drySlider);
    dampeningSliderAttachement = std::make_unique<Attachment>(*parameters.getParameter(PARAM_DAMP_ID), main.sliders.dampeningSlider);
    roomsizeSliderAttachement = std::make_unique<Attachment>(*parameters.getParameter(PARAM_ROOM_SIZE_ID), main.sliders.roomsizeSlider);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto edge = 20;
    main.setBounds(edge, edge, getWidth()-2*edge, getHeight()-2*edge);
}
