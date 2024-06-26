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

#include "fft/fftsetup.h"
#include "fft/mayer_fft.c"
#include "fft/Scales.h"
#include <math.h>

#define L2SC (float)3.32192809488736218171

class AutoPitchCorrector
{
public:
    
    unsigned long originalSampleRate;
    unsigned long fs; // Sample rate
    
    AutoPitchCorrector()
    {
        init(fs);
        
        //By default we have root of C and scale is Chromatic
        setScale(scales.NoteC, scales.Chromatic);
    }
    
    ~AutoPitchCorrector()
    {
        fft_des(fmembvars);
    };
    
    void Reset()
    {
        unsigned long sr = fs;
        
        if( fs != sr) init(sr);
    }
    
    void ProcessFloatReplacing(const float** inputs, float** outputs, int nFrames)
    {
        const float* in1 = inputs[0];
        float* out1 = outputs[0];
        float* out2 = outputs[1];
        float fPersist = glidepersist;

        aref = (float)440 * pow(2, fTune / 12);

        unsigned long N = cbsize;
        unsigned long Nf = corrsize;

        long int ti, ti2, ti3;
        float tf, tf2, tf3;

        for (int s = 0; s < nFrames; ++s)
        {
            // Load data into circular buffer
            tf = (float)in1[s];
            circularBuffer[cBufferWriteIndex] = tf;
            cBufferWriteIndex++;
            if (cBufferWriteIndex >= N) {
                cBufferWriteIndex = 0;
            }

            // ********************
            // * Low-rate section *
            // ********************

            // Every N/noverlap samples, run pitch estimation / correction code
            if ((cBufferWriteIndex) % (N / noverlap) == 0)
            {
                // ---- Obtain autocovariance ----

                // Window and fill FFT buffer
                ti2 = (long)cBufferWriteIndex;
                for (ti = 0; ti < (long)N; ti++) {
                    ffttime[ti] = (float)(circularBuffer[(ti2 - ti) % N] * cbwindow[ti]);
                }

                // Calculate FFT
                fft_forward(fmembvars, ffttime.data(), fftfreqre.data(), fftfreqim.data());

                // Remove DC
                fftfreqre[0] = 0;
                fftfreqim[0] = 0;

                // Take magnitude squared
                for (ti = 1; ti < (long)Nf; ti++) {
                    fftfreqre[ti] = (fftfreqre[ti]) * (fftfreqre[ti]) + (fftfreqim[ti]) * (fftfreqim[ti]);
                    fftfreqim[ti] = 0;
                }

                // Calculate IFFT
                fft_inverse(fmembvars, fftfreqre.data(), fftfreqim.data(), ffttime.data());

                // Normalize
                for (ti = 1; ti < (long)N; ti++) {
                    ffttime[ti] = ffttime[ti] / ffttime[0];
                }
                ffttime[0] = 1;

                // ---- END Obtain autocovariance ----

                // ---- Calculate pitch and confidence ----

                // Calculate pitch period
                tf2 = 0;
                pperiod = pmin;
                for (ti = nmin; ti < (long)nmax; ti++) {
                    ti2 = ti - 1;
                    ti3 = ti + 1;
                    if (ti2 < 0) ti2 = 0;
                    if (ti3 > (long)Nf) ti3 = Nf;
                    tf = ffttime[ti];

                    if (tf > ffttime[ti2] && tf >= ffttime[ti3] && tf > tf2) {
                        tf2 = tf;
                        conf = tf * acwinv[ti];
                        pperiod = (float)ti / fs;
                    }
                }

                // Convert to semitones
                pitch = (float)-12 * log10((float)aref * pperiod) * L2SC;

                // ---- END Calculate pitch and confidence ----

                // ---- Determine pitch target ----

                // If voiced
                if (conf >= vthresh) {
                    // Determine pitch target
                    tf = -1;
                    tf2 = 0;
                    tf3 = 0;
                    for (ti = 0; ti < 12; ti++) {
                        switch (ti) {
                            case 0: tf2 = fNotes[9]; break;
                            case 1: tf2 = fNotes[10]; break;
                            case 2: tf2 = fNotes[11]; break;
                            case 3: tf2 = fNotes[0]; break;
                            case 4: tf2 = fNotes[1]; break;
                            case 5: tf2 = fNotes[2]; break;
                            case 6: tf2 = fNotes[3]; break;
                            case 7: tf2 = fNotes[4]; break;
                            case 8: tf2 = fNotes[5]; break;
                            case 9: tf2 = fNotes[6]; break;
                            case 10: tf2 = fNotes[7]; break;
                            case 11: tf2 = fNotes[8]; break;
                        }
                        tf2 = tf2 - (float)fabs((pitch - (float)ti) / 6 - 2 * floorf(((pitch - (float)ti) / 12 + 0.5)));
                        if (tf2 >= tf) {
                            tf3 = (float)ti;
                            tf = tf2;
                        }
                    }
                    ptarget = tf3;

                    // Glide persist
                    if (wasvoiced == 0) {
                        wasvoiced = 1;
                        tf = persistamt;
                        sptarget = (1 - tf) * ptarget + tf * sptarget;
                        persistamt = 1;
                    }

                    // Glide on circular scale
                    tf3 = (float)ptarget - sptarget;
                    tf3 = tf3 - (float)12 * floorf(tf3 / 12 + 0.5);
                    tf2 = (fGlide > 0) ? (1 - pow((float)1 / 24, (float)N * 1000 / (noverlap * fs * fGlide))) : 1;
                    sptarget = sptarget + tf3 * tf2;
                }
                // If not voiced
                else {
                    wasvoiced = 0;
                    tf = (fPersist > 0) ? pow((float)1 / 2, (float)N * 1000 / (noverlap * fs * fPersist)) : 0;
                    persistamt = persistamt * tf; // Persist amount decays exponentially
                }
                // END If voiced

                // ---- END Determine pitch target ----

                // ---- Determine correction to feed to the pitch shifter ----
                tf = sptarget - pitch; // Correction amount
                tf = tf - (float)12 * floorf(tf / 12 + 0.5); // Never do more than +- 6 semitones of correction
                if (conf < vthresh) {
                    tf = 0;
                }
                lrshift = fShift + fAmount * tf; // Add in pitch shift slider

                // ---- Compute variables for pitch shifter that depend on pitch ----
                phincfact = (float)pow(2, lrshift / 12);
                if (conf >= vthresh) {
                    phinc = (float)1 / (pperiod * fs);
                    phprd = pperiod * 2;
                }
            }
            // ************************
            // * END Low-Rate Section *
            // ************************

            // *****************
            // * Pitch Shifter *
            // *****************

            // Pitch shifter (overlap-add, pitch synchronous)
            phasein = phasein + phinc;
            phaseout = phaseout + phinc * phincfact;

            // When input phase resets, take a snippet from N/2 samples in the past
            if (phasein >= 1) {
                phasein = phasein - 1;
                ti2 = cBufferWriteIndex - (long int)N / 2;
                for (ti = -((long int)N) / 2; ti < (long int)N / 2; ti++) {
                    frag[ti % N] = circularBuffer[(ti + ti2) % N];
                }
            }

            // When output phase resets, put a snippet N/2 samples in the future
            if (phaseout >= 1) {
                fragsize = fragsize * 2;
                if (fragsize >= N) {
                    fragsize = N;
                }
                phaseout = phaseout - 1;
                ti2 = cbord + N / 2;
                ti3 = (long int)(((float)fragsize) / phincfact);
                for (ti = -ti3 / 2; ti < (ti3 / 2); ti++) {
                    tf = hannwindow[(long int)N / 2 + ti * (long int)N / ti3];
                    cbo[(ti + ti2) % N] = cbo[(ti + ti2) % N] + frag[((int)(phincfact * ti)) % N] * tf;
                    cbonorm[(ti + ti2) % N] = cbonorm[(ti + ti2) % N] + tf;
                }
                fragsize = 0;
            }
            fragsize++;

            // Get output signal from buffer and normalize
            tf = cbonorm[cbord];
            tf = (tf > 0.5) ? (float)1 / tf : 1;
            tf = tf * cbo[cbord]; // read buffer
            tf = cbo[cbord];
            cbo[cbord] = 0; // erase for next cycle
            cbonorm[cbord] = 0;
            cbord++;
            if (cbord >= N) {
                cbord = 0;
            }

            // *********************
            // * END Pitch Shifter *
            // *********************

            // Write audio to output of plugin
            out1[s] = (double)fMix * tf + (1.0 - fMix) * circularBuffer[(cBufferWriteIndex - N + 1) % N];
            out2[s] = (double)fMix * tf + (1.0 - fMix) * circularBuffer[(cBufferWriteIndex - N + 1) % N];
        }
    }
    
