#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(ValueTree &tree) :
        multiChannelSource(std::make_unique<MultiChannelAudioSource>(NUM_AUDIO_SOURCES)),
        xyController(NUM_AUDIO_SOURCES),
        jack(NUM_JACKTRIP_CHANNELS),
        valueTree(tree) {
    auto fg = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);

    addAndMakeVisible(xyController);
    xyController.onValueChange = [this](uint nodeIndex, Point<float> position) {
        valueTree.setProperty("/source/" + String{nodeIndex} + "/x", position.x, nullptr);
        valueTree.setProperty("/source/" + String{nodeIndex} + "/y", position.y, nullptr);
    };
    xyController.onAddNode = [this](uint nodeIndex) { addSource(nodeIndex); };
    xyController.onRemoveNode = [this](uint nodeIndex) { removeSource(nodeIndex); };

    for (uint i = 0; i < NUM_MODULES; ++i) {
        auto cb{new ComboBox};
        addAndMakeVisible(cb);
        cb->onChange = [this, cb, i] {
            auto ip{cb->getText()};
            valueTree.setProperty("/module/" + String(i), ip, nullptr);
        };
        cb->setColour(ComboBox::textColourId, fg);
        cb->setColour(ComboBox::arrowColourId, fg);
        cb->setColour(ComboBox::backgroundColourId, Colours::whitesmoke);
        moduleSelectors.add(cb);
    }

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Settings");
    settingsButton.onClick = [this] { showSettings(); };
    settingsButton.setColour(TextButton::textColourOnId, fg);
    settingsButton.setColour(TextButton::textColourOffId, fg);
    settingsButton.setColour(TextButton::buttonColourId, Colours::white);
    settingsButton.setColour(TextButton::buttonOnColourId, Colours::ghostwhite);

    addAndMakeVisible(connectToModulesButton);
    connectToModulesButton.setButtonText("Refresh ports");
    connectToModulesButton.onClick = [this] { refreshDevicesAndPorts(); };
    connectToModulesButton.setColour(TextButton::textColourOffId, fg);
    connectToModulesButton.setColour(TextButton::textColourOnId, fg);
    connectToModulesButton.setColour(TextButton::buttonColourId, Colours::white);
    connectToModulesButton.setColour(TextButton::buttonOnColourId, Colours::ghostwhite);

    addAndMakeVisible(gainSlider);
    gainSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    gainSlider.setColour(Slider::thumbColourId, fg.withLightness(.66f));
    gainSlider.setNormalisableRange({0., 1.25, .01});
    gainSlider.setValue(1.);
    gainSlider.onValueChange = [this] { multiChannelSource->setGain(static_cast<float>(gainSlider.getValue())); };
    gainSlider.setTextBoxStyle(Slider::TextBoxRight, false, gainSlider.getTextBoxWidth() * .75f,
                               gainSlider.getTextBoxHeight());
    gainSlider.setColour(Slider::textBoxTextColourId, fg);

    addAndMakeVisible(gainLabel);
    gainLabel.attachToComponent(&gainSlider, true);
    gainLabel.setText("Gain", dontSendNotification);
    gainLabel.setColour(Label::textColourId, fg);

    setSize(1080, 800);

    setAudioChannels(0, NUM_AUDIO_SOURCES);
    refreshDevicesAndPorts();
}

