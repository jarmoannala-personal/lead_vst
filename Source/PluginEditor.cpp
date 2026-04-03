#include "PluginEditor.h"

static const juce::StringArray waveformNames { "Sine", "Saw", "Square", "Triangle" };
static const juce::StringArray lfoDestNames  { "Pitch", "Filter", "Both" };
static const juce::StringArray slopeNames    { "12dB", "24dB" };

LeadSynthEditor::LeadSynthEditor(LeadSynthProcessor& p)
    : AudioProcessorEditor(p), synthProcessor(p),
      keyboard(p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&synthLookAndFeel);
    setSize(designWidth, designHeight);
    setResizable(true, true);
    setResizeLimits(770, 336, 2200, 960);
    getConstrainer()->setFixedAspectRatio(
        static_cast<double>(designWidth) / static_cast<double>(designHeight));
    setWantsKeyboardFocus(true);

    auto& apvts = synthProcessor.getAPVTS();

    osc1Wave.init(*this, "osc1Wave", "WAVE", waveformNames, apvts);
    osc1Level.init(*this, "osc1Level", "LEVEL", apvts);
    osc2Wave.init(*this, "osc2Wave", "WAVE", waveformNames, apvts);
    osc2Level.init(*this, "osc2Level", "LEVEL", apvts);
    osc2Coarse.init(*this, "osc2Coarse", "COARSE", apvts);
    osc2Fine.init(*this, "osc2Fine", "FINE", apvts);
    noiseLevel.init(*this, "noiseLevel", "NOISE", apvts);
    filterCutoff.init(*this, "filterCutoff", "CUTOFF", apvts);
    filterReso.init(*this, "filterReso", "RESO", apvts);
    filterEnvAmt.init(*this, "filterEnvAmt", "ENV AMT", apvts);
    filterKeyTrack.init(*this, "filterKeyTrack", "KEY TRK", apvts);
    filterSlope.init(*this, "filterSlope", "SLOPE", slopeNames, apvts);
    fltA.init(*this, "fltAttack", "A", apvts);  fltD.init(*this, "fltDecay", "D", apvts);
    fltS.init(*this, "fltSustain","S", apvts);   fltR.init(*this, "fltRelease","R", apvts);
    ampA.init(*this, "ampAttack", "A", apvts);  ampD.init(*this, "ampDecay", "D", apvts);
    ampS.init(*this, "ampSustain","S", apvts);   ampR.init(*this, "ampRelease","R", apvts);
    lfoRate.init(*this, "lfoRate", "RATE", apvts);
    lfoDepth.init(*this, "lfoDepth", "DEPTH", apvts);
    lfoDest.init(*this, "lfoDest", "DEST", lfoDestNames, apvts);
    glideTime.init(*this, "glideTime", "GLIDE", apvts);
    masterGain.init(*this, "masterGain", "MASTER", apvts);

    // --- Arpeggiator ---
    // Tempo as editable numeric box
    arpTempoSlider.setSliderStyle(juce::Slider::LinearBar);
    arpTempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    arpTempoSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff2a2a40));
    arpTempoSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    arpTempoSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2a2a40));
    arpTempoSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff555555));
    arpTempoSlider.setTextValueSuffix(" BPM");
    arpTempoSlider.setNumDecimalPlacesToDisplay(0);
    addAndMakeVisible(arpTempoSlider);
    arpTempoAttachment = std::make_unique<SliderAttachment>(apvts, "arpTempo", arpTempoSlider);

    arpTempoLabel.setText("TEMPO", juce::dontSendNotification);
    arpTempoLabel.setFont(juce::FontOptions(9.5f));
    arpTempoLabel.setJustificationType(juce::Justification::centred);
    arpTempoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(arpTempoLabel);

    arpGate.init(*this, "arpGate", "GATE", apvts);
    arpPattern.init(*this, "arpPattern", "PATTERN", juce::StringArray{"Up","Down","Up/Dn","Rnd"}, apvts);
    arpOctaves.init(*this, "arpOctaves", "OCTAVES", juce::StringArray{"1","2","3","4"}, apvts);
    arpSync.init(*this, "arpSync", "SYNC", juce::StringArray{"Int","Host"}, apvts);
    arpRate.init(*this, "arpRate", "RATE", juce::StringArray{"1/4","1/8","1/8T","1/16","1/16T","1/32"}, apvts);

    arpToggle.setClickingTogglesState(true);
    arpToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333338));
    arpToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff227722));
    arpToggle.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    arpToggle.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(arpToggle);
    arpEnabledAttachment = std::make_unique<juce::ButtonParameterAttachment>(
        *apvts.getParameter("arpEnabled"), arpToggle);

    arpLatchToggle.setClickingTogglesState(true);
    arpLatchToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333338));
    arpLatchToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff886622));
    arpLatchToggle.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    arpLatchToggle.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(arpLatchToggle);
    arpLatchAttachment = std::make_unique<juce::ButtonParameterAttachment>(
        *apvts.getParameter("arpLatch"), arpLatchToggle);

    allKnobs = { &osc1Level, &osc2Level, &osc2Coarse, &osc2Fine,
                 &noiseLevel,
                 &filterCutoff, &filterReso, &filterEnvAmt, &filterKeyTrack,
                 &fltA, &fltD, &fltS, &fltR, &ampA, &ampD, &ampS, &ampR,
                 &lfoRate, &lfoDepth, &glideTime, &masterGain,
                 &arpGate };

    // Preset nav
    for (auto* btn : { &presetPrev, &presetNext }) {
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333338));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcc7700));
        addAndMakeVisible(btn);
    }
    presetPrev.onClick = [this]() { synthProcessor.loadPrevPreset(); repaint(); };
    presetNext.onClick = [this]() { synthProcessor.loadNextPreset(); repaint(); };

    // Preset name (clickable, transparent — opens browser)
    presetNameButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetNameButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    presetNameButton.setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    presetNameButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(presetNameButton);
    presetNameButton.onClick = [this]() { showPresetBrowser(); };

    // MIDI Learn
    midiLearnButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333338));
    midiLearnButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(midiLearnButton);
    midiLearnButton.onClick = [this]() {
        midiLearnMode = !midiLearnMode;
        if (midiLearnMode) {
            midiLearnButton.setButtonText("LEARN ON");
            midiLearnButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffcc6600));
            midiLearnButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        } else {
            midiLearnButton.setButtonText("MIDI LEARN");
            midiLearnButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333338));
            midiLearnButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));
            selectedLearnParam.clear();
            synthProcessor.getMidiLearn().cancelLearn();
            updateLearnHighlights();
        }
        repaint();
    };

    // CLR
    clearMappingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333338));
    clearMappingsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    addAndMakeVisible(clearMappingsButton);
    clearMappingsButton.onClick = [this]() {
        synthProcessor.getMidiLearn().clearAllMappings();
        saveMappingsForCurrentDevice(); updateCCLabels(); updateLearnHighlights(); repaint();
    };

    // PANIC
    panicButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff442222));
    panicButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcc6666));
    addAndMakeVisible(panicButton);
    panicButton.onClick = [this]() {
        auto& synth = synthProcessor.getSynth();
        for (int ch = 1; ch <= 16; ++ch) synth.allNotesOff(ch, true);
    };

   #if JUCE_DEBUG
    testButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d5b8a));
    testButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(testButton);
    testButton.onClick = [this]() {
        if (!noteIsOn) { synthProcessor.getSynth().noteOn(1, 60, 0.8f); noteIsOn = true;
            testButton.setButtonText("STOP"); testButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8a2d2d));
        } else { synthProcessor.getSynth().noteOff(1, 60, 0.0f, true); noteIsOn = false;
            testButton.setButtonText("TEST"); testButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d5b8a));
        }
    };
   #endif

    // On-screen keyboard
    keyboard.setAvailableRange(24, 108); // C1 to C8
    keyboard.setOctaveForMiddleC(4);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe8e0d0));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff222222));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff888888));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0x66cc7700));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x22cc7700));
    keyboard.setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, juce::Colour(0xff222228));
    keyboard.setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId, juce::Colour(0xffcc7700));
    setupKeyboardMapping();
    addAndMakeVisible(keyboard);

    detectMidiDevice(); updateCCLabels(); updateLearnHighlights();
    startTimerHz(30);
}

