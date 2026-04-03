#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Oscillator.h"
#include "LadderFilter.h"

// Simple synth sound — accepts all MIDI channels/notes
class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// Full analog-style voice: dual oscillator + ladder filter + dual ADSR + LFO + glide
class SynthVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override;

    void prepareToPlay(double sampleRate);

    // Called per-block from the processor to push current parameter values
    struct VoiceParams
    {
        // Oscillators
        int osc1Waveform = 1;       // 0=sine, 1=saw, 2=square, 3=triangle
        int osc2Waveform = 1;
        float osc1Level = 1.0f;
        float osc2Level = 0.5f;
        int osc2Coarse = 0;         // semitones
        float osc2Fine = 0.0f;      // cents
        float noiseLevel = 0.0f;

        // Filter
        float filterCutoff = 8000.0f;
        float filterResonance = 0.0f;
        float filterEnvAmount = 0.0f;   // bipolar: -1 to +1
        float filterKeyTracking = 0.0f; // 0 to 1
        int filterSlope = 1;            // 0=12dB (2-pole), 1=24dB (4-pole)

        // Filter envelope
        float filterAttack = 0.01f;
        float filterDecay = 0.3f;
        float filterSustain = 0.0f;
        float filterRelease = 0.3f;

        // Amp envelope
        float ampAttack = 0.01f;
        float ampDecay = 0.1f;
        float ampSustain = 0.8f;
        float ampRelease = 0.3f;

        // LFO
        float lfoRate = 3.0f;       // Hz
        float lfoDepth = 0.0f;      // 0 to 1
        int lfoDestination = 0;     // 0=pitch, 1=filter, 2=both

        // Glide
        float glideTime = 0.0f;     // seconds
    };

    void setParams(const VoiceParams& p) { params = p; }

private:
    VoiceParams params;

    Oscillator osc1, osc2;
    Oscillator lfo;
    LadderFilter filter;

    juce::ADSR ampEnv, filterEnv;

    // Glide
    double targetFrequency = 440.0;
    double currentFrequency = 440.0;
    double glideCoeff = 1.0;
    bool hasPlayedBefore = false;

    // Pitch wheel
    double pitchWheelSemitones = 0.0;

    // Noise
    juce::Random noiseGen;

    int currentNote = -1;
    float noteVelocity = 0.0f;

    double sampleRate = 44100.0;
};
