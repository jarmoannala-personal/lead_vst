#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

// Lock-free MIDI CC-to-parameter mapping manager.
// Audio thread writes via processMidiCC (lock-free).
// UI thread reads/writes mappings via other methods (called only from message thread).
class MidiLearnManager
{
public:
    // --- UI thread methods ---

    // Called from the editor when user clicks a knob in learn mode
    void setLearningParam(const juce::String& paramId)
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        learningParamId = paramId;
        lastLearnedCC.store(-1);
    }

    bool isLearning() const { return learningParamId.isNotEmpty(); }
    juce::String getLearningParamId() const { return learningParamId; }
    int getLastLearnedCC() const { return lastLearnedCC.load(); }

    // Get the CC number mapped to a parameter (or -1)
    int getCCForParam(const juce::String& paramId) const
    {
        for (int cc = 0; cc < 128; ++cc)
            if (mappings[static_cast<size_t>(cc)].paramId == paramId)
                return cc;
        return -1;
    }

    // Remove mapping for a specific parameter (UI thread)
    void removeMappingForParam(const juce::String& paramId)
    {
        for (int cc = 0; cc < 128; ++cc)
            if (mappings[static_cast<size_t>(cc)].paramId == paramId)
                mappings[static_cast<size_t>(cc)].clear();
        publishToAudioThread();
    }

    // Clear all mappings (UI thread)
    void clearAllMappings()
    {
        for (auto& m : mappings) m.clear();
        learningParamId.clear();
        lastLearnedCC.store(-1);
        publishToAudioThread();
    }

    // Cancel learn mode without assigning
    void cancelLearn()
    {
        learningParamId.clear();
        lastLearnedCC.store(-1);
    }

    // --- Audio thread method ---

    // Called from processBlock. Lock-free: reads from atomic snapshot,
    // writes only to atomic flags for learn completion.
    void processMidiCC(int ccNumber, int ccValue, juce::AudioProcessorValueTreeState& apvts)
    {
        if (ccNumber < 0 || ccNumber >= 128) return;

        // Check if we're in learn mode (atomic read of flag set by UI)
        if (learningActive.load())
        {
            // Store the CC that was received — UI will pick this up
            pendingLearnCC.store(ccNumber);
            pendingLearnValue.store(ccValue);
            learningActive.store(false);
            lastLearnedCC.store(ccNumber);
        }

        // Apply existing mapping from the audio-thread snapshot
        auto& paramId = audioMappings[static_cast<size_t>(ccNumber)];
        if (paramId.isNotEmpty())
        {
            if (auto* param = apvts.getParameter(paramId))
            {
                float normalized = static_cast<float>(ccValue) / 127.0f;
                param->setValueNotifyingHost(normalized);
            }
        }
    }

    // Called from UI timer to complete learn assignments and sync state
    void uiTimerUpdate()
    {
        // Complete any pending learn assignment
        int cc = pendingLearnCC.exchange(-1);
        if (cc >= 0 && learningParamId.isNotEmpty())
        {
            // Remove any existing mapping for this CC
            mappings[static_cast<size_t>(cc)].clear();

            // Remove any existing CC mapped to this param
            for (auto& m : mappings)
                if (m.paramId == learningParamId)
                    m.clear();

            mappings[static_cast<size_t>(cc)].paramId = learningParamId;
            learningParamId.clear();
            publishToAudioThread();
        }

        // If UI set a learning param, activate the audio-thread flag
        if (learningParamId.isNotEmpty())
            learningActive.store(true);
    }

    // --- Serialization (UI thread) ---

    std::unique_ptr<juce::XmlElement> toXml() const
    {
        auto xml = std::make_unique<juce::XmlElement>("MidiLearnMappings");
        for (int cc = 0; cc < 128; ++cc)
        {
            if (mappings[static_cast<size_t>(cc)].paramId.isNotEmpty())
            {
                auto* mapping = xml->createNewChildElement("Mapping");
                mapping->setAttribute("cc", cc);
                mapping->setAttribute("param", mappings[static_cast<size_t>(cc)].paramId);
            }
        }
        return xml;
    }

    void fromXml(const juce::XmlElement* xml)
    {
        for (auto& m : mappings) m.clear();
        if (xml == nullptr) return;

        for (auto* mapping : xml->getChildIterator())
        {
            if (mapping->hasTagName("Mapping"))
            {
                int cc = mapping->getIntAttribute("cc", -1);
                juce::String paramId = mapping->getStringAttribute("param");
                if (cc >= 0 && cc < 128 && paramId.isNotEmpty())
                    mappings[static_cast<size_t>(cc)].paramId = paramId;
            }
        }
        publishToAudioThread();
    }

private:
    // Publish current mappings to audio thread snapshot
    void publishToAudioThread()
    {
        for (int cc = 0; cc < 128; ++cc)
            audioMappings[static_cast<size_t>(cc)] = mappings[static_cast<size_t>(cc)].paramId;
    }

    struct Mapping { juce::String paramId; void clear() { paramId.clear(); } };

    // UI-thread owned mappings
    std::array<Mapping, 128> mappings;

    // Audio-thread readable snapshot (written by UI via publishToAudioThread)
    // juce::String is ref-counted and safe for this use pattern
    // (UI writes, audio reads — no concurrent writes)
    std::array<juce::String, 128> audioMappings;

    // Learn workflow (lock-free communication)
    juce::String learningParamId;               // UI thread only
    std::atomic<bool> learningActive { false };  // UI sets, audio reads/clears
    std::atomic<int> pendingLearnCC { -1 };      // audio sets, UI reads
    std::atomic<int> pendingLearnValue { -1 };
    std::atomic<int> lastLearnedCC { -1 };       // for UI polling
};