    void init(unsigned long sr)
    {
        originalSampleRate = sr;
        unsigned long ti;
        
        fs = sr;
        aref = 440;
        
        if (fs >=88200) {
            cbsize = 4096;
        }
        else {
            cbsize = 2048;
        }
        corrsize = cbsize / 2 + 1;
        
        pmax = 1/(float)70;
        pmin = 1/(float)700;
        
        pperiod = pmax;
        
        nmax = (unsigned long)(fs * pmax);
        if (nmax > corrsize) {
            nmax = corrsize;
        }
        nmin = (unsigned long)(fs * pmin);
        
        circularBuffer.resize (cbsize);
        cbo.resize (cbsize);
        cbonorm.resize (cbsize);
        
        cBufferWriteIndex = 0;
        cbord = 0;
        
        hannwindow.resize (cbsize);
        for (ti=0; ti<cbsize; ti++) {
            hannwindow[ti] = -0.5*cos(2*M_PI*ti/(cbsize - 1)) + 0.5;
        }
        
        cbwindow.resize (cbsize);
        for (ti=0; ti<(cbsize / 2); ti++) {
            cbwindow[ti+cbsize/4] = -0.5*cos(4*M_PI*ti/(cbsize - 1)) + 0.5;
        }
        
        noverlap = 4;
        
        fmembvars = fft_con (cbsize);
        
        ffttime.resize (cbsize);
        fftfreqre.resize (cbsize);
        fftfreqim.resize (cbsize);
        
        acwinv.resize (cbsize);
        for (ti=0; ti<cbsize; ti++) {
            ffttime[ti] = cbwindow[ti];
        }
        fft_forward(fmembvars, cbwindow.data(), fftfreqre.data(), fftfreqim.data());
        for (ti=0; ti<corrsize; ti++) {
            fftfreqre[ti] = (fftfreqre[ti])*(fftfreqre[ti]) + (fftfreqim[ti])*(fftfreqim[ti]);
            fftfreqim[ti] = 0;
        }
        fft_inverse(fmembvars, fftfreqre.data(), fftfreqim.data(), ffttime.data());
        for (ti=1; ti<cbsize; ti++) {
            acwinv[ti] = ffttime[ti]/ffttime[0];
            if (acwinv[ti] > 0.000001) {
                acwinv[ti] = (float)1/acwinv[ti];
            }
            else {
                acwinv[ti] = 0;
            }
        }
        acwinv[0] = 1;
        
        lrshift = 0;
        ptarget = 0;
        sptarget = 0;
        wasvoiced = 0;
        persistamt = 0;
        
        glidepersist = 100;
        
        vthresh = 0.8;
        
        phprdd = 0.01;
        phprd = phprdd;
        phinc = (float)1/(phprd * fs);
        phincfact = 1;
        phasein = 0;
        phaseout = 0;
        frag.resize (cbsize);
        fragsize = 0;
    }
    
