#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <algorithm>

class Arpeggiator
{
public:
    enum class Pattern { Up, Down, UpDown, Random };
    enum class SyncMode { Internal, HostSync };
    enum class RateDiv { Quarter, Eighth, EighthTriplet, Sixteenth, SixteenthTriplet, ThirtySecond };

    Arpeggiator()
    {
        // Pre-allocate to avoid audio-thread heap allocations
        heldNotes.reserve(16);
        latchedNotes.reserve(16);
        sequence.reserve(64);
        sortBuffer.reserve(16);
    }

    void setSampleRate(double sr) { sampleRate = sr; }

    void setEnabled(bool on, juce::MidiBuffer* buffer = nullptr, int samplePos = 0)
    {
        if (enabled && !on)
        {
            if (currentPlayingNote >= 0 && buffer != nullptr)
                buffer->addEvent(
                    juce::MidiMessage::noteOff(currentChannel, currentPlayingNote), samplePos);
            reset();
        }
        enabled = on;
    }
    bool isEnabled() const { return enabled; }

    void setTempoBPM(float bpm) { tempoBPM = std::clamp(bpm, 30.0f, 300.0f); }
    void setPattern(Pattern p) { pattern = p; }
    void setOctaveRange(int range) { octaveRange = std::clamp(range, 1, 4); }
    void setGateLength(float g) { gateLength = std::clamp(g, 0.05f, 1.0f); }
    void setLatch(bool on) { latchMode = on; if (!on) latchedNotes.clear(); }
    void setSyncMode(SyncMode m) { syncMode = m; }
    void setRateDiv(RateDiv r) { rateDiv = r; }

    // Called from processBlock — lock-free, no allocations
    void process(juce::MidiBuffer& midiMessages, int numSamples,
                 const juce::AudioPlayHead::PositionInfo* posInfo)
    {
        if (!enabled)
            return;

        // Process incoming note on/off to update held notes
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn(msg.getNoteNumber(), msg.getChannel());
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber(), msg.getChannel());
        }

        // Remove all note messages (replace with arp output)
        // Use pre-allocated buffer
        filteredBuffer.clear();
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (!msg.isNoteOn() && !msg.isNoteOff())
                filteredBuffer.addEvent(msg, metadata.samplePosition);
        }
        midiMessages.swapWith(filteredBuffer);

        // Get active note list
        auto& notes = latchMode ? latchedNotes : heldNotes;
        if (notes.empty())
        {
            if (currentPlayingNote >= 0)
            {
                midiMessages.addEvent(
                    juce::MidiMessage::noteOff(currentChannel, currentPlayingNote), 0);
                currentPlayingNote = -1;
            }
            return;
        }

        // Rebuild sequence only when notes changed
        if (sequenceDirty)
        {
            buildSequence(notes);
            sequenceDirty = false;
        }

        // Step timing
        double notesPerBeat = rateDivToMultiplier(rateDiv);
        double bpm = (syncMode == SyncMode::HostSync && posInfo != nullptr
                      && posInfo->getBpm().hasValue())
                     ? *posInfo->getBpm()
                     : static_cast<double>(tempoBPM);

        double stepDuration = (sampleRate * 60.0) / (bpm * notesPerBeat);
        if (stepDuration < 1.0) stepDuration = 1.0;

        int gateOnSamples = static_cast<int>(stepDuration * gateLength);

        // Generate arp notes
        for (int i = 0; i < numSamples; ++i)
        {
            if (sampleCounter == 0)
            {
                if (currentPlayingNote >= 0)
                    midiMessages.addEvent(
                        juce::MidiMessage::noteOff(currentChannel, currentPlayingNote), i);

                int note = getNextNote();
                if (note >= 0)
                {
                    currentPlayingNote = note;
                    midiMessages.addEvent(
                        juce::MidiMessage::noteOn(currentChannel, note, currentVelocity), i);
                }
            }
            else if (sampleCounter == gateOnSamples && currentPlayingNote >= 0)
            {
                midiMessages.addEvent(
                    juce::MidiMessage::noteOff(currentChannel, currentPlayingNote), i);
                currentPlayingNote = -1;
            }

            ++sampleCounter;
            if (sampleCounter >= static_cast<int>(stepDuration))
                sampleCounter = 0;
        }
    }

    void reset()
    {
        heldNotes.clear();
        currentPlayingNote = -1;
        stepIndex = 0;
        sampleCounter = 0;
        sequenceDirty = true;
    }

    void allNotesOff()
    {
        heldNotes.clear();
        latchedNotes.clear();
        currentPlayingNote = -1;
        stepIndex = 0;
        sampleCounter = 0;
        sequenceDirty = true;
    }

