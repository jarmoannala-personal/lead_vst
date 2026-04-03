#pragma once
#include <cmath>
#include <algorithm>

// Multi-mode ladder filter with selectable slope
// 12 dB/oct (2-pole, Oberheim-style) or 24 dB/oct (4-pole, Moog-style)
class LadderFilter
{
public:
    enum class Slope { TwoPole, FourPole };

    void setSampleRate(double sr)
    {
        sampleRate = sr;
    }

    void reset()
    {
        for (int i = 0; i < 4; ++i)
        {
            stage[i] = 0.0;
            delay[i] = 0.0;
        }
    }

    void setSlope(Slope s) { slope = s; }

    // cutoffHz: filter cutoff in Hz
    // reso: resonance 0.0 - 1.0
    void setParameters(double cutoffHz, double reso)
    {
        cutoffHz = std::clamp(cutoffHz, 20.0, sampleRate * 0.45);
        resonance = std::clamp(reso, 0.0, 1.0);

        double fc = cutoffHz / sampleRate;
        g = 4.0 * juce::MathConstants<double>::pi * fc;
        g = g / (1.0 + g);
    }

    float process(float input)
    {
        int numPoles = (slope == Slope::TwoPole) ? 2 : 4;

        // Feedback tap from the last active stage
        double feedbackTap = delay[numPoles - 1];

        // Resonance scaling — 2-pole needs less feedback to self-oscillate
        double maxFeedback = (slope == Slope::TwoPole) ? 2.0 : 4.0;
        double feedback = resonance * maxFeedback;
        double inputWithFeedback = input - feedback * feedbackTap;

        // Soft-clip for analog warmth
        inputWithFeedback = std::tanh(inputWithFeedback);

        // Cascaded one-pole filters
        for (int i = 0; i < numPoles; ++i)
        {
            double in = (i == 0) ? inputWithFeedback : stage[i - 1];
            stage[i] = delay[i] + g * (in - delay[i]);
            delay[i] = stage[i];
        }

        return static_cast<float>(stage[numPoles - 1]);
    }

private:
    Slope slope = Slope::FourPole;
    double sampleRate = 44100.0;
    double resonance = 0.0;
    double g = 0.0;
    double stage[4] = {};
    double delay[4] = {};
};