    void setMixAmount(float mixAmt){
        fMix = mixAmt;
    }
    void setShiftAmount(float shiftAmt){
        fShift = shiftAmt;
    }
    void setTuneAmount(float tuneAmt){
        fTune = tuneAmt;
    }
    void setAmountAmount(float amtAmt){
        fAmount = amtAmt;
    }
    void setGlideAmount(float glideAmt){
        fGlide = glideAmt;
    }
    
    void setScale(int root, int scale){
        fRoot = root;
        fScale = scale;
      int sc[12];
      scales.makeScale(root, scale, sc);
      for (int i = 0; i< 12; i++) {
          fNotes[i] = sc[i];
      }
    }
    
    float getMixAmount(){
        return fMix;
    }
    float getShiftAmount(){
        return fShift;
    }
    float getTuneAmount(){
        return fTune;
    }
    float getAmountAmount(){
        return fAmount;
    }
    float getGlideAmount(){
        return fGlide;
    }
    int getScale(){
        return fScale;
    }
    int getRoot(){
        return fRoot;
    }
    
private:
    
    // parameters
    float fMix = 1.0f; //This is our scaling factor of how much total effect is applied here from 0 -> 1
    //side note, if we place it at 100, it is very loud and creepy autoTuned like a monster
    
    float fShift = 0; // Easy This transposes from -12 to +12 semitones, step size of 1
    
