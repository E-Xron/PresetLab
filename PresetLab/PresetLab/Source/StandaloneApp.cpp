// The PresetLab app: JUCE's standalone window, set up for playing live. MIDI keyboards open
// automatically and the audio input is not muted on first launch (the Live Input mode needs it).
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

#if JucePlugin_Build_Standalone && JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

class PresetLabApp final : public juce::JUCEApplication
{
public:
    PresetLabApp()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "PresetLab";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        o.folderName = "~/.config";
       #endif
        props.setStorageParameters (o);
    }

    const juce::String getApplicationName() override           { return "PresetLab"; }
    const juce::String getApplicationVersion() override        { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override                 { return true; }
    void anotherInstanceStarted (const juce::String&) override {}

    void initialise (const juce::String&) override
    {
        auto* settings = props.getUserSettings();
        if (! settings->containsKey ("shouldMuteInput")) settings->setValue ("shouldMuteInput", false);

        auto holder = std::make_unique<juce::StandalonePluginHolder> (settings, false, juce::String(), nullptr,
                                                                      juce::Array<juce::StandalonePluginHolder::PluginInOuts>(), true);
        window = std::make_unique<juce::StandaloneFilterWindow> ("PresetLab", juce::Colour (0xff0c0e13), std::move (holder));
        window->setVisible (true);
    }

    void shutdown() override
    {
        window = nullptr;
        props.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (window != nullptr) window->pluginHolder->savePluginState();
        if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
            juce::Timer::callAfterDelay (100, [] { if (auto* app = juce::JUCEApplicationBase::getInstance()) app->systemRequestedQuit(); });
        else
            quit();
    }

private:
    juce::ApplicationProperties props;
    std::unique_ptr<juce::StandaloneFilterWindow> window;
};

juce::JUCEApplicationBase* juce_CreateApplication() { return new PresetLabApp(); }

#endif
