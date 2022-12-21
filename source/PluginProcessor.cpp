#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::discreteChannels(64), true)
                     #endif
                       ),
       parameters(*this, nullptr, "reverb_farevale", {
               std::make_unique<juce::AudioParameterFloat> (PARAM_DRY_ID, "Dry", juce::NormalisableRange<float> (0.0f, 1.0f), initialdry),
               std::make_unique<juce::AudioParameterFloat> (PARAM_WET_ID, "Wet", juce::NormalisableRange<float> (0.0f, 1.0f), initialwet),
               std::make_unique<juce::AudioParameterFloat> (PARAM_ROOM_SIZE_ID, "Room Size", juce::NormalisableRange<float> (0.0f, 1.0f), initialroom),
               std::make_unique<juce::AudioParameterFloat> (PARAM_DAMP_ID, "Dampening", juce::NormalisableRange<float> (0.0f, 1.0f), initialdamp),
               std::make_unique<juce::AudioParameterBool>  (PARAM_FREEZE_ID, "Freeze", initialfreeze)
       })
{
    for (auto & parameterID : parameterIDs) parameters.addParameterListener(parameterID, this);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    for (auto & parameterID : parameterIDs) {
        if (parameterID != PARAM_ROOM_SIZE_ID) parameters.removeParameterListener(parameterID, this);
    }
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    
    numInputChannels = getTotalNumInputChannels();
    inputBuffer.setSize(1, samplesPerBlock);
    
    numOutputChannels = getTotalNumOutputChannels();
    
    ACN_normalization = new float [numOutputChannels];
    
    comb= new comb_filter * [numOutputChannels-1];
    comb_buffer_size = new int * [numOutputChannels-1];
    allpass = new allpass_filter * [numOutputChannels-1];
    allpass_buffer_size = new int * [numOutputChannels-1];
    for (int i = 0; i < numOutputChannels-1; i++){
        comb[i] = new comb_filter [numcombs];
        comb_buffer_size[i] = new int [numcombs];
        allpass[i] = new allpass_filter [numallpasses];
        allpass_buffer_size[i] = new int [numallpasses];
        for (int j = 0; j < numcombs; j++){
            max_comb_buffactor = (1 + (scale_comb_buffer)-(scale_comb_buffer/2));
            comb_buffer_size[i][j] = 0;
            comb_buffer_size[i][j] = (int) ((i*spreadvalue) + (comb_buffer_tuning[j]*max_comb_buffactor));
            comb[i][j].initBuffer(comb_buffer_size[i][j]);
//            std::cout << comb[i][j].ready() << std::endl;
        }
        for (int j = 0; j < numallpasses; j++){
            max_allpass_buffactor = (1 + (scale_allpass_buffer)-(scale_allpass_buffer/2));
            allpass_buffer_size[i][j] = 0;
            allpass_buffer_size[i][j] = (int) ((i*spreadvalue) + (allpass_buffer_tuning[j]*max_allpass_buffactor));
            allpass[i][j].initBuffer(allpass_buffer_size[i][j]);
        }
    }
    
    setwet(initialwet);
    setdry(initialdry);
    setroomsize(initialroom);
    setdamp(initialdamp);
    setfreezemode(initialfreeze);
    SN3D_normalization(numOutputChannels);
    
    std::cout << "number Output Channels: " << numOutputChannels << std::endl;
    std::cout << "number Input Channels: " << numInputChannels << std::endl;
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return true;
//  #if JucePlugin_IsMidiEffect
//    juce::ignoreUnused (layouts);
//    return true;
//  #else
//    // This is the place where you check if the layout is supported.
//    // In this template code we only support mono or stereo.
//    // Some plugin hosts, such as certain GarageBand versions, will only
//    // load plugins that support stereo bus layouts.
//    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
//     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
//        return false;
//
//    // This checks if the input layout matches the output layout
//   #if ! JucePlugin_IsSynth
//    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
//        return false;
//   #endif
//
//    return true;
//  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    bool ready = true;
    
    inputBuffer.clear();
    
    for(int channel=0; channel<numInputChannels; channel++) {
        inputBuffer.addFrom(0, 0, buffer, channel, 0, buffer.getNumSamples());
    }
    
    auto readinPointer = inputBuffer.getReadPointer(0);
    auto readoutPointerACN0 = buffer.getReadPointer(0);
    auto writePointerACN0 = buffer.getWritePointer(0);
    
    buffer.clear();
    
    inputBuffer.applyGain(1.f/(float) numInputChannels);
    
    for(int channel=1; channel<numOutputChannels; channel++)
    {
        auto* readoutPointer = buffer.getReadPointer(channel);
        auto* writePointer = buffer.getWritePointer(channel);

        for(int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {

            writePointer[sample] = 0;

            for(int j = 0; j<numcombs; j++){
                writePointer[sample] += comb[channel-1][j].process(gain*readinPointer[sample]);
            }
            for(int j = 0; j<numallpasses; j++){
                writePointer[sample] = allpass[channel-1][j].process(readoutPointer[sample]);
            }
            writePointer[sample] = (wet_factor * readoutPointer[sample] + dry * readinPointer[sample]) * ACN_normalization[channel];
            writePointerACN0[sample] += readoutPointer[sample];
        }
    }

    for(int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        writePointerACN0[sample] = readoutPointerACN0[sample] / (numOutputChannels-1);
    }
    for (int i = 0; i < numOutputChannels-1; i++){
        for (int j = 0; j < numcombs; j++){
            if (not comb[i][j].ready()) ready = false;
        }
        for (int j = 0; j < numallpasses; j++){
            if (not allpass[i][j].ready()) ready = false;
        }
    }
    if (newroom != oldroom && ready == true) setroomsize(newroom);
    else if (newroom != oldroom) std::cout << "Not ready!" << std::endl;
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
//    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

void AudioPluginAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue) {
    if (parameterID == PARAM_DRY_ID) {
        setdry(newValue);
    } else if (parameterID == PARAM_WET_ID) {
        setwet(newValue);
    } else if (parameterID == PARAM_ROOM_SIZE_ID) {
        newroom = newValue;
    } else if (parameterID == PARAM_DAMP_ID) {
        setdamp(newValue);
    } else if (parameterID == PARAM_FREEZE_ID) {
        setfreezemode((bool) newValue);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

void AudioPluginAudioProcessor::mute()
{
    for (int i = 0; i < numOutputChannels-1; i++){
        for (int j = 0; j < numcombs; j++){
            comb[i][j].mute();
        }
    }
    
    for (int i = 0; i < numOutputChannels-1; i++){
        for (int j = 0; j < numallpasses; j++){
            allpass[i][j].mute();
        }
    }
}

void AudioPluginAudioProcessor::setroomsize(float value)
{
    feedback = (value*scalefeedback) + offsetfeedback;
    comb_buffactor = 1 + (value*scale_comb_buffer)-(scale_comb_buffer/2);
    allpass_buffactor = 1 + (value*scale_allpass_buffer)-(scale_allpass_buffer/2);
    
    for (int i = 0; i < numOutputChannels-1; i++){
        for (int j = 0; j < numcombs; j++){
            comb_buffer_size[i][j] = ((int) ((i*spreadvalue) + comb_buffer_tuning[j]*comb_buffactor));
            comb[i][j].setbuffer(comb_buffer_size[i][j]);
            comb[i][j].setfeedback(feedback);
        }
        for (int j = 0; j < numallpasses; j++){
            allpass_buffer_size[i][j] = ((int) ((i*spreadvalue) + allpass_buffer_tuning[j]*allpass_buffactor));
            allpass[i][j].setbuffer(allpass_buffer_size[i][j]);
            allpass[i][j].setfeedback(feedback);
        }
    }
    oldroom = value;
}

float AudioPluginAudioProcessor::getroomsize()
{
    return (feedback-offsetfeedback)/scalefeedback;
}

void AudioPluginAudioProcessor::setdamp(float value)
{
    if (value < 0.95f && value > 0.05f) {
        damp = value;
        for (int i = 0; i < numOutputChannels-1; i++){
            for (int j = 0; j < numcombs; j++){
                comb[i][j].setdamp(damp);
            }
        }
    }
}

float AudioPluginAudioProcessor::getdamp()
{
    return damp;
}

void AudioPluginAudioProcessor::setwet(float value)
{
    wet_factor = value/numcombs;
}

float AudioPluginAudioProcessor::getwet()
{
    return wet_factor*numcombs;
}

void AudioPluginAudioProcessor::setdry(float value)
{
    dry = value;
}

float AudioPluginAudioProcessor::getdry()
{
    return dry;
}

void AudioPluginAudioProcessor::setfreezemode(bool state){
    freezemode = state;

    // Recalculate internal values after parameter change
    if (freezemode){
        feedback_filters = 1;
        damp_comb = 0;
        gain = 0;
    }
    else {
        feedback_filters = feedback;
        damp_comb = damp;
        gain = initialgain;
        mute();
    }

    for (int i = 0; i < numOutputChannels-1; i++){
        for (int j = 0; j < numcombs; j++){
            comb[i][j].setfeedback(feedback_filters);
            comb[i][j].setdamp(damp_comb);
        }
        for (int j = 0; j < numallpasses; j++){
            allpass[i][j].setfeedback(feedback_filters);
        }
    }
}

bool AudioPluginAudioProcessor::getfreezemode()
{
    return freezemode;
}

void AudioPluginAudioProcessor::SN3D_normalization(int channelnum){
    for (int i = 0; i < numOutputChannels; i++){
        if(i <= 3 || i == 6 || i == 12){
            ACN_normalization[i] = 1.f;
        }
        if(i == 4 || i == 8){
            ACN_normalization[i] = std::sqrtf(2/(4*24*M_PI))/std::sqrtf(1/(4*M_PI));
        }
        if(i == 5 || i == 7){
            ACN_normalization[i] = std::sqrtf(2/(4*6*M_PI))/std::sqrtf(1/(4*M_PI));
        }
        if(i == 9 || i == 15){
            ACN_normalization[i] = std::sqrtf(2/(4*720*M_PI))/std::sqrtf(1/(4*M_PI));
        }
        if(i == 10 || i == 14){
            ACN_normalization[i] = std::sqrtf(2/(4*120*M_PI))/std::sqrtf(1/(4*M_PI));
        }
        if(i == 11 || i == 13){
            ACN_normalization[i] = std::sqrtf(2/(4*12*M_PI))/std::sqrtf(1/(4*M_PI));
        }
        std::cout << "ACN " << i << " norm factor: " << ACN_normalization[i] << std::endl;
    }
    std::cout << "normalization according to SN3D/ambiX standard" << std::endl;
}
