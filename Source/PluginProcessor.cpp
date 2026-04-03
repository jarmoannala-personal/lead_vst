#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthVoice.h"

LeadSynthProcessor::LeadSynthProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    synth.addSound(new SynthSound());

    for (int i = 0; i < 8; ++i)
        synth.addVoice(new SynthVoice());
}

juce::AudioProcessorValueTreeState::ParameterLayout LeadSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- Oscillator 1 ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("osc1Wave", 1), "Osc 1 Wave",
        juce::StringArray{ "Sine", "Saw", "Square", "Triangle" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc1Level", 1), "Osc 1 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    // --- Oscillator 2 ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("osc2Wave", 1), "Osc 2 Wave",
        juce::StringArray{ "Sine", "Saw", "Square", "Triangle" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Level", 1), "Osc 2 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("osc2Coarse", 1), "Osc 2 Coarse",
        -24, 24, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Fine", 1), "Osc 2 Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f), 7.0f)); // slight detune default

    // --- Noise ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("noiseLevel", 1), "Noise Level",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // --- Filter ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterCutoff", 1), "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 8000.0f)); // skewed

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterReso", 1), "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterEnvAmt", 1), "Filter Env Amount",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterKeyTrack", 1), "Filter Key Track",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("filterSlope", 1), "Filter Slope",
        juce::StringArray{ "12 dB", "24 dB" }, 1));

    // --- Filter Envelope ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fltAttack", 1), "Filter Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f), 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fltDecay", 1), "Filter Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fltSustain", 1), "Filter Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fltRelease", 1), "Filter Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f), 0.3f));

    // --- Amp Envelope ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampAttack", 1), "Amp Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f), 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampDecay", 1), "Amp Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampSustain", 1), "Amp Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampRelease", 1), "Amp Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.4f), 0.3f));

    // --- LFO ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("lfoRate", 1), "LFO Rate",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.0f, 0.5f), 3.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("lfoDepth", 1), "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("lfoDest", 1), "LFO Destination",
        juce::StringArray{ "Pitch", "Filter", "Both" }, 0));

    // --- Glide ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("glideTime", 1), "Glide Time",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.0f, 0.5f), 0.0f));

    // --- Master ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("masterGain", 1), "Master Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    // --- Arpeggiator ---
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("arpEnabled", 1), "Arp Enabled", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("arpTempo", 1), "Arp Tempo",
        juce::NormalisableRange<float>(30.0f, 300.0f, 1.0f), 120.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("arpPattern", 1), "Arp Pattern",
        juce::StringArray{ "Up", "Down", "Up/Down", "Random" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("arpOctaves", 1), "Arp Octaves", 1, 4, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("arpGate", 1), "Arp Gate",
        juce::NormalisableRange<float>(0.05f, 1.0f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("arpLatch", 1), "Arp Latch", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("arpSync", 1), "Arp Sync",
        juce::StringArray{ "Internal", "Host" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("arpRate", 1), "Arp Rate",
        juce::StringArray{ "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3)); // default 1/16

    return { params.begin(), params.end() };
}

void LeadSynthProcessor::pushParamsToVoices()
{
    SynthVoice::VoiceParams vp;

    vp.osc1Waveform    = static_cast<int>(*apvts.getRawParameterValue("osc1Wave"));
    vp.osc1Level       = *apvts.getRawParameterValue("osc1Level");
    vp.osc2Waveform    = static_cast<int>(*apvts.getRawParameterValue("osc2Wave"));
    vp.osc2Level       = *apvts.getRawParameterValue("osc2Level");
    vp.osc2Coarse      = static_cast<int>(*apvts.getRawParameterValue("osc2Coarse"));
    vp.osc2Fine        = *apvts.getRawParameterValue("osc2Fine");
    vp.noiseLevel      = *apvts.getRawParameterValue("noiseLevel");

    vp.filterCutoff    = *apvts.getRawParameterValue("filterCutoff");
    vp.filterResonance = *apvts.getRawParameterValue("filterReso");
    vp.filterEnvAmount = *apvts.getRawParameterValue("filterEnvAmt");
    vp.filterKeyTracking = *apvts.getRawParameterValue("filterKeyTrack");
    vp.filterSlope     = static_cast<int>(*apvts.getRawParameterValue("filterSlope"));

    vp.filterAttack    = *apvts.getRawParameterValue("fltAttack");
    vp.filterDecay     = *apvts.getRawParameterValue("fltDecay");
    vp.filterSustain   = *apvts.getRawParameterValue("fltSustain");
    vp.filterRelease   = *apvts.getRawParameterValue("fltRelease");

    vp.ampAttack       = *apvts.getRawParameterValue("ampAttack");
    vp.ampDecay        = *apvts.getRawParameterValue("ampDecay");
    vp.ampSustain      = *apvts.getRawParameterValue("ampSustain");
    vp.ampRelease      = *apvts.getRawParameterValue("ampRelease");

    vp.lfoRate         = *apvts.getRawParameterValue("lfoRate");
    vp.lfoDepth        = *apvts.getRawParameterValue("lfoDepth");
    vp.lfoDestination  = static_cast<int>(*apvts.getRawParameterValue("lfoDest"));

    vp.glideTime       = *apvts.getRawParameterValue("glideTime");

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->setParams(vp);
}

void LeadSynthProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    arpeggiator.setSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->prepareToPlay(sampleRate);
}

void LeadSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (getSampleRate() <= 0.0) return;

    // Inject on-screen keyboard MIDI into the buffer
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Process MIDI CC messages through learn manager
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isController())
            midiLearn.processMidiCC(msg.getControllerNumber(),
                                    msg.getControllerValue(), apvts);
    }

    // Update arpeggiator parameters
    arpeggiator.setEnabled(*apvts.getRawParameterValue("arpEnabled") > 0.5f,
                           &midiMessages, 0);
    arpeggiator.setTempoBPM(*apvts.getRawParameterValue("arpTempo"));
    arpeggiator.setPattern(static_cast<Arpeggiator::Pattern>(
        static_cast<int>(*apvts.getRawParameterValue("arpPattern"))));
    arpeggiator.setOctaveRange(static_cast<int>(*apvts.getRawParameterValue("arpOctaves")));
    arpeggiator.setGateLength(*apvts.getRawParameterValue("arpGate"));
    arpeggiator.setLatch(*apvts.getRawParameterValue("arpLatch") > 0.5f);
    arpeggiator.setSyncMode(static_cast<Arpeggiator::SyncMode>(
        static_cast<int>(*apvts.getRawParameterValue("arpSync"))));
    arpeggiator.setRateDiv(static_cast<Arpeggiator::RateDiv>(
        static_cast<int>(*apvts.getRawParameterValue("arpRate"))));

    // Process arpeggiator (replaces held notes with arpeggiated sequence)
    juce::AudioPlayHead::PositionInfo posInfo;
    if (auto* playHead = getPlayHead())
    {
        auto pos = playHead->getPosition();
        if (pos.hasValue())
            posInfo = *pos;
    }
    arpeggiator.process(midiMessages, buffer.getNumSamples(), &posInfo);

    // Push latest parameter values to all voices
    pushParamsToVoices();

    auto startTime = juce::Time::getHighResolutionTicks();

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master gain
    float gain = *apvts.getRawParameterValue("masterGain");
    buffer.applyGain(gain);

    // Feed stereo scope buffers
    auto* dataL = buffer.getReadPointer(0);
    auto* dataR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : dataL;
    int numSamples = buffer.getNumSamples();
    int downsampleRatio = std::max(1, static_cast<int>(getSampleRate()) / (scopeBufferSize * 30));

    for (int i = 0; i < numSamples; ++i)
    {
        if (++scopeDownsampleCounter >= downsampleRatio)
        {
            scopeDownsampleCounter = 0;
            int pos = scopeWritePos.load();
            scopeBufferL[static_cast<size_t>(pos)] = dataL[i];
            scopeBufferR[static_cast<size_t>(pos)] = dataR[i];
            scopeWritePos.store((pos + 1) % scopeBufferSize);
        }
    }

    // Count active voices
    int voiceCount = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (synth.getVoice(i)->isVoiceActive())
            ++voiceCount;
    activeVoiceCount.store(voiceCount);

    // CPU load estimate
    auto endTime = juce::Time::getHighResolutionTicks();
    double elapsed = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    double blockDuration = static_cast<double>(numSamples) / getSampleRate();
    cpuLoad.store(static_cast<float>(elapsed / blockDuration * 100.0));
}

void LeadSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Append MIDI learn mappings
    if (auto midiLearnXml = midiLearn.toXml())
        xml->addChildElement(midiLearnXml.release());

    // Save preset info
    auto* presetXml = xml->createNewChildElement("PresetInfo");
    presetXml->setAttribute("index", currentPresetIndex);
    presetXml->setAttribute("name", currentPresetName);

    copyXmlToBinary(*xml, destData);
}

void LeadSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        // Restore MIDI learn mappings
        if (auto* midiLearnXml = xml->getChildByName("MidiLearnMappings"))
        {
            midiLearn.fromXml(midiLearnXml);
            xml->removeChildElement(midiLearnXml, true);
        }

        // Restore preset info
        if (auto* presetXml = xml->getChildByName("PresetInfo"))
        {
            currentPresetIndex = presetXml->getIntAttribute("index", 0);
            currentPresetName = presetXml->getStringAttribute("name", "001 Analog Lead");
            xml->removeChildElement(presetXml, true);
        }

        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

void LeadSynthProcessor::loadPreset(int index)
{
    index = std::clamp(index, 0, presetManager.getNumPresets() - 1);
    auto& preset = presetManager.getPreset(index);
    presetManager.applyPreset(preset, apvts);
    currentPresetIndex = index;
    currentPresetName = preset.name;
    presetModified = false;
}

void LeadSynthProcessor::loadNextPreset()
{
    int next = (currentPresetIndex + 1) % presetManager.getNumPresets();
    loadPreset(next);
}

void LeadSynthProcessor::loadPrevPreset()
{
    int prev = currentPresetIndex - 1;
    if (prev < 0) prev = presetManager.getNumPresets() - 1;
    loadPreset(prev);
}

void LeadSynthProcessor::onMidiDeviceChanged(const juce::String& newDeviceName)
{
    if (newDeviceName == currentMidiDevice)
        return;

    // Save current device's mappings before switching
    if (currentMidiDevice.isNotEmpty())
        settings.saveMappingsForDevice(currentMidiDevice, midiLearn);

    currentMidiDevice = newDeviceName;

    // Load mappings for the new device (if any saved)
    if (newDeviceName.isNotEmpty())
        settings.loadMappingsForDevice(newDeviceName, midiLearn);
}

juce::AudioProcessorEditor* LeadSynthProcessor::createEditor()
{
    return new LeadSynthEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LeadSynthProcessor();
}
