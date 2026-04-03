#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

// Band-limited oscillator using polyBLEP anti-aliasing
class Oscillator
{
public:
    enum class Waveform { Sine, Saw, Square, Triangle };

    void setSampleRate(double sr) { sampleRate = sr; }

    void setFrequency(double freq)
    {
        frequency = freq;
        phaseIncrement = frequency / sampleRate;
    }

    void setWaveform(Waveform wf) { waveform = wf; }

    void reset() { phase = 0.0; }

    float process()
    {
        float output = 0.0f;

        switch (waveform)
        {
            case Waveform::Sine:
                output = static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi));
                break;

            case Waveform::Saw:
                output = static_cast<float>(2.0 * phase - 1.0);
                output -= polyBLEP(phase, phaseIncrement);
                break;

            case Waveform::Square:
                output = (phase < 0.5) ? 1.0f : -1.0f;
                output += polyBLEP(phase, phaseIncrement);
                output -= polyBLEP(std::fmod(phase + 0.5, 1.0), phaseIncrement);
                break;

            case Waveform::Triangle:
                // Integrated square wave, then normalised
                output = (phase < 0.5) ? 1.0f : -1.0f;
                output += polyBLEP(phase, phaseIncrement);
                output -= polyBLEP(std::fmod(phase + 0.5, 1.0), phaseIncrement);
                // Leaky integrator to form triangle from square
                triState = 0.999f * triState + output * static_cast<float>(4.0 * phaseIncrement);
                output = triState;
                break;
        }

        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        return output;
    }

private:
    // PolyBLEP residual — smooths discontinuities near transitions
    static float polyBLEP(double phase, double increment)
    {
        double dt = increment;
        if (dt <= 0.0) return 0.0f;
        if (phase < dt)
        {
            double t = phase / dt;
            return static_cast<float>(t + t - t * t - 1.0);
        }
        else if (phase > 1.0 - dt)
        {
            double t = (phase - 1.0) / dt;
            return static_cast<float>(t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    Waveform waveform = Waveform::Saw;
    double sampleRate = 44100.0;
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float triState = 0.0f; // integrator state for triangle
};
