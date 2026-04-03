#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "MidiLearnManager.h"

// Persists per-device MIDI learn mappings to disk.
// Settings file: ~/Library/Application Support/LeadSynth/LeadSynth.xml
class SettingsManager
{
public:
    SettingsManager()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "LeadSynth";
        options.folderName = "LeadSynth";
        options.filenameSuffix = ".xml";
        options.osxLibrarySubFolder = "Application Support";

        propsFile = std::make_unique<juce::PropertiesFile>(options);
    }

    // Save MIDI learn mappings for a specific device
    void saveMappingsForDevice(const juce::String& deviceName,
                               const MidiLearnManager& midiLearn)
    {
        if (deviceName.isEmpty())
            return;

        auto mappingsXml = midiLearn.toXml();
        propsFile->setValue("midiLearn_" + sanitiseKey(deviceName), mappingsXml.get());
        propsFile->setValue("lastMidiDevice", deviceName);
        propsFile->saveIfNeeded();
    }

    // Load MIDI learn mappings for a specific device
    bool loadMappingsForDevice(const juce::String& deviceName,
                                MidiLearnManager& midiLearn)
    {
        if (deviceName.isEmpty())
            return false;

        auto xml = propsFile->getXmlValue("midiLearn_" + sanitiseKey(deviceName));
        if (xml != nullptr)
        {
            midiLearn.fromXml(xml.get());
            return true;
        }
        return false;
    }

    // Get the last used MIDI device name
    juce::String getLastMidiDevice() const
    {
        return propsFile->getValue("lastMidiDevice", "");
    }

    // Check if we have saved mappings for a device
    bool hasMappingsForDevice(const juce::String& deviceName) const
    {
        if (deviceName.isEmpty())
            return false;
        return propsFile->containsKey("midiLearn_" + sanitiseKey(deviceName));
    }

    // Delete mappings for a device
    void deleteMappingsForDevice(const juce::String& deviceName)
    {
        if (deviceName.isEmpty())
            return;
        propsFile->removeValue("midiLearn_" + sanitiseKey(deviceName));
        propsFile->saveIfNeeded();
    }

private:
    std::unique_ptr<juce::PropertiesFile> propsFile;

    // Sanitise device name for use as a properties key
    static juce::String sanitiseKey(const juce::String& name)
    {
        return name.replaceCharacters(" /\\:*?\"<>|", "__________");
    }
};