void MainComponent::refreshDevicesAndPorts() {
    // Use JACK for output
    if (deviceManager.getCurrentAudioDeviceType() != "JACK") {
        multiChannelSource->stop();

        auto setup{deviceManager.getAudioDeviceSetup()};
        setup.bufferSize = AUDIO_BLOCK_SAMPLES;
        setup.useDefaultOutputChannels = false;
        setup.outputChannels = NUM_AUDIO_SOURCES;
        auto &deviceTypes{deviceManager.getAvailableDeviceTypes()};
        // TODO: warn if JACK not found...
        for (auto type: deviceTypes) {
            auto typeName{type->getTypeName()};
            if (typeName == "JACK") {
                deviceManager.setCurrentAudioDeviceType(typeName, true);

                // Preliminarily connect to the dummy jack client
                type->scanForDevices();
                auto names{type->getDeviceNames()};
                if (names.contains(jack.getClientName(), true)) {
                    setup.outputDeviceName = jack.getClientName();
                    // Should really check for an error string here.
                    auto result{deviceManager.setAudioDeviceSetup(setup, true)};
                    break;
                }
            }
        }

        multiChannelSource->start();
    }

    jack.connect();

    auto clients{jack.getJackTripClients()};
    // Update the module selector lists.
    for (auto *selector: moduleSelectors) {
        // Cache the current value.
        auto text{selector->getText()};
        // Refresh the list
        selector->clear();
        selector->addItemList(clients, 1);
        // Try to restore the old value.
        for (int i = 0; i < clients.size(); ++i) {
            if (clients[i] == text) {
                selector->setSelectedItemIndex(i);
            }
        }
    }
}

void MainComponent::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::ghostwhite);
}

void MainComponent::resized() {
    auto bounds{getLocalBounds()};
    auto padding{5}, xyPadX{75}, xyPadY{100};
    xyController.setBounds(xyPadX, xyPadY / 2, bounds.getWidth() - 2 * xyPadX, bounds.getHeight() - 2 * xyPadY);
    auto moduleSelectorWidth{static_cast<float>(xyController.getWidth() + 1) / static_cast<float>(NUM_MODULES)};
    for (int i{0}; i < moduleSelectors.size(); ++i) {
        moduleSelectors[i]->setBounds(
                xyPadX + i * moduleSelectorWidth,
                xyController.getBottom() + padding,
                moduleSelectorWidth - 1,
                30
        );
    }
    settingsButton.setBounds(padding, bounds.getBottom() - padding - 20, 60, 20);
    connectToModulesButton.setBounds(xyController.getRight() - 100, xyController.getY() - 25, 100, 20);
    gainSlider.setBounds(xyController.getX() + 35, xyController.getY() - 25, 250, 20);
}

void MainComponent::showSettings() {
    DialogWindow::LaunchOptions settings;
    auto deviceSelector = new AudioDeviceSelectorComponent(
            deviceManager,
            0,     // minimum input channels
            256,   // maximum input channels
            0,     // minimum output channels
            256,   // maximum output channels
            false, // ability to select midi inputs
            false, // ability to select midi output device
            false, // treat channels as stereo pairs
            false // hide advanced options
    );

    settings.content.setOwned(deviceSelector);

    Rectangle<int> area(0, 0, 600, 400);

    settings.content->setSize(area.getWidth(), area.getHeight());

    settings.dialogTitle = "Audio Settings";
    settings.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    settings.escapeKeyTriggersCloseButton = true;
    settings.useNativeTitleBar = true;
    settings.resizable = false;

    settingsWindow = settings.launchAsync();

    if (settingsWindow != nullptr)
        settingsWindow->centreWithSize(600, 400);
}

MainComponent::~MainComponent() {
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRateReported) {
    multiChannelSource->prepareToPlay(samplesPerBlockExpected, sampleRateReported);
}

void MainComponent::releaseResources() {
    multiChannelSource->releaseResources();
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    multiChannelSource->getNextAudioBlock(bufferToFill);
}

void MainComponent::addSource(uint sourceIndex) {
    fileChooser = std::make_unique<FileChooser>("Select an audio file",
                                                File("~/Documents"),
                                                "*.wav;*.aiff;*.flac;*.ogg");
    fileChooser->launchAsync(
            FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
            [this, sourceIndex](const FileChooser &chooser) {
                auto file = chooser.getResult();
                if (file != File{}) {
                    multiChannelSource->addSource(sourceIndex, file);
                    multiChannelSource->start();
                } else {
                    xyController.removeNode(sourceIndex);
                }
            }
    );
}

void MainComponent::removeSource(uint sourceIndex) {
    multiChannelSource->removeSource(sourceIndex);
}
