declare name "Distributed WFS";
declare description "Basic WFS for a distributed setup consisting of modules that each handle two output channels.";
import("stdfaust.lib");
import("WFS_Params.lib");

// Set which speakers to control.
moduleID = hslider("moduleID", 0, 0, (N_SPEAKERS / SPEAKERS_PER_MODULE) - 1, 1);

// Simulate distance by changing gain and applying a lowpass as a function
// of distance
distanceSim(distance) = *(dGain) : fi.lowpass(2, fc)
with{
    // Use inverse square law; I_2/I_1 = (d_1/d_2)^2
    // Assume sensible listening distance of 5 m from array.
    i1 = 1.; // Intensity 1...
    d1 = 5.; // ...at distance 5 m
    d2 = d1 + distance;
    i2 = i1 * (d1/d2)^2; //
    dGain = i2;
    // dGain = (MAX_Y_DIST - distance*.5)/(MAX_Y_DIST);

    fc = dGain*15000 + 5000;
};

// Create a speaker array *perspective* for one source
// i.e. give each source a distance simulation and a delay
// relative to each speaker.
speakerArray(x, y) = _ <:
    par(i, SPEAKERS_PER_MODULE, distanceSim(hypotenuse(i)) : 
    de.fdelay2(MAX_DELAY, smallDelay(i)))
with{
    // y (front-to-back) is always just y, the longitudinal
    // distance of the source from the array.
    // Get x between the source and specific speaker in the array
    // E.g. for 16 speakers (8 modules), with a spacing, s, of .25 m,
    //      array width, w = (16-1)*.25 = 3.75,
    //        let module m = 2 (third module in array)
    //       let speaker j = 0 (first speaker in module)
    //               let x = 2.25 (m, relative to left edge of array)
    //                  cx = x - s*(m*2 + j)
    //                     = 2.25 - .25*(2*2 + 0)
    //                     = 1.25
    //
    //               let m = 7, j = 1, x = 2.25
    //                  cx = 2.25 - .25*(7*2 + 1) = -1.5
    //
    //               let m = 0, j = 0, x = 2.25
    //                  cx = 2.25 - .25*(0*2 + 0) = 2.25
    cathetusX(k) = x - (SPEAKER_DIST*(k + moduleID*2));
    hypotenuse(j) = cathetusX(j)^2 + y^2 : sqrt;
    smallDelay(j) = (hypotenuse(j) - y)*SAMPLES_PER_METRE;
};

// Take each source...
sourcesArray(s) = par(i, ba.count(s), ba.take(i + 1, s) :
    // ...and distribute it across the speaker array for this module.
    speakerArray(x(i), y(i)))
    // Merge onto the output speakers.
    :> par(i, SPEAKERS_PER_MODULE, _)
with{
    // Use normalised input co-ordinate space; scale to dimensions.

    // X position lies on the width of the speaker array
    // x(p) = hslider("%p/x", 0, 0, 1, 0.001) : si.smoo : *(SPEAKER_DIST*N_SPEAKERS);
    // TODO: smooth the incoming OSC data on Teensy instead
    x(p) = hslider("%p/x", 0, 0, 1, 0.001) : *(SPEAKER_DIST*N_SPEAKERS);
    // Y position is from zero (on the array) to a quasi-arbitrary maximum.
    // y(p) = hslider("%p/y", 0, 0, 1, 0.001) : si.smoo : *(MAX_Y_DIST);
    y(p) = hslider("%p/y", 0, 0, 1, 0.001) : *(MAX_Y_DIST);
};

// Distribute input channels (i.e. sources) across the sources array.
process = sourcesArray(par(i, N_SOURCES, _));