    float fTune = 0.0f; // This is -100 -> +100 cents transposed, parameter in code is from -1 to +1 step size should be 0.01
    
    float fAmount = 1.0f; //Amount of pitch slide how much you want it? from 0 -> 1
    
    float fGlide = 1.0f; //control how much pitch slide from one note to the next 0 -> 1000ms step size 0.01. Param itself is 0 -> 1 step size 0.00001.
    
    float fNotes[12];
    
    int fRoot;
    
    int fScale;
    
    fft_vars* fmembvars; // member variables for fft routine
    
    Scales scales = Scales();
    
    enum ScaleNames{
        Chromatic=0,
        Major,
        Minor,
        Dorian,
        Mixolydian,
        Lydian,
        Phrygian,
        Locrian,
        HarmonicMinor,
        MelodicMinor,
        MajorPentatonic,
        MinorPentatonic,
        MinorBlues
    };
    
    enum Notes{
        NoteC=0,
        NoteDb,
        NoteD,
        NoteEb,
        NoteE,
        NoteF,
        NoteGb,
        NoteG,
        NoteAb,
        NoteA,
        NoteBb,
        NoteB
    };
    
    unsigned long cbsize; // size of circular buffer
    unsigned long corrsize; // cbsize/2 + 1
    unsigned long cBufferWriteIndex;
    unsigned long cbord;
    std::vector<float> circularBuffer; // circular input buffer
    std::vector<float> cbo; // circular output buffer
    std::vector<float> cbonorm; // circular output buffer used to normalize signal
    
    std::vector<float> cbwindow; // hann of length N/2, zeros for the rest
    std::vector<float> acwinv; // inverse of autocorrelation of window
    std::vector<float> hannwindow; // length-N hann
    int noverlap;
    
    std::vector<float> ffttime;
    std::vector<float> fftfreqre;
    std::vector<float> fftfreqim;
    
    // VARIABLES FOR LOW-RATE SECTION
    float aref; // A tuning reference (Hz)
    float pperiod; // Pitch period (seconds)
    float pitch; // Pitch (semitones)
    float pitchpers; // Pitch persist (semitones)
    float conf; // Confidence of pitch period estimate (between 0 and 1)
    float vthresh; // Voiced speech threshold
    
    float pmax; // Maximum allowable pitch period (seconds)
    float pmin; // Minimum allowable pitch period (seconds)
    unsigned long nmax; // Maximum period index for pitch prd est
    unsigned long nmin; // Minimum period index for pitch prd est
    
