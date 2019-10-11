# Glue the Giant Rack Modules for VCV Rack

Mixing a VCV Rack patch to stereo output is a common task.
You can use monolithic rack modules that mimic real-world mixing consoles, but they don't take advantage of the power or flexibility inherent in a modular environment.
And other modular mixers tend to make it complicated to route multiple sounds to send effects.

The modular bus mixers from Glue the Giant allow you to choose the right mixer strip, decide where to place each strip, and easily create mix groups.
The modules also provide features like polyphonic stereo spread and latency compensation on send effects.

To use these mixers, simply connect them together with the BUS IN and BUS OUT ports.
The audio is routed along three stereo buses: red, orange, and blue.
The bus design makes it easy to patch together modules while creating simple or complex effect sends and returns.

![alt text](https://github.com/gluethegiant/gtg-rack/blob/master/design/screenshot.png)

## The Modular Bus Mixers

### 1. Mini Bus Mixer

This mixer strip uses the least amount of real estate and is perfect for sounds that sit steady in the middle of your mix, like a kick or a bass.

* One mono or polyphonic input
* Level controls to three stereo buses
* 2x or 4x preamp-style gain on input (selected from the context menu)
* On button with CV input and pop filter

### 2. Gig Bus Mixer

A mixer strip configured as a master bus and two post fader send buses, but without the CV inputs or the extra real estate of the School Bus Mixer.

* Stereo, mono, or polyphonic input
* Constant power pan control
* Level controls to three stereo buses
* Blue and orange levels are post red level (red is the master bus) 
* 2x or 4x preamp-style gain on inputs (selected from the context menu)
* On button with CV input and pop filter

### 3. School Bus Mixer

Use this mixer strip for any sound in your mix.
With CV inputs on almost everything, you can connect LFOs, master control knobs, envelop generators, and more.

* Stereo, mono, or polyphonic input
* Constant power pan control with CV input and attenuator
* Pan CV input expects bi-directional source from -5.0 to 5.0 
* Three level controls, with CV inputs, to three stereo buses
* Post fader option on two level controls (red becomes the master bus)
* 2x or 4x preamp-style gain on inputs (selected from the context menu)
* On button with CV input and pop filter

Note that panning rolls back seamlessly when the pan CV input, attenuator, and pan knob overload the pan circuit.

### 4. Metro City Bus Mixer

This mixer can take a polyphonic input and spread the channels evenly across the stereo field.
The pan knob controls the placement of the first channel from the polyphonic input (or the last channel if the REVERSE CHANNELS button is on).
The spread knob spaces the other polyphonic channels to the left or to the right.
The LED indicators give you a visual indication of the stereo spread.

Automate the pan with an LFO on the CV input for a polyphonic pan follow that can be adjusted with the attenuator and spread knobs.
Pan spread and pan follow are smoothed to allow for dynamic polyphonic channels.

* One polyphonic input
* Polyphonic stereo spread with LED visuals
* Constant power pan control with CV input and attenuator
* Pan CV input expects bi-directional source from -5.0 to 5.0 
* Reverse channel order on polyphonic spread or pan follow
* Three level controls, with CV inputs, to three stereo buses
* Post fader option on two level controls (red becomes master bus)
* 2x or 4x preamp-style gain on polyphonic input (selected from context menu)
* On button with CV input and pop filter

### 5. Bus Route

Connects standard effect sends and returns to your buses.

Bus Route also contains an optional integrated sample delay on each send.
These delays can be used for latency compensation.
For example, most effect modules in VCV Rack will create a latency of only one sample.
If you route to an effect on the orange bus, you could add a one sample delay to your red bus to keep your audio in perfect sync.
This latency compensation of one sample is hardly noticeable, but some effects, including external plugins, can require latency compensation to prevent unwanted phase or other issues.
When latency is not documented by a plugin, a scope can be used to detect and correct for latency.

The integrated sample delays can also be used to create phase effects, millisecond delays, or subtle time shifts on a bus or a mix group.
These delays are limited to 999 samples so they can be light on resources and set easily (use CTRL to improve the accuracy of a VCV Rack knob).

* Three stereo sends for use with modular bus mixers
* Three stereo returns for use with modular bus mixers
* Three sample delay lines, one on each stereo bus send, for up to 999 samples of latency compensation

Note: manually connect outputs to inputs on all buses that need pass through.
To merge the audio of sends and returns onto buses, the Exit Bus and Enter Bus modules can be used.

### 6. Enter Bus

For advanced routing or loop backs from different buses, this utility module merges audio onto any bus.

* Three stereo inputs to three stereo buses

### 7. Exit Bus

For advanced routing or a simple extension panel to expose the outputs of any modular bus mixer module.

* Three stereo outputs from three stereo buses

### 8. Bus Depot

This module is placed at the end of your bus mixer chains.
The stereo outputs can then be connected to the left and right channels on your audio device.

The provided aux inputs can be used to chain Bus Depot modules or to attached any other sound source.
Mixer groups are created by using a different Bus Depot module after each group of mixers.

Bus Depot has an on button that controls an auto fader on the output.
By changing the speed knob, the fader can go from providing a 20 millisecond pop filter to a long fade of up to 17 seconds.

* Sums three stereo buses to the left and right stereo outputs
* Master level control with CV
* Aux input that accepts stereo, mono, or polyphonic cables
* Aux level control
* Left and right peak meters with a brief hold on peaks over 0dB
* Each meter light represents -6dB
* Output on/off button with a linear fader that can be a pop filter or a fade up to 17 seconds
* Auto fader speed knob with CV

## More Information

For more information about these rack modules, see the [Wiki](https://github.com/gluethegiant/gtg-rack/wiki).
You can also open an issue or contact the author on the VCV Rack Community forum.

## Copyrights, Trademarks, Licenses, Etc.

"Glue the Giant" is the name of a band and a software developer.
Do your best to not steal it (we know it's cool).
The designs help to give these modules a unique look.
Try to not copy them too closely.
All the code is released to use under the GPL 3 license and above.

## Building from Source

To build these rack modules, see the official [VCV Rack documentation](https://vcvrack.com/manual/Building.html).

## Release Notes

v. 1.0.2 The Tick Release (October 12, 2019)

- Added 2x or 4x preamp-style gain on all four mixer strips (selected from the context menu)
- New sample delay lines provide latency compensation on Bus Route
- New auto fader with speed control on Bus Depot
- Improved performance on Metro City Bus Mixer's polyphonic pan follow
- Initialization of all parameters that are not attached to a knob
- Improved behavior when changing sample rates
- Other improvements in the code (e.g. a fader object allowed the addition of new features with no CPU increase)
- Small UI enhancements, including fleas ... I mean lice ... no, I mean ticks (tick marks, that is, our buses do not have lice)

v. 1.0.1 Metro City Bus Release (September 23, 2019)

- Added Metro City Bus Mixer with polyphonic stereo spread
- Added Gig Bus Mixer for easy, standard mixing
- Minor CPU optimizations
- Small UI enhancements, including a better 70's cream color

Note: Some VCV patches saved with version 1.0.0 of the modular bus mixers will need to have levels reset after loading the patches in 1.0.1.
The developer thought parameters were being saved by their names.
He now knows parameters are saved by their order.

v. 1.0.0 Initial VCV Rack Library Release (September 6, 2019)

- Peak indicators on Bus Depot are now sample accurate
- Peak indicators stay on longer
- Text fixed on Bus Route

v. 1.0.0 Initial Release (September 3, 2019)

- Mini Bus Mixer, School Bus Mixer, and Bus Depot modules released
- Bus Route, Enter Bus, and Exit Bus utility modules for bus mixers released
