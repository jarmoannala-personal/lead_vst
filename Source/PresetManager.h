#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct Preset
{
    juce::String name;

    // Oscillators
    int osc1Wave;       float osc1Level;
    int osc2Wave;       float osc2Level;
    int osc2Coarse;     float osc2Fine;
    float noiseLevel;

    // Filter
    float filterCutoff; float filterReso;
    float filterEnvAmt; float filterKeyTrack;
    int filterSlope;    // 0=12dB, 1=24dB

    // Filter envelope
    float fltA, fltD, fltS, fltR;

    // Amp envelope
    float ampA, ampD, ampS, ampR;

    // LFO
    float lfoRate, lfoDepth;
    int lfoDest;        // 0=pitch, 1=filter, 2=both

    // Glide
    float glideTime;

    // Master
    float masterGain;

    // Arpeggiator (optional — defaults applied if not set)
    bool arpEnabled = false;
    float arpTempo = 120.0f;
    int arpPattern = 0;     // 0=Up, 1=Down, 2=Up/Down, 3=Random
    int arpOctaves = 1;
    float arpGate = 0.8f;
    bool arpLatch = false;
    int arpSync = 0;        // 0=Internal, 1=Host
    int arpRate = 3;        // 0=1/4, 1=1/8, 2=1/8T, 3=1/16, 4=1/16T, 5=1/32
};

class PresetManager
{
public:
    PresetManager()
    {
        buildFactoryPresets();
    }

    int getNumPresets() const { return static_cast<int>(presets.size()); }

    const Preset& getPreset(int index) const
    {
        return presets[static_cast<size_t>(std::clamp(index, 0, getNumPresets() - 1))];
    }

    const Preset* findPreset(const juce::String& name) const
    {
        for (auto& p : presets)
            if (p.name == name)
                return &p;
        return nullptr;
    }

    void applyPreset(const Preset& preset, juce::AudioProcessorValueTreeState& apvts) const
    {
        auto set = [&](const juce::String& id, float value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        auto setInt = [&](const juce::String& id, int value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(value)));
        };

        setInt("osc1Wave",      preset.osc1Wave);
        set("osc1Level",        preset.osc1Level);
        setInt("osc2Wave",      preset.osc2Wave);
        set("osc2Level",        preset.osc2Level);
        setInt("osc2Coarse",    preset.osc2Coarse);
        set("osc2Fine",         preset.osc2Fine);
        set("noiseLevel",       preset.noiseLevel);

        set("filterCutoff",     preset.filterCutoff);
        set("filterReso",       preset.filterReso);
        set("filterEnvAmt",     preset.filterEnvAmt);
        set("filterKeyTrack",   preset.filterKeyTrack);
        setInt("filterSlope",   preset.filterSlope);

        set("fltAttack",        preset.fltA);
        set("fltDecay",         preset.fltD);
        set("fltSustain",       preset.fltS);
        set("fltRelease",       preset.fltR);

        set("ampAttack",        preset.ampA);
        set("ampDecay",         preset.ampD);
        set("ampSustain",       preset.ampS);
        set("ampRelease",       preset.ampR);

        set("lfoRate",          preset.lfoRate);
        set("lfoDepth",         preset.lfoDepth);
        setInt("lfoDest",       preset.lfoDest);

        set("glideTime",        preset.glideTime);
        set("masterGain",       preset.masterGain);

        // Arpeggiator
        auto setBool = [&](const juce::String& id, bool value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(value ? 1.0f : 0.0f);
        };

        setBool("arpEnabled",   preset.arpEnabled);
        set("arpTempo",         preset.arpTempo);
        setInt("arpPattern",    preset.arpPattern);
        setInt("arpOctaves",    preset.arpOctaves);
        set("arpGate",          preset.arpGate);
        setBool("arpLatch",     preset.arpLatch);
        setInt("arpSync",       preset.arpSync);
        setInt("arpRate",       preset.arpRate);
    }

    const std::vector<Preset>& getAllPresets() const { return presets; }

