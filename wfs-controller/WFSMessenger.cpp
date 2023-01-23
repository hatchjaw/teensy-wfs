//
// Created by tar on 13/11/22.
//

#include "WFSMessenger.h"

WFSMessenger::WFSMessenger(ValueTree &tree) :
        socket(std::make_unique<DatagramSocket>()),
        valueTree(tree) {
    valueTree.addListener(this);
}

WFSMessenger::~WFSMessenger() {
    valueTree.removeListener(this);
}

void WFSMessenger::connect() {
    // Prepare to send OSC messages over UDP multicast.
    // Got to bind to the local address of the appropriate network interface.
    // (This will only work if ethernet is attached with the below IPv4 assigned.)
    // TODO: make these specifiable via the UI
    socket->bindToPort(8888, "192.168.10.10");
    // TODO: also make multicast IP and port specifiable via the UI.
    connectToSocket(*socket, "230.0.0.20", 41814);
}

void WFSMessenger::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    ignoreUnused(treeWhosePropertyHasChanged);

    OSCBundle bundle;

    if (property.toString().contains("module")) {
//        DBG("Sending OSC: " << property.toString() << " " << valueTree.getProperty(property).toString());
        bundle.addElement(OSCMessage{property.toString(), valueTree.getProperty(property).toString()});
    } else {
//        DBG("Sending OSC: " << property.toString() << " " << static_cast<float>(valueTree.getProperty(property)));
        bundle.addElement(OSCMessage{property.toString(), static_cast<float>(valueTree.getProperty(property))});
    }

    send(bundle);
}


