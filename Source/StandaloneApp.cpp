#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace juce
{

class LeadSynthStandaloneApp final : public JUCEApplication
{
public:
    LeadSynthStandaloneApp()
    {
        PropertiesFile::Options options;
        options.applicationName     = CharPointer_UTF8(JucePlugin_Name);
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        appProperties.setStorageParameters(options);
    }

    const String getApplicationName() override    { return CharPointer_UTF8(JucePlugin_Name); }
    const String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override     { return true; }
    void anotherInstanceStarted(const String&) override {}

    void initialise(const String&) override
    {
        mainWindow = rawToUniquePtr(createWindow());
        if (mainWindow != nullptr)
            mainWindow->setVisible(true);

        // macOS menu bar
       #if JUCE_MAC
        MenuBarModel::setMacMainMenu(&menuModel);
       #endif
    }

    void shutdown() override
    {
       #if JUCE_MAC
        MenuBarModel::setMacMainMenu(nullptr);
       #endif
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->pluginHolder->savePluginState();

        if (ModalComponentManager::getInstance()->cancelAllModalComponents())
        {
            Timer::callAfterDelay(100, []()
            {
                if (auto app = JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        }
        else
        {
            quit();
        }
    }

private:
    StandaloneFilterWindow* createWindow()
    {
        if (Desktop::getInstance().getDisplays().displays.isEmpty())
            return nullptr;

        constexpr bool autoOpenMidiDevices = false;
        const Array<StandalonePluginHolder::PluginInOuts> channelConfig;

        auto holder = std::make_unique<StandalonePluginHolder>(
            appProperties.getUserSettings(), false, String{}, nullptr,
            channelConfig, autoOpenMidiDevices);

        return new StandaloneFilterWindow(
            getApplicationName(),
            LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
            std::move(holder));
    }

    // =========================================================================
    // macOS menu bar with About
    // =========================================================================
    class AppMenuModel : public MenuBarModel
    {
    public:
        StringArray getMenuBarNames() override
        {
            return { "File" };
        }

        PopupMenu getMenuForIndex(int menuIndex, const String&) override
        {
            PopupMenu menu;
            if (menuIndex == 0)
            {
                menu.addItem(1, "About LeadSynth");
            }
            return menu;
        }

        void menuItemSelected(int menuItemID, int) override
        {
            if (menuItemID == 1)
                showAbout();
        }

        static void showAbout()
        {
            auto options = DialogWindow::LaunchOptions();
            auto* content = new Component();
            content->setSize(420, 280);

            auto* text = new Label("aboutText", String(
                "LEADSYNTH\n"
                "Virtual Analog Synthesizer\n"
                "\n"
                "Inspired by the legendary Moog synthesizers,\n"
                "Prophet synthesizer family by Sequential,\n"
                "and Oberheim.\n"
                "\n"
                "Created by Jarmo Annala\n"
                "\n"
                "Version 1.0.0"));

            text->setFont(FontOptions(14.0f));
            text->setJustificationType(Justification::centred);
            text->setColour(Label::textColourId, Colours::white);
            text->setBounds(0, 20, 420, 240);
            content->addAndMakeVisible(text);

            options.content.setOwned(content);
            options.dialogTitle = "About LeadSynth";
            options.dialogBackgroundColour = Colour(0xff1a1a2e);
            options.escapeKeyTriggersCloseButton = true;
            options.useNativeTitleBar = true;
            options.resizable = false;
            options.launchAsync();
        }
    };

    AppMenuModel menuModel;
    ApplicationProperties appProperties;
    std::unique_ptr<StandaloneFilterWindow> mainWindow;
};

} // namespace juce

// This is what JUCE calls to create the application
juce::JUCEApplicationBase* juce_CreateApplication()
{
    return new juce::LeadSynthStandaloneApp();
}
