#pragma once
#include "PluginProcessor.h"
#include "SynthLookAndFeel.h"
#include <juce_audio_utils/juce_audio_utils.h>

class LeadSynthEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit LeadSynthEditor(LeadSynthProcessor&);
    ~LeadSynthEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    LeadSynthProcessor& synthProcessor;
    SynthLookAndFeel synthLookAndFeel;

    static constexpr int designWidth  = 1250;
    static constexpr int designHeight = 540;
    static constexpr int keyboardHeight = 55;
    static constexpr int woodPanelWidth = 30;
    static constexpr int headerHeight = 40;
    static constexpr int sectionHeaderH = 22;

    float scaleFactor = 1.0f;
    float offsetX = 0.0f;  // horizontal centering offset for kiosk mode
    float offsetY = 0.0f;

    // --- MIDI Learn ---
    bool midiLearnMode = false;
    juce::String selectedLearnParam;
    juce::TextButton midiLearnButton { "MIDI LEARN" };
    juce::TextButton clearMappingsButton { "CLR" };
    juce::TextButton panicButton { "PANIC" };

    void timerCallback() override;
    void onKnobClickedInLearnMode(const juce::String& paramId);
    void detectMidiDevice();
    void saveMappingsForCurrentDevice();

    juce::String lastDetectedDevice;

    // --- Preset navigation ---
    juce::TextButton presetPrev { "<" };
    juce::TextButton presetNext { ">" };
    juce::TextButton presetNameButton;
    void updatePresetDisplay();
    void showAboutDialog();
    void showPresetBrowser();
    void toggleFullscreen();

    // --- Visualizations ---
    void drawADSRCurve(juce::Graphics& g, juce::Rectangle<int> area,
                       float a, float d, float s, float r);
    void drawOscilloscope(juce::Graphics& g, juce::Rectangle<int> area);
    void drawFooter(juce::Graphics& g, juce::Rectangle<int> area);

    // --- Knob with label ---
    struct KnobWithLabel
    {
        juce::Slider slider;
        juce::Label label;
        juce::Label ccLabel;
        std::unique_ptr<SliderAttachment> attachment;
        juce::String paramId;

        void init(LeadSynthEditor& editor, const juce::String& pid,
                  const juce::String& labelText, juce::AudioProcessorValueTreeState& apvts)
        {
            paramId = pid;
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
           #if JUCE_DEBUG
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
           #else
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
           #endif
            slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffcccccc));
            slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
            editor.addAndMakeVisible(slider);
            slider.addMouseListener(&editor, false);

            label.setText(labelText, juce::dontSendNotification);
            label.setFont(juce::FontOptions(9.5f));
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
            editor.addAndMakeVisible(label);

            ccLabel.setFont(juce::FontOptions(8.0f, juce::Font::bold));
            ccLabel.setJustificationType(juce::Justification::centred);
            ccLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcc7700));
            ccLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0x00000000));
            editor.addAndMakeVisible(ccLabel);

            attachment = std::make_unique<SliderAttachment>(apvts, pid, slider);
        }
    };

    struct ComboWithLabel
    {
        juce::ComboBox combo;
        juce::Label label;
        std::unique_ptr<ComboAttachment> attachment;

        void init(LeadSynthEditor& editor, const juce::String& paramId,
                  const juce::String& labelText, const juce::StringArray& items,
                  juce::AudioProcessorValueTreeState& apvts)
        {
            combo.addItemList(items, 1);
            editor.addAndMakeVisible(combo);
            label.setText(labelText, juce::dontSendNotification);
            label.setFont(juce::FontOptions(9.5f));
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
            editor.addAndMakeVisible(label);
            attachment = std::make_unique<ComboAttachment>(apvts, paramId, combo);
        }
    };

    std::vector<KnobWithLabel*> allKnobs;

    void mouseDown(const juce::MouseEvent& event) override;
    void updateCCLabels();
    void updateLearnHighlights();

    ComboWithLabel osc1Wave;    KnobWithLabel osc1Level;
    ComboWithLabel osc2Wave;    KnobWithLabel osc2Level;
    KnobWithLabel osc2Coarse;   KnobWithLabel osc2Fine;
    KnobWithLabel noiseLevel;
    KnobWithLabel filterCutoff;  KnobWithLabel filterReso;
    KnobWithLabel filterEnvAmt;  KnobWithLabel filterKeyTrack;
    ComboWithLabel filterSlope;
    KnobWithLabel fltA, fltD, fltS, fltR;
    KnobWithLabel ampA, ampD, ampS, ampR;
    KnobWithLabel lfoRate, lfoDepth;
    ComboWithLabel lfoDest;
    KnobWithLabel glideTime;
    KnobWithLabel masterGain;

    // --- Arpeggiator ---
    juce::TextButton arpToggle { "ARP" };
    juce::TextButton arpLatchToggle { "LATCH" };
    // Tempo as editable number box
    juce::Slider arpTempoSlider;
    juce::Label arpTempoLabel;
    std::unique_ptr<SliderAttachment> arpTempoAttachment;
    KnobWithLabel arpGate;
    ComboWithLabel arpPattern;
    ComboWithLabel arpOctaves;
    ComboWithLabel arpSync;
    ComboWithLabel arpRate;
    std::unique_ptr<juce::ButtonParameterAttachment> arpEnabledAttachment;
    std::unique_ptr<juce::ButtonParameterAttachment> arpLatchAttachment;

   #if JUCE_DEBUG
    juce::TextButton testButton { "TEST" };
    bool noteIsOn = false;
   #endif

    // --- On-screen keyboard ---
    juce::MidiKeyboardComponent keyboard;
    int keyboardBaseOctave = 4;
    void setupKeyboardMapping();
    void updateOctave(int delta);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeadSynthEditor)
};