    float lrshift; // Shift prescribed by low-rate section
    int ptarget; // Pitch target, between 0 and 11
    float sptarget; // Smoothed pitch target
    int wasvoiced; // 1 if previous frame was voiced
    float persistamt; // Proportion of previous pitch considered during next voiced period
    float glidepersist;
    
    // VARIABLES FOR PITCH SHIFTER
    float phprd; // phase period
    float phprdd; // default (unvoiced) phase period
    float phinc; // input phase increment
    float phincfact; // factor determining output phase increment
    float phasein;
    float phaseout;
    std::vector<float> frag; // windowed fragment of speech
    unsigned long fragsize; // size of fragment in samples
    
};

class AutoPitchEffect : public AudioEffect
{
public:
    
    AutoPitchEffect()
    {
        //Amount of pitch correction how much you want it?
        registerParameter("Amount", 0.0f, 0.0f, 100.0f, 1.0f, "%");
    
        //control how much pitch slide from one note to the next 0 -> 1000ms
        registerParameter("Glide", 0.0f, 0.0f, 1000.0f, 0.01f, "ms");
    
        //This is our scaling factor of how much total effect is applied
        registerParameter("Mix", 0.0f, 0.0f, 100.0f, 1.0f, "%");
        
        // Easy This transposes from -12 to +12 semitones, step size of 1
        registerParameter("Shift", 0.0f, -12.0f, 12.0f, 1.0f, "st");
        
        // This is -100 -> +100 cents transposed
        registerParameter("Tune", 0.0f, -100.0f, 100.0f, 1.0f, "cents");
        
        //TODO: register Root with a drop down menu
        
        //TODO: register Scale with a drop down menu
    };
    
    void prepare(const juce::dsp::ProcessSpec& spec) override {
            autoPitchCorrector.init(spec.sampleRate);
    }
    
    void reset() override {
        //have to double check on this logic
        //autoPitchCorrector.reset(pitchShifter.fs);
    }
    
    void parameterChanged (const std::string& parameterID, float newValue) override {
        if(parameterID == "Mix")
        {
            autoPitchCorrector.setMixAmount(newValue / 100);
        } else if (parameterID == "Shift")
            {
            autoPitchCorrector.setShiftAmount(newValue);
        } else if (parameterID == "Tune")
            {
            autoPitchCorrector.setTuneAmount(newValue / 100);
        } else if (parameterID == "Amount")
                {
            autoPitchCorrector.setAmountAmount(newValue / 100);
        } else if (parameterID == "Glide")
                {
            autoPitchCorrector.setGlideAmount(newValue / 1000);
        }
        
        //TODO: use setRoot with a drop down menu
        //AutoPitchCorrector.setScale(newRoot, autoPitchCorrector.getScale());
        //
        
        //TODO: implement setScaleType with a drop down menu
        //AutoPitchCorrector.setScale(autoPitchCorrect.getRoot(), newScale);
    }
    
    void process(const juce::dsp::ProcessContextReplacing<float>& context) override
    {
        auto& buffer = context.getOutputBlock();
        
        auto numSamples = buffer.getNumSamples();
        
const float* input = buffer.getChannelPointer (0);
float* output1 = buffer.getChannelPointer(0);
float* output2 = buffer.getChannelPointer(1);
        
        std::vector<const float*> inputs = { reinterpret_cast<const float*>(input)};
        std::vector<float*> outputs = { reinterpret_cast<float*>(output1), reinterpret_cast<float*>(output2)};
        
            autoPitchCorrector.ProcessFloatReplacing (inputs.data(), outputs.data(), numSamples);
    }
    
int getNumParameters() const override { return AdvancedPlateReverbEffect::ParamCount; }

    juce::String getName() const override { return "AutoPitchEffect"; }
    
private:
    AutoPitchCorrector autoPitchCorrector;
};
