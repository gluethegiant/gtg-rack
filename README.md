# Glue the Giant Rack Modules for VCV Rack

Mixing a VCV Rack patch to stereo output is a common task.
You can use monolithic rack modules that mimic real-world mixing consoles, but they don't take advantage of the power or flexibility inherent in a modular environment.
And other modular mixers tend to make it complicated to route multiple sounds to send effects.

The Glue the Giant modular bus mixers turn a polyphonic cable into three stereo mix buses.
The bus design makes it easy to patch together modules while creating simple or complex effect sends and returns.

![alt text](https://github.com/gluethegiant/gtg-rack/blob/master/design/screenshot.png)

## The Modular Bus Mixer Suite

### 1. Mini Bus Mixer

This mixer strip uses the least amount of real estate and is perfect for sounds that sit steady in the middle of your mix, like a kick or a bass.

* One mono or polyphonic input
* Level controls to three stereo buses
* On button with CV input and pop filter

### 2. Gig Bus Mixer

A mixer strip with standard features, but without the CV inputs or the extra real estate of the School Bus Mixer.

* Stereo, mono, or polyphonic input
* Constant power pan control
* Level controls to three stereo buses
* Blue and orange levels are post red level (red is master bus) 
* On button with CV input and pop filter

### 3. School Bus Mixer

Use this mixer strip for any sound in your mix.
With CV inputs on almost everything, you can connect LFOs, master control knobs, envelop generators, and more.

* Stereo, mono, or polyphonic input
* Constant power pan control with CV input and attenuator
* Pan CV input expects bi-direction LFO sine wave from -5.0 to 5.0 
* Three level controls, with CV inputs, to three stereo buses
* Post fader option on two level controls (red becomes master bus)
* On button with CV input and pop filter

Note that panning rolls back seamlessly when the pan CV input, attenuator, and pan knob combine to be less than -1 or greater than 1.

### 4. Metro City Bus Mixer

This mixer can take a polyphonic input and spread the channels across the stereo field.
The pan knob controls the placement of the first channel from the polyphonic input (or the last channel if the REVERSE CHANNELS button is on).
The spread knob evenly spaces the other polyphonic channels to the left or to the right.
The LED indicators give you a dynamic indication of the stereo spread.
Pan is smoothed to allow for a dynamic number of polyphonic channels.
Automate the pan with an LFO on the CV input for a polyphonic pan follow that can be adjusted with the attenuator and spread knobs.

* One polyphonic input
* Polyphonic stereo spread with LED visuals
* Constant power pan control with CV input and attenuator
* Pan CV input expects bi-direction LFO sine wave from -5.0 to 5.0 
* Reverse channel order in polyphonic spread or pan follow
* Three level controls, with CV inputs, to three stereo buses
* Post fader option on two level controls (red becomes master bus)
* On button with CV input and pop filter

### 5. Bus Route

Connects standard effect sends and returns to your buses

* Three stereo sends for use with modular bus mixers
* Three stereo returns for use with modular bus mixers

Note: manually connect outputs to inputs on buses that need pass through.
To merge the audio of sends and returns onto buses, the Exit Bus and Enter Bus modules can be used.

### 6. Enter Bus

For advanced routing, this utility module merges audio onto any bus.

* Three stereo inputs to three stereo buses

### 7. Exit Bus

For advanced routing, this utility module provides stereo outputs from all buses.

* Three stereo outputs from three stereo buses

### 8. Bus Depot

This module is placed at the end of your bus chains.
The stereo outputs can be connected to your left and right audio outputs.
The aux input can be used to chain Bus Depot modules or another mixer.

* Sums three stereo buses to the left and right stereo mix
* Master level control
* Aux input that accepts stereo, mono, or polyphonic cables
* Aux level control
* Left and right peak meters with a brief hold on peaks over 0db
* Each meter light represents -6db.

## Copyrights, Trademarks, Licenses, Etc.

"Glue the Giant" is the name of a band and a software developer.
Do your best to not steal it (we know it's cool).
The designs help to give these modules a unique look.
Try to not copy them too closely.
All the code is released to use under the GPL 3 license and above.

## Building from Source

To build these rack modules, see the official [VCV Rack documentation](https://vcvrack.com/manual/Building.html).

## Release Notes

v. 1.0.1 Metro City Bus Release (TBD)

- Added Metro City Bus Mixer with polyphonic stereo spread
- Added Gig Bus Mixer for easy, standard mixing
- Minor CPU optimizations
- Small UI enhancements

v. 1.0.0 Initial VCV Rack Library Release (September 6, 2019)

- Peak indicators on Bus Depot are now sample accurate
- Peak indicators stay on longer
- Text fixed on Bus Route

v. 1.0.0 Initial Release (September 3, 2019)

- Mini Bus Mixer, School Bus Mixer, and Bus Depot modules released
- Bus Route, Enter Bus, and Exit Bus utility modules for bus mixers released