private:
    std::vector<Preset> presets;

    void buildFactoryPresets()
    {
        // ================================================================
        // 001 — Analog Lead (default smooth lead, Pink Floyd / MMEB style)
        //       Moog-style 24dB, warm and singing
        // ================================================================
        presets.push_back({
            "001 Analog Lead",
            /*osc1*/ 1, 1.0f,              // Saw, full
            /*osc2*/ 1, 0.5f, 0, 7.0f,     // Saw, half, no coarse, +7 cents
            /*noise*/ 0.0f,
            /*filter*/ 8000.0f, 0.15f, 0.3f, 0.5f, 1,  // 24dB
            /*fltEnv*/ 0.01f, 0.3f, 0.0f, 0.3f,
            /*ampEnv*/ 0.01f, 0.1f, 0.8f, 0.3f,
            /*lfo*/ 5.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 002 — VH Jump Brass (Van Halen "Jump" — OB-Xa)
        //       The iconic brass riff, 12dB Oberheim character
        // ================================================================
        presets.push_back({
            "002 VH Jump Brass",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.9f, 0, 4.0f,     // Saw, slight detune
            /*noise*/ 0.0f,
            /*filter*/ 11000.0f, 0.15f, 0.35f, 0.6f, 0,  // 12dB! Oberheim character
            /*fltEnv*/ 0.008f, 0.4f, 0.65f, 0.3f,
            /*ampEnv*/ 0.003f, 0.25f, 0.85f, 0.4f,
            /*lfo*/ 5.5f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 003 — VH Ill Wait Pad (Van Halen "I'll Wait" — OB-Xa)
        //       Wide warm pad, slow envelopes, wider detune
        // ================================================================
        presets.push_back({
            "003 VH Ill Wait Pad",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.85f, 0, 9.0f,    // Saw, wider detune
            /*noise*/ 0.0f,
            /*filter*/ 6000.0f, 0.08f, 0.25f, 0.5f, 0,  // 12dB, warmer cutoff
            /*fltEnv*/ 0.1f, 0.6f, 0.7f, 0.6f,
            /*ampEnv*/ 0.12f, 0.4f, 0.85f, 0.8f,
            /*lfo*/ 4.5f, 0.05f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 004 — VH Why Cant Lead (Van Halen "Why Can't This Be Love" — OB-Xa)
        //       Bright, shimmering poly lead. Oberheim OB-Xa played by Eddie.
        //       Brighter than Jump, more open filter, sparkly top end,
        //       medium attack for a smooth singing quality.
        // ================================================================
        presets.push_back({
            "004 VH Why Cant Lead",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.8f, 0, 6.0f,     // Saw, moderate detune for shimmer
            /*noise*/ 0.0f,
            /*filter*/ 14000.0f, 0.1f, 0.2f, 0.7f, 0,  // 12dB, very bright/open
            /*fltEnv*/ 0.02f, 0.5f, 0.7f, 0.4f,   // smooth onset, sustains bright
            /*ampEnv*/ 0.015f, 0.3f, 0.9f, 0.5f,   // slightly softer attack, singing
            /*lfo*/ 5.0f, 0.03f, 0,         // very subtle vibrato
            /*glide*/ 0.0f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 005 — VH Why Cant Arp (Van Halen "Why Can't This Be Love" — OB-Xa)
        //       The rhythmic arpeggiated synth pattern. Bright, tight.
        //       ARP on, LATCH on, 89 BPM, 1/16 notes.
        // ================================================================
        presets.push_back({
            "005 VH Why Cant Arp",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 2, 0.45f, 0, 3.0f,    // Square, low mix, tight detune
            /*noise*/ 0.0f,
            /*filter*/ 12000.0f, 0.12f, 0.4f, 0.8f, 0,  // 12dB, bright, snappy env
            /*fltEnv*/ 0.001f, 0.08f, 0.4f, 0.06f,
            /*ampEnv*/ 0.001f, 0.05f, 0.85f, 0.06f,
            /*lfo*/ 5.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.82f,
            // Arp: enabled, 89 BPM, Up, 1 octave, 80% gate, latch on, internal, 1/16
            /*arpEnabled*/ true, /*arpTempo*/ 89.0f, /*arpPattern*/ 0,
            /*arpOctaves*/ 1, /*arpGate*/ 0.8f, /*arpLatch*/ true,
            /*arpSync*/ 0, /*arpRate*/ 3
        });

        // ================================================================
        // 004 — OB Brass (classic Oberheim brass stab)
        //       12dB filter, punchy filter envelope
        // ================================================================
        presets.push_back({
            "006 OB Brass",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.85f, 0, 5.0f,    // Saw, moderate detune
            /*noise*/ 0.0f,
            /*filter*/ 7000.0f, 0.2f, 0.4f, 0.5f, 0,   // 12dB, brassy
            /*fltEnv*/ 0.005f, 0.35f, 0.5f, 0.3f,
            /*ampEnv*/ 0.003f, 0.2f, 0.85f, 0.3f,
            /*lfo*/ 5.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 005 — Moog Lead (fat mono-style lead, 24dB filter, glide)
        //       Classic prog-rock singing lead
        // ================================================================
        presets.push_back({
            "007 Moog Lead",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 2, 0.6f, 0, 5.0f,     // Square, slight detune
            /*noise*/ 0.0f,
            /*filter*/ 5500.0f, 0.3f, 0.45f, 0.7f, 1,  // 24dB, resonant
            /*fltEnv*/ 0.01f, 0.2f, 0.4f, 0.25f,
            /*ampEnv*/ 0.005f, 0.15f, 0.75f, 0.2f,
            /*lfo*/ 5.5f, 0.08f, 0,        // subtle vibrato
            /*glide*/ 0.06f,                // short portamento
            /*master*/ 0.8f
        });

        // ================================================================
        // 006 — OB Strings (lush slow string pad)
        //       12dB, heavy detune, slow everything
        // ================================================================
        presets.push_back({
            "008 OB Strings",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.9f, 0, 12.0f,    // Saw, heavy detune for chorus
            /*noise*/ 0.02f,                // hint of noise for air
            /*filter*/ 4500.0f, 0.05f, 0.2f, 0.5f, 0,  // 12dB, warm
            /*fltEnv*/ 0.3f, 1.0f, 0.75f, 1.2f,     // slow
            /*ampEnv*/ 0.4f, 0.8f, 0.85f, 1.5f,      // slow pad
            /*lfo*/ 4.0f, 0.04f, 0,         // gentle vibrato
            /*glide*/ 0.0f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 007 — Prophet Lead (Prophet-5 style, saw+pulse, cutting)
        //       24dB but more open, key tracking
        // ================================================================
        presets.push_back({
            "009 Prophet Lead",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 2, 0.7f, 0, 3.0f,     // Square
            /*noise*/ 0.0f,
            /*filter*/ 9000.0f, 0.25f, 0.3f, 0.7f, 1,  // 24dB
            /*fltEnv*/ 0.01f, 0.25f, 0.3f, 0.2f,
            /*ampEnv*/ 0.005f, 0.2f, 0.8f, 0.25f,
            /*lfo*/ 5.8f, 0.06f, 0,        // vibrato
            /*glide*/ 0.04f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 008 — Analog Bass (fat Moog-style bass)
        //       24dB, osc2 octave down
        // ================================================================
        presets.push_back({
            "010 Analog Bass",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.8f, -12, 3.0f,   // Saw, one octave down
            /*noise*/ 0.0f,
            /*filter*/ 2500.0f, 0.25f, 0.5f, 0.3f, 1,  // 24dB, low cutoff
            /*fltEnv*/ 0.003f, 0.3f, 0.2f, 0.15f,
            /*ampEnv*/ 0.003f, 0.2f, 0.6f, 0.1f,
            /*lfo*/ 3.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.85f
        });

        // ================================================================
        // 009 — KR Arp Sequence (Knight Rider main hook — Jupiter-8)
        //       Ralph Grierson's arpeggiated sequence
        //       Roland Jupiter-8: brilliant, clean, fast arpeggio response
        // ================================================================
        presets.push_back({
            "011 KR Arp Seq",
            /*osc1*/ 1, 1.0f,              // Saw (Jupiter-8 primary)
            /*osc2*/ 2, 0.35f, 0, 3.0f,    // Square, low mix for body
            /*noise*/ 0.0f,
            /*filter*/ 13000.0f, 0.12f, 0.4f, 0.8f, 0,  // 12dB, very bright
            /*fltEnv*/ 0.001f, 0.1f, 0.5f, 0.08f,  // instant snap
            /*ampEnv*/ 0.001f, 0.06f, 0.9f, 0.08f,  // tight, fast for arpeggios
            /*lfo*/ 6.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.85f
        });

        // ================================================================
        // 010 — KR Brass (Knight Rider brassy textures — OB-X)
        //       Ian Underwood's thick brassy Oberheim layers
        //       Oberheim OB-X: dual saw, 12dB, aggressive filter env
        // ================================================================
        presets.push_back({
            "012 KR Brass",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.95f, 0, 6.0f,    // Saw, wider detune for thickness
            /*noise*/ 0.0f,
            /*filter*/ 9000.0f, 0.22f, 0.55f, 0.5f, 0,  // 12dB, brassy, strong env
            /*fltEnv*/ 0.003f, 0.25f, 0.4f, 0.2f,
            /*ampEnv*/ 0.002f, 0.3f, 0.8f, 0.25f,
            /*lfo*/ 5.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.85f
        });

        // ================================================================
        // 011 — KR Chords (Knight Rider chord layers — Prophet-5)
        //       Warm analog poly chord pads, versatile
        //       Sequential Prophet-5: warm saw, medium filter, lush
        // ================================================================
        presets.push_back({
            "013 KR Chords",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 1, 0.75f, 0, 8.0f,    // Saw, chorus-like detune
            /*noise*/ 0.01f,                // hint of air
            /*filter*/ 6500.0f, 0.1f, 0.3f, 0.6f, 1,  // 24dB, warm
            /*fltEnv*/ 0.05f, 0.4f, 0.6f, 0.5f,
            /*ampEnv*/ 0.03f, 0.3f, 0.85f, 0.6f,
            /*lfo*/ 4.5f, 0.04f, 2,        // gentle motion on both
            /*glide*/ 0.0f,
            /*master*/ 0.8f
        });

        // ================================================================
        // 012 — KR Deep Bass (Knight Rider bassline — Minimoog)
        //       Deep, tight Moog bass for the rhythmic foundation
        //       Moog Minimoog: 24dB ladder, osc2 octave down, punchy
        // ================================================================
        presets.push_back({
            "014 KR Deep Bass",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 2, 0.9f, -12, 2.0f,   // Square, octave down
            /*noise*/ 0.0f,
            /*filter*/ 1600.0f, 0.3f, 0.7f, 0.2f, 1,  // 24dB Moog, low
            /*fltEnv*/ 0.001f, 0.12f, 0.1f, 0.06f,  // instant zap, short
            /*ampEnv*/ 0.001f, 0.12f, 0.0f, 0.06f,  // percussive
            /*lfo*/ 3.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.9f
        });

        // ================================================================
        // 013 — KR Lead (Knight Rider lead melody — Jupiter-8/Juno)
        //       Bright, cutting lead for the melody lines
        //       Clean with slight vibrato, very responsive
        // ================================================================
        presets.push_back({
            "015 KR Lead",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 2, 0.6f, 0, 4.0f,     // Square, moderate mix
            /*noise*/ 0.0f,
            /*filter*/ 11000.0f, 0.18f, 0.45f, 0.7f, 0,  // 12dB, bright
            /*fltEnv*/ 0.003f, 0.15f, 0.45f, 0.15f,
            /*ampEnv*/ 0.003f, 0.1f, 0.9f, 0.15f,
            /*lfo*/ 5.5f, 0.05f, 0,        // subtle vibrato
            /*glide*/ 0.03f,               // short glide for expressiveness
            /*master*/ 0.85f
        });

        // ================================================================
        // 014 — KR Arp Odyssey (aggressive arp — Arp Odyssey style)
        //       Nasal, biting, resonant — the edgier side of KR
        //       Arp Odyssey: 12dB, high resonance, bright
        // ================================================================
        presets.push_back({
            "016 KR Odyssey",
            /*osc1*/ 1, 1.0f,              // Saw
            /*osc2*/ 2, 0.8f, 0, 2.0f,     // Square, tight
            /*noise*/ 0.0f,
            /*filter*/ 8000.0f, 0.45f, 0.5f, 0.7f, 0,  // 12dB, resonant bite
            /*fltEnv*/ 0.001f, 0.08f, 0.3f, 0.06f,  // very snappy
            /*ampEnv*/ 0.001f, 0.06f, 0.85f, 0.06f,
            /*lfo*/ 5.0f, 0.0f, 0,
            /*glide*/ 0.0f,
            /*master*/ 0.82f
        });
    }
};