private:
    void noteOn(int noteNumber, int channel)
    {
        currentChannel = channel;
        currentVelocity = 0.8f;

        if (latchMode)
        {
            if (heldNotes.empty())
                latchedNotes.clear();

            if (std::find(latchedNotes.begin(), latchedNotes.end(), noteNumber) == latchedNotes.end())
                latchedNotes.push_back(noteNumber);
        }

        if (std::find(heldNotes.begin(), heldNotes.end(), noteNumber) == heldNotes.end())
            heldNotes.push_back(noteNumber);

        sequenceDirty = true;
    }

    void noteOff(int noteNumber, int /*channel*/)
    {
        auto it = std::find(heldNotes.begin(), heldNotes.end(), noteNumber);
        if (it != heldNotes.end())
            heldNotes.erase(it);

        sequenceDirty = true;
    }

    void buildSequence(const std::vector<int>& notes)
    {
        sequence.clear();
        sortBuffer.clear();
        for (int n : notes) sortBuffer.push_back(n);
        std::sort(sortBuffer.begin(), sortBuffer.end());

        for (int oct = 0; oct < octaveRange; ++oct)
            for (int n : sortBuffer)
                sequence.push_back(n + oct * 12);

        // Clamp step index to new sequence length
        if (!sequence.empty())
            stepIndex = stepIndex % static_cast<int>(sequence.size());
        else
            stepIndex = 0;
    }

    int getNextNote()
    {
        if (sequence.empty()) return -1;

        int note = -1;
        int seqLen = static_cast<int>(sequence.size());

        switch (pattern)
        {
            case Pattern::Up:
                stepIndex = stepIndex % seqLen;
                note = sequence[static_cast<size_t>(stepIndex)];
                ++stepIndex;
                break;

            case Pattern::Down:
                stepIndex = stepIndex % seqLen;
                note = sequence[static_cast<size_t>(seqLen - 1 - stepIndex)];
                ++stepIndex;
                break;

            case Pattern::UpDown:
                if (seqLen <= 1) { note = sequence[0]; break; }
                {
                    int cycleLen = (seqLen - 1) * 2;
                    int pos = stepIndex % cycleLen;
                    note = (pos < seqLen)
                        ? sequence[static_cast<size_t>(pos)]
                        : sequence[static_cast<size_t>(cycleLen - pos)];
                    ++stepIndex;
                }
                break;

            case Pattern::Random:
                note = sequence[static_cast<size_t>(random.nextInt(seqLen))];
                break;
        }

        return note;
    }

    static double rateDivToMultiplier(RateDiv r)
    {
        switch (r)
        {
            case RateDiv::Quarter:           return 1.0;
            case RateDiv::Eighth:            return 2.0;
            case RateDiv::EighthTriplet:     return 3.0;
            case RateDiv::Sixteenth:         return 4.0;
            case RateDiv::SixteenthTriplet:  return 6.0;
            case RateDiv::ThirtySecond:      return 8.0;
            default:                         return 4.0;
        }
    }

    bool enabled = false;
    float tempoBPM = 120.0f;
    Pattern pattern = Pattern::Up;
    int octaveRange = 1;
    float gateLength = 0.8f;
    bool latchMode = false;
    SyncMode syncMode = SyncMode::Internal;
    RateDiv rateDiv = RateDiv::Sixteenth;

    double sampleRate = 44100.0;
    std::vector<int> heldNotes;
    std::vector<int> latchedNotes;
    std::vector<int> sequence;
    std::vector<int> sortBuffer;      // pre-allocated sort scratch
    juce::MidiBuffer filteredBuffer;   // pre-allocated filter scratch
    bool sequenceDirty = true;
    int stepIndex = 0;
    int sampleCounter = 0;

    int currentPlayingNote = -1;
    int currentChannel = 1;
    float currentVelocity = 0.8f;

    juce::Random random;
};
