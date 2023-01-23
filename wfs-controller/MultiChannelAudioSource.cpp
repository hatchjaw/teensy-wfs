//
// Created by tar on 23/11/22.
//

#include "MultiChannelAudioSource.h"

MultiChannelAudioSource::MultiChannelAudioSource(uint maxNumSources) : maxSources(maxNumSources) {
    formatManager.registerBasicFormats();
}

void MultiChannelAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRateToUse) {
    tempBuffer.setSize(NUM_AUDIO_SOURCES, samplesPerBlockExpected);

    const ScopedLock sl{lock};

    blockSize = samplesPerBlockExpected;
    sampleRate = sampleRateToUse;

    isPrepared = true;
}

void MultiChannelAudioSource::releaseResources() {
    const ScopedLock sl{lock};

    for (auto &source: sources) {
        source.second->releaseResources();
    }

    sources.clear();

    tempBuffer.setSize(2, 0);

    isPrepared = false;
}

void MultiChannelAudioSource::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    const ScopedLock sl{lock};

    if (!sources.empty() && !stopped) {
        auto numChannels{static_cast<uint>(bufferToFill.buffer->getNumChannels())};
        for (auto &source: sources) {
            // Prevent doomed attempts to write to channels that don't exist.
            if (source.first < numChannels) {
                auto writePointer = bufferToFill.buffer->getWritePointer(static_cast<int>(source.first));
                // Set up a single-channel temp buffer.
                tempBuffer.setDataToReferTo(&writePointer, 1, bufferToFill.buffer->getNumSamples());
                // Combine it with the incoming channel info
                AudioSourceChannelInfo channelInfo(&tempBuffer, bufferToFill.startSample, bufferToFill.numSamples);
                // Write the next block of the audio file.
                source.second->getNextAudioBlock(channelInfo);
            }
        }

        if (!playing) {
            DBG("MultiChannelAudioSource: Just stopped playing...");
            // just stopped playing, so fade out the last block...
            for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;)
                bufferToFill.buffer->applyGainRamp(i, bufferToFill.startSample, jmin(256, bufferToFill.numSamples),
                                                   1.0f, 0.0f);

            if (bufferToFill.numSamples > 256)
                bufferToFill.buffer->clear(bufferToFill.startSample + 256, bufferToFill.numSamples - 256);
        }

        if (!sources.empty()
            && sources.begin()->second->getNextReadPosition() > sources.begin()->second->getTotalLength() + 1
            && !sources.begin()->second->isLooping()) {
            playing = false;
            sendChangeMessage();
        }

        stopped = !playing;

        for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;) {
            bufferToFill.buffer->applyGainRamp(i, bufferToFill.startSample, bufferToFill.numSamples, lastGain, gain);
        }
    } else {
        bufferToFill.clearActiveBufferRegion();
        stopped = true;
    }

    lastGain = gain;
}

void MultiChannelAudioSource::setNextReadPosition(int64 newPosition) {
    for (auto &source: sources) {
        source.second->setNextReadPosition(newPosition);
    }
}

int64 MultiChannelAudioSource::getNextReadPosition() const {
    if (sources.empty()) {
        return 0;
    }

    return sources.find(0)->second->getNextReadPosition();
}

int64 MultiChannelAudioSource::getTotalLength() const {
    const ScopedLock sl{lock};

    if (sources.empty()) {
        return 0;
    }

    int64 length{0};
    for (auto &source: sources) {
        if (source.second->getTotalLength() > length) {
            length = source.second->getTotalLength();
        }
    }
    return length;
}

bool MultiChannelAudioSource::isLooping() const {
    const ScopedLock sl{lock};

    return !sources.empty() && sources.find(0)->second->isLooping();
}

void MultiChannelAudioSource::addSource(uint index, File &file) {
    if (!canAddSource()) {
        DBG("MultiChannelAudioSource: already full of sources.");
        return;
    }
    if (auto *reader = formatManager.createReaderFor(file)) {
        DBG("MultiChannelAudioSource: Adding source with ID " << sources.size());
        auto source = std::make_unique<AudioFormatReaderSource>(reader, true);
        source->setLooping(true);
        sources.insert(std::make_pair(index, std::move(source)));
    } else {
        jassertfalse;
    }
}

void MultiChannelAudioSource::removeSource(uint index) {
    const ScopedLock sl{lock};

    DBG("MultiChannelAudioSource: Removing source with ID " << String(index));

    auto source{sources.find(index)};
    if (source != sources.end()) {
        source->second->releaseResources();
        sources.erase(source);
    }

    if (sources.empty()) {
        stopped = true;
        stop();
    }
}

void MultiChannelAudioSource::start() {
    if (!sources.empty() && !playing) {
        {
            const ScopedLock sl{lock};
            playing = true;
            stopped = false;
        }

        sendChangeMessage();
    }
}

void MultiChannelAudioSource::stop() {
    if (playing) {
        playing = false;

        int n = 500;
        while (--n >= 0 && !stopped)
            Thread::sleep(2);

        sendChangeMessage();
    }
}

bool MultiChannelAudioSource::canAddSource() {
    return maxSources == 0 || sources.size() < maxSources;
}

void MultiChannelAudioSource::setGain(float newGain) {
    lastGain = gain;
    gain = newGain;
}