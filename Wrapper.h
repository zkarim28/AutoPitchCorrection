#pragma once
/*
     _______         __  __  __             _____    __       __
    |       |.--.--.|__||  ||__|.-----.    |     |_ |  |_ .--|  |    Copyright 2023
    |   -  _||  |  ||  ||  ||  ||  _  |    |       ||   _||  _  |    Quilio Limited
    |_______||_____||__||__||__||_____|    |_______||____||_____|    www.quilio.dev
                                                             
    Quilio Software uses a commercial licence     -     see LICENCE.md for details.
*/

//Au2Tune Effect

/*
   This pitch shifter algorithm uses FFT-based pitch detection and correction combined with an overlap-add method for pitch shifting. The process involves computing the autocorrelation of the input signal using FFT to estimate the pitch period and confidence. Pitch shifting is achieved through phase vocoder techniques, modifying phase increments, and using circular buffers to handle input and output fragments. Hann windowing is applied to smooth the signal and reduce spectral leakage. The algorithm includes pitch correction towards target values and smoothing mechanisms to ensure seamless transitions, providing an effective auto-tune effect for vocals.
*/

class Au2TuneEffect : public AudioEffect
{
public:
    
    AutoTuneEffect()
    {
        //This is our scaling factor of how much total effect is applied here from 0 -> 1
        registerParameter("Mix", 0.0f, 0.0f, 1.0f, 0.01f, "% Percent");
        
        // Easy This transposes from -12 to +12 semitones, step size of 1
        registerParameter("Shift", 0.0f, -12.0f, 12.0f, 1.0f, "Semitones");
        
        // This is -100 -> +100 cents transposed, parameter in code is from -1 to +1 step size should be 0.01
        registerParameter("Tune", 0.0f, -1.0f, 1.0f, 0.01f, "Cents");
        
        //Amount of pitch slide how much you want it? from 0 -> 1
        registerParameter("Amount", 0.0f, 0.0f, 1.0f, 0.01f, "% Percent");
        
        //control how much pitch slide from one note to the next 0 -> 1000ms step size 0.01. Param itself is 0 -> 1 step size 0.00001.
        registerParameter("Glide", 0.0f, 0.0f, 1.0f, 0.00001f, "Cents");
        
        //TODO: implement setKey
        
        //TODO: implement setScale
    }
    
    void prepare(const juce::dsp::ProcessSpec& spec) override{
        pitchShifter.init(spec.sampleRate);
    }
    
    void reset() override {
        //have to double check on this logic
        pitchShifter.reset(pitchShifter.fs);
    }
    
    void parameterChanged (const std::string& parameterID, float newValue) override {
        if(paramterID == "Mix")
        {
            pitchShifter.setMixAmount(newValue);
        } else if (paramterID == "Shift")
        {
            pitchShfiter.setShiftAmount(newValue);
        } else if (paramterID == "Tune")
        {
            pitchShfiter.setTuneAmount(newValue);
        } else if (paramterID == "Amount")
        {
            pitchShfiter.setAmountAmount(newValue);
        } else if (paramterID == "Glide")
        {
            pitchShfiter.setGlideAmount(newValue);
        }
        
        //TODO: implement setKey
        
        //TODO: implement setScale
    }
    
    void process(const juce::dsp::ProcessContextReplacing<float>& context) override
    {
        auto& buffer = context.getOutputBlock();
        
        auto* leftChannel = buffer.getChannelPointer(0);
        auto* rightChannel = buffer.getChannelPointer(1);
        
        auto numSamples = buffer.getNumSamples();
        
        const float* input = buffer.getReadPointer (0);
        float* output1 = buffer.getWritePointer(0);
        float* output2 = buffer.getWritePointer(1);
        
        std::vector<const float*> inputs = { reinterpret_cast<const float*>(input)};
        std::vector<float*> outputs = { reinterpret_cast<float*>(output1), reinterpret_cast<float*>(output2)};
        
        pitchShifter.ProcessFloatReplacing (inputs.data(), outputs.data(), numSamples);
    }

    juce::String getName() const override { return "Au2Tune Effect"; }
    
private:
    PitchShifter pitchShifter;
}