LeadSynthEditor::~LeadSynthEditor()
{
    stopTimer(); saveMappingsForCurrentDevice(); setLookAndFeel(nullptr);
}

void LeadSynthEditor::setupKeyboardMapping()
{
    // Clear all default mappings
    keyboard.clearKeyMappings();

    // Lower octave: Z row = white keys, A row sharps interspersed
    // Z=C  S=C#  X=D  D=D#  C=E  V=F  G=F#  B=G  H=G#  N=A  J=A#  M=B
    keyboard.setKeyPressForNote(juce::KeyPress('z'), 0);   // C
    keyboard.setKeyPressForNote(juce::KeyPress('s'), 1);   // C#
    keyboard.setKeyPressForNote(juce::KeyPress('x'), 2);   // D
    keyboard.setKeyPressForNote(juce::KeyPress('d'), 3);   // D#
    keyboard.setKeyPressForNote(juce::KeyPress('c'), 4);   // E
    keyboard.setKeyPressForNote(juce::KeyPress('v'), 5);   // F
    keyboard.setKeyPressForNote(juce::KeyPress('g'), 6);   // F#
    keyboard.setKeyPressForNote(juce::KeyPress('b'), 7);   // G
    keyboard.setKeyPressForNote(juce::KeyPress('h'), 8);   // G#
    keyboard.setKeyPressForNote(juce::KeyPress('n'), 9);   // A
    keyboard.setKeyPressForNote(juce::KeyPress('j'), 10);  // A#
    keyboard.setKeyPressForNote(juce::KeyPress('m'), 11);  // B

    // Upper octave: Q row = white keys, number row sharps
    // Q=C  2=C#  W=D  3=D#  E=E  R=F  5=F#  T=G  6=G#  Y=A  7=A#  U=B  I=C+2
    keyboard.setKeyPressForNote(juce::KeyPress('q'), 12);  // C+1
    keyboard.setKeyPressForNote(juce::KeyPress('2'), 13);  // C#+1
    keyboard.setKeyPressForNote(juce::KeyPress('w'), 14);  // D+1
    keyboard.setKeyPressForNote(juce::KeyPress('3'), 15);  // D#+1
    keyboard.setKeyPressForNote(juce::KeyPress('e'), 16);  // E+1
    keyboard.setKeyPressForNote(juce::KeyPress('r'), 17);  // F+1
    keyboard.setKeyPressForNote(juce::KeyPress('5'), 18);  // F#+1
    keyboard.setKeyPressForNote(juce::KeyPress('t'), 19);  // G+1
    keyboard.setKeyPressForNote(juce::KeyPress('6'), 20);  // G#+1
    keyboard.setKeyPressForNote(juce::KeyPress('y'), 21);  // A+1
    keyboard.setKeyPressForNote(juce::KeyPress('7'), 22);  // A#+1
    keyboard.setKeyPressForNote(juce::KeyPress('u'), 23);  // B+1
    keyboard.setKeyPressForNote(juce::KeyPress('i'), 24);  // C+2

    keyboard.setKeyPressBaseOctave(keyboardBaseOctave);
}

