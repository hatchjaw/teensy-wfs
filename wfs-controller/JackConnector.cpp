//
// Created by tar on 21/11/22.
//

#include "JackConnector.h"

JackConnector::JackConnector(uint numPortsToRegister) : numPorts(numPortsToRegister) {
    openClient();
}

void JackConnector::openClient() {
    jack_status_t status;
    client = jack_client_open(CONNECTOR_CLIENT_NAME, JackNoStartServer, &status);
    if (client == nullptr) {
        fprintf(stderr, "jack_client_open() failed, "
                        "status = 0x%2.0x\n", status);
    }
    for (uint i = 1; i <= numPorts; ++i) {
        char portName[6];
        sprintf(portName, "in_%d", i);
        jack_port_register(client, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    }
    jack_activate(client);
}

JackConnector::~JackConnector() {
    jack_client_close(client);
}

void JackConnector::connect() {
    if (client == nullptr) {
        openClient();
    }

    const char **inPorts, **outPorts;

    // Get available jacktrip output devices (i.e. those with input ports);
    char inPattern[50], outPattern[50]; // Agh
    sprintf(inPattern, JACKTRIP_IN_PORT_FORMAT, 1);
    outputDevices = jack_get_ports(client, inPattern, nullptr, JackPortIsInput);

    // Connect each output of this app to the corresponding input for each
    // JackTrip client.
    auto i{1};
    do {
        sprintf(outPattern, WFS_OUT_PORT_FORMAT, i);
        sprintf(inPattern, JACKTRIP_IN_PORT_FORMAT, i);
        outPorts = jack_get_ports(client, outPattern, nullptr, JackPortIsOutput);
        inPorts = jack_get_ports(client, inPattern, nullptr, JackPortIsInput);

        for (int ch = 0; outPorts && inPorts && inPorts[ch] != nullptr; ch++) {
            DBG("Connecting " << outPorts[0] << " to " << inPorts[ch]);
            // Should check whether ports are already connected.
            jack_connect(client, outPorts[0], inPorts[ch]);
        }
        jack_free(outPorts);
        jack_free(inPorts);
        ++i;
    } while (outPorts != nullptr);

    jack_deactivate(client);
}

StringArray JackConnector::getJackTripClients() {
    StringArray clients{};
    for (int i = 0; outputDevices && outputDevices[i] != nullptr; ++i) {
        // Quick/dirty extraction of an IP from jack port name of the form:
        // __ffff_<IP>:<send|receive>_<n>
        auto device = String{outputDevices[i]}
                .upToFirstOccurrenceOf(":", false, true)
                .fromLastOccurrenceOf("_", false, true);
        clients.add(device);
    }
    std::sort(clients.begin(), clients.end(),
              [](const String &a, const String &b) {
                  auto A{std::stoi(a.fromLastOccurrenceOf(".", false, true).toStdString())},
                          B{std::stoi(b.fromLastOccurrenceOf(".", false, true).toStdString())};
                  return B > A;
              });
    return clients;
}

String JackConnector::getClientName() {
    return String{CONNECTOR_CLIENT_NAME};
}
