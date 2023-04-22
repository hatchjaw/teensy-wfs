# WFS Controller

Requires cmake and JUCE.

Developed with JUCE v7.0.5; builds with JUCE at least as far back as v6.1.6.

| OS       | Builds | Runs |
|----------|:------:|:----:|
| Linux    |   ✅    |  ✅   |
| Mac OS X |   ✅    |  ❗*  |
| Windows  |   ❔    |  ❔   |

&ast; Relies on the availability at runtime of 
[JACK](https://jackaudio.org) as an audio host/output device. 
Although it's possible to install JACK on Mac OS X, JackRouter,
the bridge from JACK to Core Audio, hasn't worked since 
OS X 10.12 (see 
[here](https://github.com/jackaudio/jack2/issues/618) and various
other issues in the jack2 repository.)

The app uses the JACK API to make connections between itself and 
any available JackTrip clients, but this is contingent on 
establishing the app itself as a device in the JACK graph; 
via JUCE on OS X, this is not possible.

## JUCE

The build is configured via JUCE's cmake API. Kindly consult the
[Getting Started](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#getting-started) 
section of JUCE's cmake documentation.

[CMakeLists.txt](CMakeLists.txt) is set up to use the `find_package` method.
If you prefer the `add_subdirectory` method, add JUCE as a subdirectory here, 
and modify `CMakeLists.txt` as follows:

```cmake
# Comment the following two lines
#list(APPEND CMAKE_PREFIX_PATH ${JUCE_INSTALL_PATH})
#find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# And uncomment this one:
add_subdirectory(JUCE)                    # If you've put JUCE in a subdirectory called JUCE
```

---

## Build

Configure:

```shell
cmake -DCMAKE_BUILD_TYPE=Debug -DJUCE_INSTALL_PATH=/path/to/juce -B ./build-dir
```
Set `-DCMAKE_BUILD_TYPE` to `Release` for a release-optimised build.

Then build:

```shell
cmake --build ./build-dir
```

---

## Run

The app UI is essentially a multiple-node XY controller representing a WFS
virtual sound field and an array of secondary point sources, represented by
the dropdown menus at the bottom.

On launch, the app will attempt to query JACK for any JackTrip clients, which
it identifies by the pattern of the device name they're assigned by JackTrip
when they make contact with the server, i.e. `__ffff_[IPv4 address]`. The
IPv4 part of the device names is extracted and used to populate the 
dropdown lists. (Yes, _any_ JackTrip client found will make its way into the 
dropdowns.) Select IPs according to the positions of your modules. Click
**Refresh Ports** to rescan and update the lists.

Right-click the creamy void to add a node; you will be prompted to select an 
audio file to associate with the new node. If you select a multichannel audio 
file, only the first channel will be used. Once selected, the file will begin
to play, and will loop for as long as the node exists; JackTrip will stream
the audio data to all connected clients.

Left-click (and drag) a node to move the corresponding sound source around the
sound field. Right click a node to remove it.

The **Gain** control is a master volume for all sound sources. There's no way
at present to change the level of individual sources; for that you'll need to 
preprocess your sources in some other application.

There's very little to be gained by clicking the **Settings** button, other
than to verify that the app is using JACK as its audio host, and perhaps 
that the buffer size is set correctly; both of these things should happen 
automatically on launch, assuming that JACK is installed and `jackd` running on
the server, which it has to be in order for JackTrip to be running.