void LeadSynthEditor::updateOctave(int delta)
{
    int newOctave = keyboardBaseOctave + delta;
    if (newOctave < 1 || newOctave > 7) return;
    keyboardBaseOctave = newOctave;
    keyboard.setKeyPressBaseOctave(keyboardBaseOctave);
    repaint();
}

bool LeadSynthEditor::keyPressed(const juce::KeyPress& key)
{
    bool keyboardHasFocus = keyboard.hasKeyboardFocus(true);

    // Up/Down: octave shift when keyboard focused, preset switch otherwise
    if (key == juce::KeyPress::upKey)
    {
        if (keyboardHasFocus) { updateOctave(1); return true; }
        else { synthProcessor.loadPrevPreset(); repaint(); return true; }
    }
    if (key == juce::KeyPress::downKey)
    {
        if (keyboardHasFocus) { updateOctave(-1); return true; }
        else { synthProcessor.loadNextPreset(); repaint(); return true; }
    }

    // Left/Right: always preset switch
    if (key == juce::KeyPress::leftKey)
        { synthProcessor.loadPrevPreset(); repaint(); return true; }
    if (key == juce::KeyPress::rightKey)
        { synthProcessor.loadNextPreset(); repaint(); return true; }

    // Letter shortcuts only when keyboard doesn't have focus
    if (!keyboardHasFocus)
    {
        if (key == juce::KeyPress('f') || key == juce::KeyPress('F'))
            { toggleFullscreen(); return true; }
    }

    // Escape exits keyboard focus
    if (keyboardHasFocus && key == juce::KeyPress::escapeKey)
        { grabKeyboardFocus(); return true; }

    // When keyboard has focus, consume all key events to prevent OS beep
    if (keyboardHasFocus)
        return true;

    return false;
}

void LeadSynthEditor::toggleFullscreen()
{
    auto& desktop = juce::Desktop::getInstance();
    if (desktop.getKioskModeComponent() != nullptr) {
        desktop.setKioskModeComponent(nullptr);
        getConstrainer()->setFixedAspectRatio(
            static_cast<double>(designWidth) / static_cast<double>(designHeight));
    } else {
        getConstrainer()->setFixedAspectRatio(0.0);
        desktop.setKioskModeComponent(getTopLevelComponent(), false);
    }
}

void LeadSynthEditor::showAboutDialog()
{
    auto options = juce::DialogWindow::LaunchOptions();
    auto* content = new juce::Component(); content->setSize(380, 260);
    auto* text = new juce::Label("aboutText", juce::String(
        "LEADSYNTH\nVirtual Analog Synthesizer\n\n"
        "Inspired by the legendary Moog synthesizers,\n"
        "Prophet synthesizer family by Sequential,\nand Oberheim.\n\n"
        "Created by Jarmo Annala\n\nVersion 1.0.0"));
    text->setFont(juce::FontOptions(14.0f));
    text->setJustificationType(juce::Justification::centred);
    text->setColour(juce::Label::textColourId, juce::Colours::white);
    text->setBounds(0, 20, 380, 220);
    content->addAndMakeVisible(text);
    options.content.setOwned(content);
    options.dialogTitle = "About LeadSynth";
    options.dialogBackgroundColour = juce::Colour(0xff1a1a2e);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true; options.resizable = false;
    options.launchAsync();
}

