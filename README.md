# teensy-wfs
Distributed WFS implementation for networked Teensies

## WFS Control Application

There is a WFS Controller JUCE app in [wfs-controller](wfs-controller). 
This uses JACK's C API to set up ports and connect to any
Teensy JackTrip clients. It notifies the clients of their positions in the
WFS array, plus the positions of virtual sound sources, via OSC over UDP
multicast.

![WFS Controller](notes/wfs-controller.png)