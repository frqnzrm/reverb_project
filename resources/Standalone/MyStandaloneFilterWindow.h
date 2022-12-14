/*
 ==============================================================================
 This file is part of the IEM plug-in suite.
 Author: Daniel Rudrich
 Copyright (c) 2017 - Institute of Electronic Music and Acoustics (IEM)
 https://iem.at

 The IEM plug-in suite is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 The IEM plug-in suite is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this software.  If not, see <https://www.gnu.org/licenses/>.
 ==============================================================================
 */

/*
 The following code taken from JUCE and modified.
 */

/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "../../source/LookAndFeel_frqz_rm.h"
#include "../../source/SimpleLabel.h"
#include "IEM_AudioDeviceSelectorComponent.h"

#if JUCE_MAC || JUCE_LINUX
    #if DONT_BUILD_WITH_JACK_SUPPORT
        #define BUILD_WITH_JACK_SUPPORT 0
    #else
        #define BUILD_WITH_JACK_SUPPORT 1
    #endif
#else
    #define BUILD_WITH_JACK_SUPPORT 0
#endif


#if BUILD_WITH_JACK_SUPPORT
    #include "JackAudio.h"
#endif

#include <juce_audio_plugin_client/utility/juce_CreatePluginFilter.h>

//==============================================================================
/**
    An object that creates and plays a standalone instance of an AudioProcessor.

    The object will create your processor using the same createPluginFilter()
    function that the other plugin wrappers use, and will run it through the
    computer's audio/MIDI devices using AudioDeviceManager and AudioProcessorPlayer.

    @tags{Audio}
*/
class MyStandalonePluginHolder    : private juce::AudioIODeviceCallback,
                                  private juce::Timer
{
public:
    //==============================================================================
    /** Structure used for the number of inputs and outputs. */
    struct PluginInOuts   { short numIns, numOuts; };

    //==============================================================================
    /** Creates an instance of the default plugin.

        The settings object can be a PropertySet that the class should use to store its
        settings - the takeOwnershipOfSettings indicates whether this object will delete
        the settings automatically when no longer needed. The settings can also be nullptr.

        A default device name can be passed in.

        Preferably a complete setup options object can be used, which takes precedence over
        the preferredDefaultDeviceName and allows you to select the input & output device names,
        sample rate, buffer size etc.

        In all instances, the settingsToUse will take precedence over the "preferred" options if not null.
    */
    MyStandalonePluginHolder (juce::PropertySet* settingsToUse,
                            bool takeOwnershipOfSettings = true,
                            const juce::String& preferredDefaultDeviceName = juce::String(),
                            const juce::AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                            const juce::Array<PluginInOuts>& channels = juce::Array<PluginInOuts>(),
                           #if JUCE_ANDROID || JUCE_IOS
                            bool shouldAutoOpenMidiDevices = true
                           #else
                            bool shouldAutoOpenMidiDevices = false
                           #endif
                            )

        : settings (settingsToUse, takeOwnershipOfSettings),
          channelConfiguration (channels),
          shouldMuteInput (! isInterAppAudioConnected()),
          autoOpenMidiDevices (shouldAutoOpenMidiDevices)
    {
        createPlugin();

        juce::OwnedArray<juce::AudioIODeviceType> types;
        deviceManager.createAudioDeviceTypes (types);

        for (auto* t : types)
            deviceManager.addAudioDeviceType (std::unique_ptr<juce::AudioIODeviceType> (t));

        types.clearQuick (false);
#if BUILD_WITH_JACK_SUPPORT

        deviceManager.addAudioDeviceType (std::make_unique<JackAudioIODeviceType> ());
#endif

        auto inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns
                                                           : processor->getMainBusNumInputChannels());

        if (preferredSetupOptions != nullptr)
            options.reset (new juce::AudioDeviceManager::AudioDeviceSetup (*preferredSetupOptions));

        auto audioInputRequired = (inChannels > 0);

        if (audioInputRequired && juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
            && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
            juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                         [this, preferredDefaultDeviceName] (bool granted) { init (granted, preferredDefaultDeviceName); });
        else
            init (audioInputRequired, preferredDefaultDeviceName);
    }

    void init (bool enableAudioInput, const juce::String& preferredDefaultDeviceName)
    {
        setupAudioDevices (enableAudioInput, preferredDefaultDeviceName, options.get());
        reloadPluginState();
        startPlaying();

       if (autoOpenMidiDevices)
           startTimer (500);
    }

    virtual ~MyStandalonePluginHolder() override
    {
        stopTimer();

        deletePlugin();
        shutDownAudioDevices();
    }

    //==============================================================================
    virtual void createPlugin()
    {
      #if JUCE_MODULE_AVAILABLE_juce_audio_plugin_client
        processor.reset (createPluginFilterOfType (juce::AudioProcessor::wrapperType_Standalone));
      #else
        AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Standalone);
        processor.reset (createPluginFilter());
        AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Undefined);
      #endif
        jassert (processor != nullptr); // Your createPluginFilter() function must return a valid object!

        processor->disableNonMainBuses();
        processor->setRateAndBufferSizeDetails (44100, 512);

        int inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns
                                                          : processor->getMainBusNumInputChannels());

        int outChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numOuts
                                                           : processor->getMainBusNumOutputChannels());

        processorHasPotentialFeedbackLoop = (inChannels > 0 && outChannels > 0);
    }

    virtual void deletePlugin()
    {
        stopPlaying();
        processor = nullptr;
    }

    static juce::String getFilePatterns (const juce::String& fileSuffix)
    {
        if (fileSuffix.isEmpty())
            return {};

        return (fileSuffix.startsWithChar ('.') ? "*" : "*.") + fileSuffix;
    }

    //==============================================================================
    juce::Value& getMuteInputValue()                           { return shouldMuteInput; }
    bool getProcessorHasPotentialFeedbackLoop() const    { return processorHasPotentialFeedbackLoop; }

    //==============================================================================
    juce::File getLastFile() const
    {
        juce::File f;

        if (settings != nullptr)
            f = juce::File (settings->getValue ("lastStateFile"));

        if (f == juce::File())
            f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

        return f;
    }

    void setLastFile (const juce::FileChooser& fc)
    {
        if (settings != nullptr)
            settings->setValue ("lastStateFile", fc.getResult().getFullPathName());
    }

    /** Pops up a dialog letting the user save the processor's state to a file. */
    void askUserToSaveState (const juce::String& fileSuffix = juce::String())
    {
       #if JUCE_MODAL_LOOPS_PERMITTED
        juce::FileChooser fc (TRANS("Save current state"), getLastFile(), getFilePatterns (fileSuffix));

        if (fc.browseForFileToSave (true))
        {
            setLastFile (fc);

            juce::MemoryBlock data;
            processor->getStateInformation (data);

            if (! fc.getResult().replaceWithData (data.getData(), data.getSize()))
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                  TRANS("Error whilst saving"),
                                                  TRANS("Couldn't write to the specified file!"));
        }
       #else
        ignoreUnused (fileSuffix);
       #endif
    }

    /** Pops up a dialog letting the user re-load the processor's state from a file. */
    void askUserToLoadState (const juce::String& fileSuffix = juce::String())
    {
       #if JUCE_MODAL_LOOPS_PERMITTED
        juce::FileChooser fc (TRANS("Load a saved state"), getLastFile(), getFilePatterns (fileSuffix));

        if (fc.browseForFileToOpen())
        {
            setLastFile (fc);

            juce::MemoryBlock data;

            if (fc.getResult().loadFileAsData (data))
                processor->setStateInformation (data.getData(), (int) data.getSize());
            else
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                  TRANS("Error whilst loading"),
                                                  TRANS("Couldn't read from the specified file!"));
        }
       #else
        ignoreUnused (fileSuffix);
       #endif
    }

    //==============================================================================
    void startPlaying()
    {
        player.setProcessor (processor.get());

       #if JucePlugin_Enable_IAA && JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
        {
            processor->setPlayHead (device->getAudioPlayHead());
            device->setMidiMessageCollector (&player.getMidiMessageCollector());
        }
       #endif
    }

    void stopPlaying()
    {
        player.setProcessor (nullptr);
    }

    //==============================================================================
    /** Shows an audio properties dialog box modally. */
    void showAudioSettingsDialog()
    {
        juce::DialogWindow::LaunchOptions o;

        int minNumInputs  = std::numeric_limits<int>::max(), maxNumInputs  = 0,
            minNumOutputs = std::numeric_limits<int>::max(), maxNumOutputs = 0;

        auto updateMinAndMax = [] (int newValue, int& minValue, int& maxValue)
        {
            minValue = juce::jmin (minValue, newValue);
            maxValue = juce::jmax (maxValue, newValue);
        };

        if (channelConfiguration.size() > 0)
        {
            auto defaultConfig = channelConfiguration.getReference (0);
            updateMinAndMax ((int) defaultConfig.numIns,  minNumInputs,  maxNumInputs);
            updateMinAndMax ((int) defaultConfig.numOuts, minNumOutputs, maxNumOutputs);
        }

        if (auto* bus = processor->getBus (true, 0))
            updateMinAndMax (bus->getDefaultLayout().size(), minNumInputs, maxNumInputs);

        if (auto* bus = processor->getBus (false, 0))
            updateMinAndMax (bus->getDefaultLayout().size(), minNumOutputs, maxNumOutputs);

        minNumInputs  = juce::jmin (minNumInputs,  maxNumInputs);
        minNumOutputs = juce::jmin (minNumOutputs, maxNumOutputs);

        o.content.setOwned (new SettingsComponent (*this, deviceManager,
                                                          minNumInputs,
                                                          maxNumInputs,
                                                          minNumOutputs,
                                                          maxNumOutputs));
        o.content->setSize (500, 550);

        o.dialogTitle                   = TRANS("Audio/MIDI Settings");
        o.dialogBackgroundColour        = o.content->getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
        o.escapeKeyTriggersCloseButton  = true;
        o.useNativeTitleBar             = true;
        o.resizable                     = false;

        o.launchAsync();
    }

    void saveAudioDeviceState()
    {
        if (settings != nullptr)
        {
            std::unique_ptr<juce::XmlElement> xml (deviceManager.createStateXml());

            settings->setValue ("audioSetup", xml.get());

           #if ! (JUCE_IOS || JUCE_ANDROID)
            settings->setValue ("shouldMuteInput", (bool) shouldMuteInput.getValue());
           #endif
        }
    }

    void reloadAudioDeviceState (bool enableAudioInput,
                                 const juce::String& preferredDefaultDeviceName,
                                 const juce::AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        std::unique_ptr<juce::XmlElement> savedState;

        if (settings != nullptr)
        {
            savedState = settings->getXmlValue ("audioSetup");

           #if ! (JUCE_IOS || JUCE_ANDROID)
            shouldMuteInput.setValue (settings->getBoolValue ("shouldMuteInput", true));
           #endif
        }

        auto totalInChannels  = processor->getMainBusNumInputChannels();
        auto totalOutChannels = processor->getMainBusNumOutputChannels();

        if (channelConfiguration.size() > 0)
        {
            auto defaultConfig = channelConfiguration.getReference (0);
            totalInChannels  = defaultConfig.numIns;
            totalOutChannels = defaultConfig.numOuts;
        }

        deviceManager.initialise (enableAudioInput ? totalInChannels : 0,
                                  totalOutChannels,
                                  savedState.get(),
                                  true,
                                  preferredDefaultDeviceName,
                                  preferredSetupOptions);
    }

    //==============================================================================
    void savePluginState()
    {
        if (settings != nullptr && processor != nullptr)
        {
            juce::MemoryBlock data;
            processor->getStateInformation (data);

            settings->setValue ("filterState", data.toBase64Encoding());
        }
    }

    void reloadPluginState()
    {
        if (settings != nullptr)
        {
            juce::MemoryBlock data;

            if (data.fromBase64Encoding (settings->getValue ("filterState")) && data.getSize() > 0)
                processor->setStateInformation (data.getData(), (int) data.getSize());
        }
    }

    //==============================================================================
    void switchToHostApplication()
    {
       #if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            device->switchApplication();
       #endif
    }

    bool isInterAppAudioConnected()
    {
       #if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            return device->isInterAppAudioConnected();
       #endif

        return false;
    }

   #if JUCE_MODULE_AVAILABLE_juce_gui_basics
    juce::Image getIAAHostIcon (int size)
    {
       #if JUCE_IOS && JucePlugin_Enable_IAA
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            return device->getIcon (size);
       #else
        juce::ignoreUnused (size);
       #endif

        return {};
    }
   #endif

    static MyStandalonePluginHolder* getInstance();

    //==============================================================================
    juce::OptionalScopedPointer<juce::PropertySet> settings;
    std::unique_ptr<juce::AudioProcessor> processor;
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer player;
    juce::Array<PluginInOuts> channelConfiguration;

    // avoid feedback loop by default
    bool processorHasPotentialFeedbackLoop = true;
    juce::Value shouldMuteInput;
    juce::AudioBuffer<float> emptyBuffer;
    bool autoOpenMidiDevices;

    std::unique_ptr<juce::AudioDeviceManager::AudioDeviceSetup> options;
    juce::StringArray lastMidiDevices;

    juce::String jackClientName = "";

