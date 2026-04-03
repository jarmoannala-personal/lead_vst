#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "MidiLearnManager.h"
#include "SettingsManager.h"
#include "PresetManager.h"
#include "Arpeggiator.h"

class SynthVoice;

class LeadSynthProcessor : public juce::AudioProcessor
{
public:
    LeadSynthProcessor();
    ~LeadSynthProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LeadSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::Synthesiser& getSynth() { return synth; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    MidiLearnManager& getMidiLearn() { return midiLearn; }
    SettingsManager& getSettings() { return settings; }

    // Called by the editor when it detects the active MIDI device
    void onMidiDeviceChanged(const juce::String& newDeviceName);
    juce::String getCurrentMidiDeviceName() const { return currentMidiDevice; }

    // Presets
    PresetManager& getPresetManager() { return presetManager; }
    juce::String getPresetName() const { return currentPresetName; }
    void setPresetName(const juce::String& name) { currentPresetName = name; }
    bool isPresetModified() const { return presetModified; }
    void markPresetModified() { presetModified = true; }
    void clearPresetModified() { presetModified = false; }
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    void setCurrentPresetIndex(int idx) { currentPresetIndex = idx; }

    void loadPreset(int index);
    void loadNextPreset();
    void loadPrevPreset();

    // Oscilloscope buffers (lock-free ring buffer for UI, stereo)
    static constexpr int scopeBufferSize = 512;
    const std::array<float, scopeBufferSize>& getScopeBufferL() const { return scopeBufferL; }
    const std::array<float, scopeBufferSize>& getScopeBufferR() const { return scopeBufferR; }
    int getScopeWritePos() const { return scopeWritePos.load(); }

    // On-screen keyboard state
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    // Arpeggiator
    Arpeggiator& getArpeggiator() { return arpeggiator; }

    // Voice count
    int getActiveVoiceCount() const { return activeVoiceCount.load(); }

    // CPU load
    float getCpuLoad() const { return cpuLoad.load(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void pushParamsToVoices();

    juce::AudioProcessorValueTreeState apvts;
    juce::Synthesiser synth;
    MidiLearnManager midiLearn;
    SettingsManager settings;
    juce::String currentMidiDevice;
    PresetManager presetManager;
    juce::String currentPresetName { "001 Analog Lead" };
    int currentPresetIndex = 0;
    bool presetModified = false;

    juce::MidiKeyboardState keyboardState;
    Arpeggiator arpeggiator;

    // Scope (stereo)
    std::array<float, scopeBufferSize> scopeBufferL {};
    std::array<float, scopeBufferSize> scopeBufferR {};
    std::atomic<int> scopeWritePos { 0 };
    int scopeDownsampleCounter = 0;

    // Monitoring
    std::atomic<int> activeVoiceCount { 0 };
    std::atomic<float> cpuLoad { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeadSynthProcessor)
};