void LeadSynthEditor::showPresetBrowser()
{
    auto& pm = synthProcessor.getPresetManager();
    int numPresets = pm.getNumPresets();
    int currentIdx = synthProcessor.getCurrentPresetIndex();

    // Build a table using a ListBox in a dialog
    struct PresetListModel : public juce::ListBoxModel
    {
        PresetManager& presetMgr;
        int selected;
        LeadSynthProcessor* processor;

        struct Grouping { juce::String name; juce::String group; juce::String inspiration; };
        std::vector<Grouping> info;

        PresetListModel(PresetManager& pm, int sel, LeadSynthProcessor* proc)
            : presetMgr(pm), selected(sel), processor(proc)
        {
            // Define groupings and inspirations
            for (int i = 0; i < pm.getNumPresets(); ++i)
            {
                auto& p = pm.getPreset(i);
                juce::String name = p.name;
                juce::String group, insp;

                if (name.contains("VH "))       { group = "Van Halen";    insp = "Oberheim OB-Xa"; }
                else if (name.contains("KR "))   { group = "Knight Rider"; }
                else                              { group = "Classic"; }

                if (name.contains("Moog"))        insp = "Moog";
                if (name.contains("OB "))         insp = "Oberheim";
                if (name.contains("Prophet"))     insp = "Sequential Prophet-5";
                if (name.contains("Analog Lead")) insp = "Pink Floyd / MMEB";
                if (name.contains("Analog Bass")) insp = "Moog";
                if (name.contains("KR Arp"))      insp = "Roland Jupiter-8";
                if (name.contains("KR Brass"))    insp = "Oberheim OB-X";
                if (name.contains("KR Chords"))   insp = "Sequential Prophet-5";
                if (name.contains("KR Deep"))     insp = "Moog Minimoog";
                if (name.contains("KR Lead"))     insp = "Roland Jupiter-8";
                if (name.contains("KR Odyssey"))  insp = "ARP Odyssey";
                if (name.contains("Jump"))        insp = "Oberheim OB-Xa";
                if (name.contains("Ill Wait"))    insp = "Oberheim OB-Xa";
                if (name.contains("Why Cant Lead")) insp = "Oberheim OB-Xa";
                if (name.contains("Why Cant Arp"))  insp = "Oberheim OB-Xa";

                info.push_back({ name, group, insp });
            }
        }

        int getNumRows() override { return static_cast<int>(info.size()); }

        void listBoxItemClicked(int row, const juce::MouseEvent&) override
        {
            if (row >= 0 && processor != nullptr)
                processor->loadPreset(row);
        }

        void paintListBoxItem(int row, juce::Graphics& g, int /*w*/, int h, bool isSelected) override
        {
            if (row < 0 || row >= static_cast<int>(info.size())) return;

            if (isSelected || row == selected)
                g.fillAll(juce::Colour(0xff333355));
            else if (row % 2 == 0)
                g.fillAll(juce::Colour(0xff1e1e2e));
            else
                g.fillAll(juce::Colour(0xff222238));

            auto& item = info[static_cast<size_t>(row)];

            g.setFont(juce::FontOptions(13.0f));
            g.setColour(row == selected ? juce::Colour(0xffcc7700) : juce::Colours::white);
            g.drawText(item.name, 10, 0, 200, h, juce::Justification::centredLeft);

            g.setColour(juce::Colour(0xff88aacc));
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(item.group, 220, 0, 100, h, juce::Justification::centredLeft);

            g.setColour(juce::Colour(0xff999999));
            g.drawText(item.inspiration, 330, 0, 200, h, juce::Justification::centredLeft);
        }
    };

    // Create dialog
    auto* content = new juce::Component();
    int dlgW = 560, dlgH = 40 + numPresets * 26 + 10;
    dlgH = std::min(dlgH, 500);
    content->setSize(dlgW, dlgH);

    // Header labels
    auto* hdrName = new juce::Label({}, "PATCH");
    hdrName->setFont(juce::FontOptions(11.0f, juce::Font::bold));
    hdrName->setColour(juce::Label::textColourId, juce::Colour(0xffcc7700));
    hdrName->setBounds(10, 5, 200, 18);
    content->addAndMakeVisible(hdrName);

    auto* hdrGroup = new juce::Label({}, "GROUP");
    hdrGroup->setFont(juce::FontOptions(11.0f, juce::Font::bold));
    hdrGroup->setColour(juce::Label::textColourId, juce::Colour(0xffcc7700));
    hdrGroup->setBounds(220, 5, 100, 18);
    content->addAndMakeVisible(hdrGroup);

    auto* hdrInsp = new juce::Label({}, "INSPIRATION");
    hdrInsp->setFont(juce::FontOptions(11.0f, juce::Font::bold));
    hdrInsp->setColour(juce::Label::textColourId, juce::Colour(0xffcc7700));
    hdrInsp->setBounds(330, 5, 200, 18);
    content->addAndMakeVisible(hdrInsp);

    auto* model = new PresetListModel(pm, currentIdx, &synthProcessor);
    auto* listBox = new juce::ListBox("presets", model);
    listBox->setRowHeight(24);
    listBox->setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1a1a2e));
    listBox->setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    listBox->setBounds(0, 28, dlgW, dlgH - 28);
    listBox->selectRow(currentIdx);
    listBox->scrollToEnsureRowIsOnscreen(currentIdx);
    content->addAndMakeVisible(listBox);

    auto options = juce::DialogWindow::LaunchOptions();
    options.content.setOwned(content);
    options.dialogTitle = "LeadSynth Patches";
    options.dialogBackgroundColour = juce::Colour(0xff1a1a2e);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void LeadSynthEditor::updatePresetDisplay() { repaint(); }

void LeadSynthEditor::detectMidiDevice()
{
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    juce::String deviceName;
    auto savedDevice = synthProcessor.getSettings().getLastMidiDevice();
    for (auto& input : midiInputs)
        if (input.name == savedDevice) { deviceName = input.name; break; }
    if (deviceName.isEmpty() && !midiInputs.isEmpty())
        deviceName = midiInputs[0].name;
    if (deviceName != lastDetectedDevice) {
        lastDetectedDevice = deviceName;
        synthProcessor.onMidiDeviceChanged(deviceName); updateCCLabels();
    }
}

void LeadSynthEditor::saveMappingsForCurrentDevice()
{
    auto dn = synthProcessor.getCurrentMidiDeviceName();
    if (dn.isNotEmpty())
        synthProcessor.getSettings().saveMappingsForDevice(dn, synthProcessor.getMidiLearn());
}

void LeadSynthEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!midiLearnMode) return;
    for (auto* knob : allKnobs)
        if (event.eventComponent == &knob->slider)
            { onKnobClickedInLearnMode(knob->paramId); return; }
}

void LeadSynthEditor::onKnobClickedInLearnMode(const juce::String& paramId)
{
    selectedLearnParam = paramId;
    synthProcessor.getMidiLearn().setLearningParam(paramId);
    updateLearnHighlights(); repaint();
}