private:
    //==============================================================================
    class SettingsComponent : public juce::Component
    {
    public:
        SettingsComponent (MyStandalonePluginHolder& pluginHolder,
                           juce::AudioDeviceManager& deviceManagerToUse,
                           int minAudioInputChannels,
                           int maxAudioInputChannels,
                           int minAudioOutputChannels,
                           int maxAudioOutputChannels)
            : owner (pluginHolder),
              deviceSelector (deviceManagerToUse,
                              minAudioInputChannels, maxAudioInputChannels,
                              minAudioOutputChannels, maxAudioOutputChannels,
                              true,
                              (pluginHolder.processor.get() != nullptr && pluginHolder.processor->producesMidi()),
                              true, false),
              shouldMuteLabel  ("Feedback Loop:", "Feedback Loop:"),
              shouldMuteButton ("Mute audio input")
        {
            setOpaque (true);

            shouldMuteButton.setClickingTogglesState (true);
            shouldMuteButton.getToggleStateValue().referTo (owner.shouldMuteInput);

            addAndMakeVisible (deviceSelector);

            if (owner.getProcessorHasPotentialFeedbackLoop())
            {
                addAndMakeVisible (shouldMuteButton);
                addAndMakeVisible (shouldMuteLabel);

                shouldMuteLabel.attachToComponent (&shouldMuteButton, true);
            }
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
        }

        void resized() override
        {
            auto r = getLocalBounds();

            if (owner.getProcessorHasPotentialFeedbackLoop())
            {
                auto itemHeight = deviceSelector.getItemHeight();
                auto extra = r.removeFromTop (itemHeight);

                auto seperatorHeight = (itemHeight >> 1);
                shouldMuteButton.setBounds (juce::Rectangle<int> (extra.proportionOfWidth (0.35f), seperatorHeight,
                                                            extra.proportionOfWidth (0.60f), deviceSelector.getItemHeight()));

                r.removeFromTop (seperatorHeight);
            }

            deviceSelector.setBounds (r);
        }

    private:
        //==============================================================================
        MyStandalonePluginHolder& owner;
        iem::IEMAudioDeviceSelectorComponent deviceSelector;
        juce::Label shouldMuteLabel;
        juce::ToggleButton shouldMuteButton;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComponent)
    };

    //==============================================================================
    void audioDeviceIOCallback (const float** inputChannelData,
                                int numInputChannels,
                                float** outputChannelData,
                                int numOutputChannels,
                                int numSamples) override
    {
        const bool inputMuted = shouldMuteInput.getValue();

        if (inputMuted)
        {
            emptyBuffer.clear();
            inputChannelData = emptyBuffer.getArrayOfReadPointers();
        }

        player.audioDeviceIOCallback (inputChannelData, numInputChannels,
                                      outputChannelData, numOutputChannels, numSamples);
    }

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override
    {
        emptyBuffer.setSize (device->getActiveInputChannels().countNumberOfSetBits(), device->getCurrentBufferSizeSamples());
        emptyBuffer.clear();

        player.audioDeviceAboutToStart (device);
        player.setMidiOutput (deviceManager.getDefaultMidiOutput());

        if (device->getTypeName() == "JACK")
            jackClientName = device->getName();
        else
            jackClientName = "";
    }

    void audioDeviceStopped() override
    {
        player.setMidiOutput (nullptr);
        player.audioDeviceStopped();
        emptyBuffer.setSize (0, 0);
    }

    //==============================================================================
    void setupAudioDevices (bool enableAudioInput,
                            const juce::String& preferredDefaultDeviceName,
                            const juce::AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        deviceManager.addAudioCallback (this);
        deviceManager.addMidiInputCallback ({}, &player);

        reloadAudioDeviceState (enableAudioInput, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void shutDownAudioDevices()
    {
        saveAudioDeviceState();

        deviceManager.removeMidiInputCallback ({}, &player);
        deviceManager.removeAudioCallback (this);
    }

    void timerCallback() override
    {
        auto newMidiDevices = juce::MidiInput::getDevices();

        if (newMidiDevices != lastMidiDevices)
        {
            for (auto& oldDevice : lastMidiDevices)
                if (! newMidiDevices.contains (oldDevice))
                    deviceManager.setMidiInputEnabled (oldDevice, false);

            for (auto& newDevice : newMidiDevices)
                if (! lastMidiDevices.contains (newDevice))
                    deviceManager.setMidiInputEnabled (newDevice, true);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyStandalonePluginHolder)
};

//==============================================================================
/**
    A class that can be used to run a simple standalone application containing your filter.

    Just create one of these objects in your JUCEApplicationBase::initialise() method, and
    let it do its work. It will create your filter object using the same createPluginFilter() function
    that the other plugin wrappers use.

    @tags{Audio}
*/
class MyStandaloneFilterWindow    : public juce::DocumentWindow,
                                  public juce::Button::Listener
{
public:
    //==============================================================================
    typedef MyStandalonePluginHolder::PluginInOuts PluginInOuts;

    //==============================================================================
    /** Creates a window with a given title and colour.
        The settings object can be a PropertySet that the class should use to
        store its settings (it can also be null). If takeOwnershipOfSettings is
        true, then the settings object will be owned and deleted by this object.
    */
    MyStandaloneFilterWindow (const juce::String& title,
                            juce::Colour backgroundColour,
                            juce::PropertySet* settingsToUse,
                            bool takeOwnershipOfSettings,
                            const juce::String& preferredDefaultDeviceName = juce::String(),
                            const juce::AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                            const juce::Array<PluginInOuts>& constrainToConfiguration = {},
                           #if JUCE_ANDROID || JUCE_IOS
                            bool autoOpenMidiDevices = true
                           #else
                            bool autoOpenMidiDevices = false
                           #endif
                            )
        : juce::DocumentWindow (title, backgroundColour, juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton)
    {
//        setLookAndFeel (&LookAndFeel_frqz_rm);
       #if JUCE_IOS || JUCE_ANDROID
        setTitleBarHeight (0);
       #endif

        setUsingNativeTitleBar (true);

        pluginHolder.reset (new MyStandalonePluginHolder (settingsToUse, takeOwnershipOfSettings,
                                                        preferredDefaultDeviceName, preferredSetupOptions,
                                                        constrainToConfiguration, autoOpenMidiDevices));

       #if JUCE_IOS || JUCE_ANDROID
        setFullScreen (true);
        setContentOwned (new MainContentComponent (*this), false);
       #else
        setContentOwned (new MainContentComponent (*this), true);

        if (auto* props = pluginHolder->settings.get())
        {
            const int x = props->getIntValue ("windowX", -100);
            const int y = props->getIntValue ("windowY", -100);

            if (x != -100 && y != -100)
                setBoundsConstrained ({ x, y, getWidth(), getHeight() });
            else
                centreWithSize (getWidth(), getHeight());
        }
        else
        {
            centreWithSize (getWidth(), getHeight());
        }
       #endif
    }

    ~MyStandaloneFilterWindow() override
    {
        setLookAndFeel (nullptr);

       #if (! JUCE_IOS) && (! JUCE_ANDROID)
        if (auto* props = pluginHolder->settings.get())
        {
            props->setValue ("windowX", getX());
            props->setValue ("windowY", getY());
        }
       #endif

        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder = nullptr;
    }

    //==============================================================================
    juce::AudioProcessor* getAudioProcessor() const noexcept      { return pluginHolder->processor.get(); }
    juce::AudioDeviceManager& getDeviceManager() const noexcept   { return pluginHolder->deviceManager; }

    /** Deletes and re-creates the plugin, resetting it to its default state. */
    void resetToDefaultState()
    {
        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder->deletePlugin();

        if (auto* props = pluginHolder->settings.get())
            props->removeValue ("filterState");

        pluginHolder->createPlugin();
        setContentOwned (new MainContentComponent (*this), true);
        pluginHolder->startPlaying();
    }

    //==============================================================================
    void closeButtonPressed() override
    {
        pluginHolder->savePluginState();

        juce::JUCEApplicationBase::quit();
    }

    void buttonClicked (juce::Button*) override
    {
        juce::PopupMenu m;
        m.addItem (1, TRANS("Audio/MIDI Settings..."));
        m.addSeparator();
        m.addItem (2, TRANS("Save current state..."));
        m.addItem (3, TRANS("Load a saved state..."));
        m.addSeparator();
        m.addItem (4, TRANS("Reset to default state"));

//        m.setLookAndFeel (&LookAndFeel_frqz_rm);
        m.showMenuAsync (juce::PopupMenu::Options(),
                         juce::ModalCallbackFunction::forComponent (menuCallback, this));
    }

    void handleMenuResult (int result)
    {
        switch (result)
        {
            case 1:  pluginHolder->showAudioSettingsDialog(); break;
            case 2:  pluginHolder->askUserToSaveState(); break;
            case 3:  pluginHolder->askUserToLoadState(); break;
            case 4:  resetToDefaultState(); break;
            default: break;
        }
    }

    static void menuCallback (int result, MyStandaloneFilterWindow* button)
    {
        if (button != nullptr && result != 0)
            button->handleMenuResult (result);
    }

    void resized() override
    {
        DocumentWindow::resized();
    }

    virtual MyStandalonePluginHolder* getPluginHolder()    { return pluginHolder.get(); }

    std::unique_ptr<MyStandalonePluginHolder> pluginHolder;

private:
    //==============================================================================
    class MainContentComponent  : public juce::Component,
                                  private juce::Value::Listener,
                                  private juce::ComponentListener,
                                  private juce::Timer
    {
    public:
        MainContentComponent (MyStandaloneFilterWindow& filterWindow)
            : owner (filterWindow), header (&filterWindow),
              editor (owner.getAudioProcessor()->createEditorIfNeeded())
        {
            juce::Value& inputMutedValue = owner.pluginHolder->getMuteInputValue();

            if (editor != nullptr)
            {
                editor->addComponentListener (this);
                componentMovedOrResized (*editor, false, true);

                addAndMakeVisible (editor.get());
            }

            addAndMakeVisible (header);

            if (owner.pluginHolder->getProcessorHasPotentialFeedbackLoop())
            {
                inputMutedValue.addListener (this);
                shouldShowNotification = inputMutedValue.getValue();
            }

            inputMutedChanged (shouldShowNotification);

            startTimer (500);
        }

        ~MainContentComponent() override
        {
            if (editor != nullptr)
            {
                editor->removeComponentListener (this);
                owner.pluginHolder->processor->editorBeingDeleted (editor.get());
                editor = nullptr;
            }
        }

        void resized() override
        {
            auto r = getLocalBounds();
            header.setBounds (r.removeFromTop (Header::height));
            editor->setBounds (r);
        }

    private:
        //==============================================================================
        class Header : public juce::Component
        {
        public:
            enum { height = 18 };

            Header (juce::Button::Listener* settingsButtonListener)
                :
                 #if JUCE_IOS || JUCE_ANDROID
                  settingsButton ("Unmute Input")
                 #else
                  settingsButton ("Settings")
                 #endif
            {
//                setLookAndFeel (&LookAndFeel_frqz_rm);
                setOpaque (true);

                settingsButton.addListener (settingsButtonListener);

                addAndMakeVisible (lbMuted);
                lbMuted.setText ("INPUT MUTED", false);
                lbMuted.setTextColour (juce::Colours::red);

                addAndMakeVisible (settingsButton);
                settingsButton.setColour (juce::TextButton::buttonColourId, juce::Colours::cornflowerblue);
            }

            ~Header() override
            {
                setLookAndFeel (nullptr);
            }

            void paint (juce::Graphics& g) override
            {
                auto r = getLocalBounds();

//                g.setColour (LookAndFeel_frqz_rm.ClBackground);
                g.fillRect (r);
            }

            void resized() override
            {
                auto r = getLocalBounds();
                r.removeFromTop (2);
                r.removeFromLeft (2);
                r.removeFromRight (2);

                settingsButton.setBounds (r.removeFromRight (70));

                if (muted)
                    lbMuted.setBounds (r.removeFromLeft (80));

                if (jackDeviceName)
                    jackDeviceName->setBounds (r.removeFromLeft (200));
            }

            void setMuteStatus (bool muteStatus)
            {
                muted = muteStatus;
                lbMuted.setVisible (muted);
                resized();
                repaint();
            }

            void setJackClientName (juce::String jackClientName)
            {
                if (jackClientName.isEmpty())
                    jackDeviceName.reset();
                else
                {
                    jackDeviceName.reset (new SimpleLabel ("JACK Client: " + jackClientName));
                    addAndMakeVisible (jackDeviceName.get());
                }
                resized();
            }

        private:
            bool muted = false;
            SimpleLabel lbMuted;
            std::unique_ptr<SimpleLabel> jackDeviceName;
            juce::TextButton settingsButton;
//            LaF LookAndFeel_frqz_rm;
        };

        void timerCallback() override
        {
            auto* holder = owner.getPluginHolder();
            juce::String jackClientName = "";

            if (holder)
                jackClientName = holder->jackClientName;

            header.setJackClientName (jackClientName);
        }

        //==============================================================================
        void inputMutedChanged (bool newInputMutedValue)
        {
            header.setMuteStatus (newInputMutedValue);

           #if JUCE_IOS || JUCE_ANDROID
            resized();
           #else
            setSize (editor->getWidth(),
                     editor->getHeight()
                     + Header::height);
           #endif
        }

        void valueChanged (juce::Value& value) override     { inputMutedChanged (value.getValue()); }

        //==============================================================================
        void componentMovedOrResized (juce::Component&, bool, bool wasResized) override
        {
            if (wasResized && editor != nullptr)
                setSize (editor->getWidth(),
                         editor->getHeight() + Header::height);
        }

        //==============================================================================
        MyStandaloneFilterWindow& owner;
        Header header;
        std::unique_ptr<juce::AudioProcessorEditor> editor;
        bool shouldShowNotification = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
    };

    //==============================================================================
//    LaF LookAndFeel_frqz_rm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyStandaloneFilterWindow)
};

inline MyStandalonePluginHolder* MyStandalonePluginHolder::getInstance()
{
   #if JucePlugin_Enable_IAA || JucePlugin_Build_Standalone
    if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone)
    {
        auto& desktop = juce::Desktop::getInstance();
        const int numTopLevelWindows = desktop.getNumComponents();

        for (int i = 0; i < numTopLevelWindows; ++i)
            if (auto window = dynamic_cast<MyStandaloneFilterWindow*> (desktop.getComponent (i)))
                return window->getPluginHolder();
    }
   #endif

    return nullptr;
}
