/*
     _______         __  __  __             _____    __       __
    |       |.--.--.|__||  ||__|.-----.    |     |_ |  |_ .--|  |    Copyright 2023
    |   -  _||  |  ||  ||  ||  ||  _  |    |       ||   _||  _  |    Quilio Limited
    |_______||_____||__||__||__||_____|    |_______||____||_____|    www.quilio.dev
                                                             
    Quilio Software uses a commercial licence     -     see LICENCE.md for details.
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AutoPitchCorrectionAudioProcessor::AutoPitchCorrectionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

AutoPitchCorrectionAudioProcessor::~AutoPitchCorrectionAudioProcessor()
{
}

//==============================================================================
const juce::String AutoPitchCorrectionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AutoPitchCorrectionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AutoPitchCorrectionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AutoPitchCorrectionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AutoPitchCorrectionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AutoPitchCorrectionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AutoPitchCorrectionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AutoPitchCorrectionAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AutoPitchCorrectionAudioProcessor::getProgramName (int index)
{
    return {};
}

void AutoPitchCorrectionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AutoPitchCorrectionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    pitchShifter.init (sampleRate);
}

void AutoPitchCorrectionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutoPitchCorrectionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AutoPitchCorrectionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    //    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    //    {
    //        auto* channelData = buffer.getWritePointer (channel);
    const float* input = buffer.getReadPointer (0);
    float* output1 = buffer.getWritePointer(0);
    float* output2 = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();
    
    std::vector<const float*> inputs = { reinterpret_cast<const float*>(input)};
    std::vector<float*> outputs = { reinterpret_cast<float*>(output1), reinterpret_cast<float*>(output2)};
    
    pitchShifter.ProcessFloatReplacing (inputs.data(), outputs.data(), numSamples);
}

//==============================================================================
bool AutoPitchCorrectionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AutoPitchCorrectionAudioProcessor::createEditor()
{
    return new AutoPitchCorrectionAudioProcessorEditor (*this);
}

//==============================================================================
void AutoPitchCorrectionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AutoPitchCorrectionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoPitchCorrectionAudioProcessor();
}
