//
// Created by tar on 13/11/22.
//

#ifndef JACKTRIP_TEENSY_WFSMESSENGER_H
#define JACKTRIP_TEENSY_WFSMESSENGER_H

#include <JuceHeader.h>

class WFSMessenger : public OSCSender, public ValueTree::Listener {
public:
    explicit WFSMessenger(ValueTree &tree);

    ~WFSMessenger() override;

    void connect();

    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;

private:
    std::unique_ptr<juce::DatagramSocket> socket;
    ValueTree &valueTree;
};


#endif //JACKTRIP_TEENSY_WFSMESSENGER_H