void LeadSynthEditor::timerCallback()
{
    detectMidiDevice();
    repaint();
    // Sync MIDI learn state (lock-free UI-side update)
    synthProcessor.getMidiLearn().uiTimerUpdate();

    if (midiLearnMode) {
        int learnedCC = synthProcessor.getMidiLearn().getLastLearnedCC();
        if (learnedCC >= 0 && selectedLearnParam.isNotEmpty()) {
            selectedLearnParam.clear();
            saveMappingsForCurrentDevice(); updateCCLabels(); updateLearnHighlights();
        }
    }
}

void LeadSynthEditor::updateCCLabels()
{
    auto& ml = synthProcessor.getMidiLearn();
    for (auto* knob : allKnobs) {
        int cc = ml.getCCForParam(knob->paramId);
        knob->ccLabel.setText(cc >= 0 ? "CC" + juce::String(cc) : "", juce::dontSendNotification);
    }
}

void LeadSynthEditor::updateLearnHighlights()
{
    auto& ml = synthProcessor.getMidiLearn();
    for (auto* knob : allKnobs)
    {
        juce::Colour col;
        if (midiLearnMode && knob->paramId == selectedLearnParam)
            col = juce::Colour(0xffff6600);         // orange: waiting for CC
        else if (midiLearnMode)
            col = juce::Colour(0xff44aa44);          // green: clickable
        else if (ml.getCCForParam(knob->paramId) >= 0)
            col = juce::Colour(0xff6699cc);          // blue tint: has CC assigned
        else
            col = juce::Colour(0xffcccccc);          // default: no mapping
        knob->slider.setColour(juce::Slider::rotarySliderFillColourId, col);
        knob->slider.repaint();
    }
}

// ============================================================================
// ADSR curve
// ============================================================================

void LeadSynthEditor::drawADSRCurve(juce::Graphics& g, juce::Rectangle<int> area,
                                     float a, float d, float s, float r)
{
    g.setColour(juce::Colour(0xff181820));
    g.fillRoundedRectangle(area.toFloat(), 3.0f);

    float x = static_cast<float>(area.getX()) + 3.0f;
    float y = static_cast<float>(area.getY()) + 3.0f;
    float w = static_cast<float>(area.getWidth()) - 6.0f;
    float h = static_cast<float>(area.getHeight()) - 6.0f;
    float bottom = y + h;

    float totalTime = a + d + 0.3f + r;
    if (totalTime < 0.1f) totalTime = 0.1f;
    float scale = w / totalTime;
    float aW = std::max(a * scale, 3.0f), dW = std::max(d * scale, 3.0f);
    float sW = std::max(0.3f * scale, 8.0f), rW = std::max(r * scale, 3.0f);
    float total = aW + dW + sW + rW;
    float ratio = w / total;
    aW *= ratio; dW *= ratio; sW *= ratio; rW *= ratio;
    float sustainY = bottom - s * h;

    juce::Path path;
    path.startNewSubPath(x, bottom);
    path.lineTo(x + aW, y);
    path.lineTo(x + aW + dW, sustainY);
    path.lineTo(x + aW + dW + sW, sustainY);
    path.lineTo(x + aW + dW + sW + rW, bottom);

    juce::Path fillPath(path);
    fillPath.lineTo(x, bottom); fillPath.closeSubPath();
    g.setColour(juce::Colour(0x22cc7700));
    g.fillPath(fillPath);
    g.setColour(juce::Colour(0xffcc7700));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    g.setColour(juce::Colour(0xffee9933));
    float dotR = 2.5f;
    g.fillEllipse(x + aW - dotR, y - dotR, dotR * 2, dotR * 2);
    g.fillEllipse(x + aW + dW - dotR, sustainY - dotR, dotR * 2, dotR * 2);

    g.setColour(juce::Colour(0xff333338));
    g.drawRoundedRectangle(area.toFloat(), 3.0f, 1.0f);
}

// ============================================================================
// Stereo Oscilloscope — 70s CRT aesthetic
// ============================================================================

