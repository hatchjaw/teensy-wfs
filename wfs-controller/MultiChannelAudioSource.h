//
// Created by tar on 23/11/22.
//

#ifndef JACKTRIP_TEENSY_MULTICHANNELAUDIOSOURCE_H
#define JACKTRIP_TEENSY_MULTICHANNELAUDIOSOURCE_H

#include <JuceHeader.h>
#include "Utils.h"

class MultiChannelAudioSource : public PositionableAudioSource, public ChangeBroadcaster {
public:
    explicit MultiChannelAudioSource(uint maxNumSources = 0);

    void prepareToPlay(int samplesPerBlockExpected, double sampleRateToUse) override;

    void releaseResources() override;

    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override;

    void setNextReadPosition(int64 newPosition) override;

    int64 getNextReadPosition() const override;

    int64 getTotalLength() const override;

    bool isLooping() const override;

    void addSource(uint index, File &file);

    void removeSource(uint index);

    void start();

    void stop();

    void setGain(float newGain);

private:
    bool canAddSource();

    AudioFormatManager formatManager;
    std::unordered_map<uint, std::unique_ptr<AudioFormatReaderSource>> sources;
    AudioBuffer<float> tempBuffer;

    CriticalSection lock;

    int blockSize{0};
    double sampleRate{0.};
    float gain{1.0f}, lastGain{1.0f};
    std::atomic<bool> playing{false}, stopped{true};
    bool isPrepared = false;
    uint maxSources;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiChannelAudioSource)
};


#endif //JACKTRIP_TEENSY_MULTICHANNELAUDIOSOURCE_H
