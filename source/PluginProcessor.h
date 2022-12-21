#pragma once

#include <JuceHeader.h>
#include <stdlib.h>

#define ProcessorClass AudioPluginAudioProcessor

#if JUCE_WINDOWS
#else
    #include <unistd.h>
#endif

#include <stdint.h>
#include "comb_filter.h"
#include "allpass_filter.h"
#include "tuning.h"

#define PARAM_DRY_ID "param_dry"
#define PARAM_WET_ID "param_wet"
#define PARAM_ROOM_SIZE_ID "param_room"
#define PARAM_DAMP_ID "param_damp"
#define PARAM_FREEZE_ID "param_freeze"


//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    constexpr static int numberOfInputChannels = 2;
    constexpr static int numberOfOutputChannels = 16;
    
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
public:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    juce::AudioProcessorValueTreeState parameters;
    
public:
    /// \brief AudioPluginAudioProcessor::AudioPluginAudioProcessor The constructor; takes the input port with the flag -p as argument
    /// \param port the input port for OSC messages [int]
    AudioPluginAudioProcessor(int port);
    /// \brief AudioPluginAudioProcessor::mute Mutes all buffers within the allpass_filter and comb_filter instances
    void    mute();
    /// \brief AudioPluginAudioProcessor::setroomsize Sets the roomsize of the diffuse model
    /// \details This function calculates buffer sizes for the allpass_filter and comb_filter instances and passes those new buffer sizes to the allpass_filter::setbuffer and comb_filter::setbuffer functions. Also a new feedback value for the comb_filters is calculated and passed via the comb_filter::setfeedback function.
    /// \param value The desired roomsize value on which the allpass_filter and comb_filter buffer sizes depend as well as the comb_filter feedback value
    void    setroomsize(float value);
    
    /// \brief AudioPluginAudioProcessor::getroomsize Gets the roomsize value
    /// \return the current roomsize value [float]
    float    getroomsize();
    
    /// \brief AudioPluginAudioProcessor::setdamp Sets the dampening factor for the comb_filter instances
    /// \param value the desired dampening value
    void    setdamp(float value);
    
    
    /// \brief AudioPluginAudioProcessor::getdamp Gets the dampening value for the comb_filter instances
    /// \return the current dampening value
    float    getdamp();
    
    /// \brief AudioPluginAudioProcessor::setwet Sets the wet amount in signal output
    /// \param value The desired wet value [float]
    void    setwet(float value);
    
    /// \brief AudioPluginAudioProcessor::getwet Gets the wet amount in signal output
    /// \return The wet value [float]
    float    getwet();
    
    /// \brief AudioPluginAudioProcessor::setdry Sets the dry amount in signal output
    /// \param value The desired dry value [float]
    void    setdry(float value);
    
    /// \brief AudioPluginAudioProcessor::getdry Gets the dry amount in signal output
    /// \return The dry value [float]
    float    getdry();
        
    /// \brief AudioPluginAudioProcessor::setfreezemode Sets the freeze option on and off
    /// \param value The desired state value [bool]
    void    setfreezemode(bool value);
        
    /// \brief AudioPluginAudioProcessor::setfreezemode Gets the freeze mode state
    /// \return The freezemode state value [bool]
    bool    getfreezemode();
    
    /// \brief AudioPluginAudioProcessor::SN3D_normalization Calculates the normalization factors for the ambisonics channels based on the SN3D/ambiX standard
    /// \param channelnum The channelnumber set in the tuning.h file
    void SN3D_normalization(int channelnum);
    
private:
    juce::AudioBuffer<float> inputBuffer;

    comb_filter **comb;
    allpass_filter **allpass;
    
    float    max_comb_buffactor;
    float    max_allpass_buffactor;
    int      **comb_buffer_size;
    int      **allpass_buffer_size;
    float    gain;
    float    feedback;
    float    comb_buffactor;
    float    allpass_buffactor;
    float    damp;
    float    wet_factor;
    float    order_factor;
    float    dry;
    float    width;
    bool     freezemode;
    float    damp_comb;
    float    feedback_filters;
    float    newroom;
    float    oldroom;
    int      numInputChannels;
    int      numOutputChannels;
    float    *ACN_normalization;
    float    sum_ACN_normalization;

    std::array<std::string, 5> parameterIDs = {PARAM_DRY_ID, PARAM_WET_ID, PARAM_ROOM_SIZE_ID, PARAM_DAMP_ID, PARAM_FREEZE_ID};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