void LeadSynthEditor::drawOscilloscope(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xff0a0a10));
    g.fillRoundedRectangle(area.toFloat(), 3.0f);

    // Scanlines
    g.setColour(juce::Colour(0x08ffffff));
    for (int row = area.getY(); row < area.getBottom(); row += 2)
        g.drawHorizontalLine(row, static_cast<float>(area.getX()),
                             static_cast<float>(area.getRight()));

    // Graticule
    g.setColour(juce::Colour(0x15cc7700));
    float midY = static_cast<float>(area.getCentreY());
    g.drawHorizontalLine(static_cast<int>(midY),
                         static_cast<float>(area.getX() + 2),
                         static_cast<float>(area.getRight() - 2));
    for (int i = 1; i < 4; ++i) {
        float xp = static_cast<float>(area.getX()) + static_cast<float>(area.getWidth()) * static_cast<float>(i) / 4.0f;
        g.drawVerticalLine(static_cast<int>(xp),
                           static_cast<float>(area.getY() + 2),
                           static_cast<float>(area.getBottom() - 2));
    }

    // Read scope buffer (mono)
    auto& scopeData = synthProcessor.getScopeBufferL();
    int writePos = synthProcessor.getScopeWritePos();
    int bufSize = LeadSynthProcessor::scopeBufferSize;

    float x = static_cast<float>(area.getX()) + 3.0f;
    float w = static_cast<float>(area.getWidth()) - 6.0f;
    float h = static_cast<float>(area.getHeight()) - 6.0f;

    int numPoints = std::min(bufSize, static_cast<int>(w));

    // Find peak for auto-gain (so the waveform fills the display)
    float peak = 0.001f;
    for (int i = 0; i < numPoints; ++i) {
        int idx = (writePos - numPoints + i + bufSize) % bufSize;
        float absVal = std::abs(scopeData[static_cast<size_t>(idx)]);
        if (absVal > peak) peak = absVal;
    }
    float vizGain = std::min(1.0f / peak, 10.0f); // cap at 10x to avoid noise amplification

    juce::Path wavePath;
    bool started = false;

    for (int i = 0; i < numPoints; ++i) {
        int idx = (writePos - numPoints + i + bufSize) % bufSize;
        float sample = std::clamp(scopeData[static_cast<size_t>(idx)] * vizGain, -1.0f, 1.0f);
        float px = x + (static_cast<float>(i) / static_cast<float>(numPoints)) * w;
        float py = midY - sample * h * 0.45f;
        if (!started) { wavePath.startNewSubPath(px, py); started = true; }
        else wavePath.lineTo(px, py);
    }

    if (started) {
        g.setColour(juce::Colour(0x30cc7700));
        g.strokePath(wavePath, juce::PathStrokeType(3.5f));
        g.setColour(juce::Colour(0x55ee8800));
        g.strokePath(wavePath, juce::PathStrokeType(2.0f));
        g.setColour(juce::Colour(0xffee9933));
        g.strokePath(wavePath, juce::PathStrokeType(1.2f));
    }

    g.setColour(juce::Colour(0xff333338));
    g.drawRoundedRectangle(area.toFloat(), 3.0f, 1.0f);
}

// ============================================================================
// Footer
// ============================================================================

void LeadSynthEditor::drawFooter(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xff111115));
    g.fillRect(area);
    g.setFont(juce::FontOptions(9.5f));

    juce::String midiText = lastDetectedDevice.isNotEmpty()
        ? "MIDI: " + lastDetectedDevice : "MIDI: No device";
    g.setColour(juce::Colour(0xff666666));
    g.drawText(midiText, area.withTrimmedLeft(6).withWidth(200),
               juce::Justification::centredLeft);

    // Octave indicator
    juce::String octaveText = "Oct: C" + juce::String(keyboardBaseOctave)
                            + "-C" + juce::String(keyboardBaseOctave + 2);
    g.setColour(juce::Colour(0xffcc7700));
    g.drawText(octaveText, area.withTrimmedLeft(220).withWidth(100),
               juce::Justification::centredLeft);

    float cpu = synthProcessor.getCpuLoad();
    int voices = synthProcessor.getActiveVoiceCount();
    juce::String rightText = "Voices: " + juce::String(voices)
                           + "   CPU: " + juce::String(cpu, 1) + "%";
    g.setColour(cpu > 50.0f ? juce::Colour(0xffcc4444) : juce::Colour(0xff666666));
    g.drawText(rightText, area.withTrimmedRight(8), juce::Justification::centredRight);
}

// ============================================================================
// PAINT
// ============================================================================

void LeadSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.addTransform(juce::AffineTransform::scale(scaleFactor)
                       .translated(offsetX, offsetY));

    auto bounds = juce::Rectangle<int>(0, 0, designWidth, designHeight);
    auto leftWood = bounds.removeFromLeft(woodPanelWidth);
    auto rightWood = bounds.removeFromRight(woodPanelWidth);
    SynthLookAndFeel::drawWoodPanel(g, leftWood, true);
    SynthLookAndFeel::drawWoodPanel(g, rightWood, false);
    SynthLookAndFeel::drawFaceplate(g, bounds);

    // Header
    auto headerArea = bounds.removeFromTop(headerHeight);
    g.setColour(juce::Colour(0xff111115)); g.fillRect(headerArea);
    g.setColour(juce::Colour(0xffcc7700));
    g.fillRect(headerArea.getX(), headerArea.getBottom() - 2, headerArea.getWidth(), 2);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("LEADSYNTH", headerArea.withTrimmedLeft(12).withWidth(180),
               juce::Justification::centredLeft);

    auto presetName = synthProcessor.getPresetName();
    if (synthProcessor.isPresetModified()) presetName += " *";
    g.setColour(juce::Colour(0xffcc7700));
    g.setFont(juce::FontOptions(15.0f));
    g.drawText(presetName, headerArea.withTrimmedLeft(230).withWidth(250),
               juce::Justification::centredLeft);

    // Section headers
    int panelLeft = woodPanelWidth;
    int panelWidth = designWidth - woodPanelWidth * 2;
    int sectionY = headerHeight + 2;

    struct Section { const char* name; float widthRatio; };
    Section sections[] = {
        { "OSCILLATOR 1", 0.075f },  { "OSCILLATOR 2", 0.135f },
        { "MIXER", 0.045f },         { "FILTER", 0.155f },
        { "FILTER ENV", 0.10f },     { "AMP ENV", 0.10f },
        { "LFO", 0.085f },           { "ARP", 0.13f },
        { "OUTPUT", 0.095f }
    };

    int sxx = panelLeft;
    for (auto& sec : sections) {
        int sw = static_cast<int>(static_cast<float>(panelWidth) * sec.widthRatio);
        SynthLookAndFeel::drawSectionHeader(g, { sxx, sectionY, sw, sectionHeaderH }, sec.name);
        g.setColour(juce::Colour(0xff444448));
        g.fillRect(sxx + sw - 1, sectionY + sectionHeaderH, 1,
                   designHeight - keyboardHeight - sectionY - sectionHeaderH - 24);
        sxx += sw;
    }

    // ADSR visualizations (bigger)
    auto& apvts = synthProcessor.getAPVTS();
    int envSectionW = static_cast<int>(static_cast<float>(panelWidth) * 0.10f);
    int fltEnvX = panelLeft + static_cast<int>(static_cast<float>(panelWidth) * 0.41f) + 4;
    int ampEnvX = panelLeft + static_cast<int>(static_cast<float>(panelWidth) * 0.51f) + 4;
    int adsrVizY = 330;
    int adsrVizH = 75;
    int adsrVizW = envSectionW - 12;

    drawADSRCurve(g, { fltEnvX, adsrVizY, adsrVizW, adsrVizH },
                  *apvts.getRawParameterValue("fltAttack"),  *apvts.getRawParameterValue("fltDecay"),
                  *apvts.getRawParameterValue("fltSustain"), *apvts.getRawParameterValue("fltRelease"));
    drawADSRCurve(g, { ampEnvX, adsrVizY, adsrVizW, adsrVizH },
                  *apvts.getRawParameterValue("ampAttack"),  *apvts.getRawParameterValue("ampDecay"),
                  *apvts.getRawParameterValue("ampSustain"), *apvts.getRawParameterValue("ampRelease"));

    // Stereo oscilloscope — same size as ADSR boxes
    int outputX = panelLeft + static_cast<int>(static_cast<float>(panelWidth) * 0.825f) + 4;
    int outputW = static_cast<int>(static_cast<float>(panelWidth) * 0.095f) - 12;
    drawOscilloscope(g, { outputX, adsrVizY, outputW, adsrVizH });

    // Footer
    // Footer (above the keyboard)
    int footerY = designHeight - keyboardHeight - 22;
    drawFooter(g, { panelLeft, footerY, panelWidth, 22 });

    // Keyboard background strip
    g.setColour(juce::Colour(0xff111115));
    g.fillRect(panelLeft, designHeight - keyboardHeight, panelWidth, keyboardHeight);


    // Learn mode status
    if (midiLearnMode && selectedLearnParam.isNotEmpty()) {
        g.setColour(juce::Colour(0xffff9944));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("Move a CC knob/fader...",
                   panelLeft + panelWidth / 2 - 100, footerY + 2, 200, 18,
                   juce::Justification::centred);
    } else if (midiLearnMode) {
        g.setColour(juce::Colour(0xff44aa44));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("Click a knob to assign CC",
                   panelLeft + panelWidth / 2 - 100, footerY + 2, 200, 18,
                   juce::Justification::centred);
    }

    // Octave bar is drawn in screen coordinates after the transform
    // (see below, after restoring identity transform)
}

void LeadSynthEditor::paintOverChildren(juce::Graphics&)
{
}

// ============================================================================
// LAYOUT
// ============================================================================

