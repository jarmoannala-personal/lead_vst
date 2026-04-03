#include "SynthVoice.h"

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::prepareToPlay(double sr)
{
    sampleRate = sr;
    osc1.setSampleRate(sr);
    osc2.setSampleRate(sr);
    lfo.setSampleRate(sr);
    filter.setSampleRate(sr);
    filter.reset();
    lfo.setWaveform(Oscillator::Waveform::Sine);
}

void SynthVoice::startNote(int midiNoteNumber, float velocity,
                           juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    currentNote = midiNoteNumber;
    noteVelocity = velocity;

    targetFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    // Glide: if we've played a note before and glide > 0, slide from previous freq
    if (hasPlayedBefore && params.glideTime > 0.001f)
    {
        // Exponential glide coefficient per sample
        double glideTimeSamples = params.glideTime * sampleRate;
        glideCoeff = 1.0 - std::exp(-1.0 / glideTimeSamples);
    }
    else
    {
        currentFrequency = targetFrequency;
        glideCoeff = 1.0;
    }
    hasPlayedBefore = true;

    // Pitch wheel
    pitchWheelMoved(currentPitchWheelPosition);

    // Reset oscillators
    osc1.reset();
    osc2.reset();

    // Reset filter
    filter.reset();

    // Start envelopes
    ampEnv.setSampleRate(sampleRate);
    ampEnv.setParameters({ params.ampAttack, params.ampDecay,
                           params.ampSustain, params.ampRelease });
    ampEnv.noteOn();

    filterEnv.setSampleRate(sampleRate);
    filterEnv.setParameters({ params.filterAttack, params.filterDecay,
                              params.filterSustain, params.filterRelease });
    filterEnv.noteOn();
}

void SynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }
    else
    {
        ampEnv.reset();
        filterEnv.reset();
        clearCurrentNote();
    }
}

void SynthVoice::pitchWheelMoved(int newValue)
{
    // Map 0-16383 to -2..+2 semitones
    pitchWheelSemitones = (newValue - 8192.0) / 8192.0 * 2.0;
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                  int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    // Update envelope parameters (in case they changed mid-note)
    ampEnv.setParameters({ params.ampAttack, params.ampDecay,
                           params.ampSustain, params.ampRelease });
    filterEnv.setParameters({ params.filterAttack, params.filterDecay,
                              params.filterSustain, params.filterRelease });

    // Filter slope
    filter.setSlope(params.filterSlope == 0 ? LadderFilter::Slope::TwoPole
                                            : LadderFilter::Slope::FourPole);

    // Set oscillator waveforms
    osc1.setWaveform(static_cast<Oscillator::Waveform>(params.osc1Waveform));
    osc2.setWaveform(static_cast<Oscillator::Waveform>(params.osc2Waveform));

    // LFO setup
    lfo.setFrequency(params.lfoRate);

    for (int sample = startSample; sample < startSample + numSamples; ++sample)
    {
        // --- Glide ---
        if (glideCoeff < 1.0)
            currentFrequency += (targetFrequency - currentFrequency) * glideCoeff;
        else
            currentFrequency = targetFrequency;

        // --- LFO ---
        float lfoValue = lfo.process() * params.lfoDepth;

        // --- Pitch modulation ---
        double pitchMod = pitchWheelSemitones;
        if (params.lfoDestination == 0 || params.lfoDestination == 2)
            pitchMod += lfoValue * 2.0; // LFO vibrato: up to 2 semitones

        double pitchMultiplier = std::pow(2.0, pitchMod / 12.0);

        // --- Oscillator frequencies ---
        double osc1Freq = currentFrequency * pitchMultiplier;
        double osc2Offset = params.osc2Coarse + params.osc2Fine / 100.0;
        double osc2Freq = currentFrequency * pitchMultiplier * std::pow(2.0, osc2Offset / 12.0);

        osc1.setFrequency(osc1Freq);
        osc2.setFrequency(osc2Freq);

        // --- Mix oscillators ---
        float oscMix = osc1.process() * params.osc1Level
                     + osc2.process() * params.osc2Level;

        // --- Noise ---
        if (params.noiseLevel > 0.001f)
            oscMix += (noiseGen.nextFloat() * 2.0f - 1.0f) * params.noiseLevel;

        // --- Filter ---
        float filterEnvValue = filterEnv.getNextSample();

        // Base cutoff + envelope modulation + key tracking + LFO
        double cutoff = params.filterCutoff;

        // Envelope modulation (bipolar, scaled to ±8 octaves)
        cutoff *= std::pow(2.0, params.filterEnvAmount * filterEnvValue * 8.0);

        // Key tracking: at 1.0, filter follows pitch exactly
        if (params.filterKeyTracking > 0.001f)
        {
            double noteOffset = (currentNote - 60.0) / 12.0; // octaves from C4
            cutoff *= std::pow(2.0, noteOffset * params.filterKeyTracking);
        }

        // LFO to filter
        if (params.lfoDestination == 1 || params.lfoDestination == 2)
            cutoff *= std::pow(2.0, lfoValue * 2.0); // up to 2 octaves

        filter.setParameters(cutoff, params.filterResonance);
        float filtered = filter.process(oscMix);

        // --- Amp envelope ---
        float ampEnvValue = ampEnv.getNextSample();
        float output = filtered * ampEnvValue * noteVelocity * 0.3f;

        // --- Output to stereo ---
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.addSample(channel, sample, output);
    }

    if (!ampEnv.isActive())
        clearCurrentNote();
}