void LeadSynthEditor::resized()
{
    float scaleX = static_cast<float>(getWidth()) / static_cast<float>(designWidth);
    float scaleY = static_cast<float>(getHeight()) / static_cast<float>(designHeight);
    scaleFactor = std::min(scaleX, scaleY);
    offsetX = (static_cast<float>(getWidth()) - static_cast<float>(designWidth) * scaleFactor) * 0.5f;
    offsetY = (static_cast<float>(getHeight()) - static_cast<float>(designHeight) * scaleFactor) * 0.5f;

    auto s = [this](int v) { return static_cast<int>(static_cast<float>(v) * scaleFactor); };
    int ox = static_cast<int>(offsetX);
    int oy = static_cast<int>(offsetY);

    int panelLeft = woodPanelWidth;
    int panelWidth = designWidth - woodPanelWidth * 2;

    int knobW = 52, knobH = 52, labelH = 14, ccLabelH = 10;
    int comboH = 22;
    int controlsY = headerHeight + sectionHeaderH + 12;
    int row2Y = controlsY + labelH + knobH + ccLabelH + 8;
    int row3Y = row2Y + labelH + knobH + ccLabelH + 4;

    auto placeKnob = [&](KnobWithLabel& k, int x, int y) {
        k.label.setBounds(ox + s(x), oy + s(y), s(knobW), s(labelH));
        k.slider.setBounds(ox + s(x), oy + s(y + labelH), s(knobW), s(knobH));
        k.ccLabel.setBounds(ox + s(x), oy + s(y + labelH + knobH), s(knobW), s(ccLabelH));
    };

    int waveComboW = 75, smallComboW = 58;
    auto placeWaveCombo = [&](ComboWithLabel& c, int x, int y) {
        c.label.setBounds(ox + s(x), oy + s(y), s(waveComboW), s(labelH));
        c.combo.setBounds(ox + s(x), oy + s(y + labelH + 2), s(waveComboW), s(comboH));
    };
    auto placeSmallCombo = [&](ComboWithLabel& c, int x, int y) {
        c.label.setBounds(ox + s(x), oy + s(y), s(smallComboW), s(labelH));
        c.combo.setBounds(ox + s(x), oy + s(y + labelH + 2), s(smallComboW), s(comboH));
    };

    //                    OSC1  OSC2   MIX   FILT  FENV  AENV  LFO   ARP   OUT
    float ratios[] = { 0.0f, 0.075f, 0.21f, 0.255f, 0.41f, 0.51f, 0.61f, 0.695f, 0.825f };
    auto sx = [&](int sec) { return panelLeft + static_cast<int>(static_cast<float>(panelWidth) * ratios[sec]) + 6; };

    // Get section center X for vertically centering single knobs
    auto sectionCenterX = [&](int sec, int secWidth) {
        return sx(sec) + (secWidth - knobW) / 2;
    };

    // OSC 1: WAVE centered row 1, LEVEL centered row 2
    int osc1SecW = static_cast<int>(static_cast<float>(panelWidth) * 0.09f) - 12;
    int osc1cx = sectionCenterX(0, osc1SecW);
    placeWaveCombo(osc1Wave, osc1cx - 10, controlsY);
    placeKnob(osc1Level, osc1cx, row2Y);

    // OSC 2: WAVE row 1, LEVEL row 2 (aligned with OSC1), COARSE+FINE row 3
    placeWaveCombo(osc2Wave, sx(1), controlsY);
    placeKnob(osc2Level, sx(1), row2Y);
    placeKnob(osc2Coarse, sx(1), row3Y);
    placeKnob(osc2Fine, sx(1) + 56, row3Y);

    // Mixer: NOISE centered vertically in section
    int mixerSecW = static_cast<int>(static_cast<float>(panelWidth) * 0.055f) - 12;
    placeKnob(noiseLevel, sectionCenterX(2, mixerSecW), controlsY);

    // Filter
    placeKnob(filterCutoff,   sx(3), controlsY);
    placeKnob(filterReso,     sx(3) + 52, controlsY);
    placeSmallCombo(filterSlope, sx(3) + 104, controlsY);
    placeKnob(filterEnvAmt,   sx(3), row2Y);
    placeKnob(filterKeyTrack, sx(3) + 52, row2Y);

    // Filter Envelope
    placeKnob(fltA, sx(4),      controlsY);
    placeKnob(fltD, sx(4) + 48, controlsY);
    placeKnob(fltS, sx(4),      row2Y);
    placeKnob(fltR, sx(4) + 48, row2Y);

    // Amp Envelope
    placeKnob(ampA, sx(5),      controlsY);
    placeKnob(ampD, sx(5) + 48, controlsY);
    placeKnob(ampS, sx(5),      row2Y);
    placeKnob(ampR, sx(5) + 48, row2Y);

    // LFO: RATE + DEPTH row 1, DEST row 2
    placeKnob(lfoRate,  sx(6), controlsY);
    placeKnob(lfoDepth, sx(6) + 52, controlsY);
    lfoDest.label.setBounds(ox + s(sx(6)), oy + s(row2Y + 4), s(70), s(labelH));
    lfoDest.combo.setBounds(ox + s(sx(6)), oy + s(row2Y + 4 + labelH + 2), s(70), s(comboH));

    // ARP section (index 7)
    {
        int ax = sx(7);
        int r1 = controlsY;
        int r2 = r1 + 26;
        int r3 = r2 + 22;

        // Row 1: ARP + LATCH toggles
        arpToggle.setBounds(ox + s(ax), oy + s(r1), s(44), s(22));
        arpLatchToggle.setBounds(ox + s(ax + 48), oy + s(r1), s(50), s(22));

        // Row 2: TEMPO label + numeric box
        arpTempoLabel.setBounds(ox + s(ax), oy + s(r2), s(38), s(16));
        arpTempoSlider.setBounds(ox + s(ax + 38), oy + s(r2), s(72), s(18));

        // Row 3: RATE + SYNC combos
        placeSmallCombo(arpRate, ax, r3);
        placeSmallCombo(arpSync, ax + 62, r3);

        // Row 4: PATTERN + OCTAVES + GATE
        int r4 = row2Y;
        placeSmallCombo(arpPattern, ax, r4);
        placeSmallCombo(arpOctaves, ax + 62, r4);
        placeKnob(arpGate, ax, r4 + 42);
    }

    // Output section (index 8): Glide + Master
    placeKnob(glideTime,  sx(8), controlsY);
    placeKnob(masterGain, sx(8) + 52, controlsY);

    // Header buttons
    int hdrBtnY = 8;
    int hdrRight = panelLeft + panelWidth;

    presetPrev.setBounds(ox + s(200), oy + s(hdrBtnY), s(24), s(24));
    presetNext.setBounds(ox + s(228), oy + s(hdrBtnY), s(24), s(24));
    presetNameButton.setBounds(ox + s(260), oy + s(hdrBtnY - 2), s(220), s(28));

   #if JUCE_DEBUG
    testButton.setBounds(ox + s(hdrRight - 55), oy + s(hdrBtnY), s(50), s(24));
    panicButton.setBounds(ox + s(hdrRight - 120), oy + s(hdrBtnY), s(58), s(24));
    clearMappingsButton.setBounds(ox + s(hdrRight - 165), oy + s(hdrBtnY), s(40), s(24));
    midiLearnButton.setBounds(ox + s(hdrRight - 260), oy + s(hdrBtnY), s(90), s(24));
   #else
    panicButton.setBounds(ox + s(hdrRight - 65), oy + s(hdrBtnY), s(58), s(24));
    clearMappingsButton.setBounds(ox + s(hdrRight - 110), oy + s(hdrBtnY), s(40), s(24));
    midiLearnButton.setBounds(ox + s(hdrRight - 205), oy + s(hdrBtnY), s(90), s(24));
   #endif

    // On-screen keyboard at the bottom
    keyboard.setBounds(ox + s(panelLeft), oy + s(designHeight - keyboardHeight),
                       s(panelWidth), s(keyboardHeight));

}
