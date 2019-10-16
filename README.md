# Glue the Giant Rack Modules for VCV Rack

Mixing a VCV Rack patch to stereo output is a common task.
You can use monolithic rack modules that mimic real-world mixing consoles, but they don't take advantage of the power or flexibility inherent in a modular environment.
And other modular mixers tend to make it complicated to route multiple sounds to effect sends.

The modular bus mixers from Glue the Giant are designed for flexible effect sends, mix groups, and audio bus routing similar to what can be found in modern DAWs.
They also provide features like auto faders, polyphonic stereo spread, and manual latency compensation.

![alt text](https://github.com/gluethegiant/gtg-rack/blob/master/design/screenshot.png)

To use these mixers, simply connect them together with the BUS IN and BUS OUT ports.
The audio is routed along three stereo buses: red, orange, and blue.
The bus design makes it easy to patch together modules while creating simple or complex routing.

This README will give you an overview and list of features for each module.
Read the [Wiki](https://github.com/gluethegiant/gtg-rack/wiki) for examples on using the modules together in a patch.

## The Modular Bus Mixers

### 1. Mini Bus Mixer

This mixer strip is perfect for sounds that sit steady in the middle of your mix, like a kick or a bass.

* One mono or polyphonic input
* Level knobs to three stereo buses
* 2x or 4x preamp-style gain on input (selected from the context menu)
* On button with CV input and pop filter

### 2. Gig Bus Mixer

The Gig Bus Mixer treats the red bus as a master bus and has post fader sends to the blue and orange buses.

* Stereo, mono, or polyphonic input
* Constant power pan
* Level knobs to three stereo buses
* Blue and orange levels are post red level (red is the master bus) 
* 2x or 4x preamp-style gain on inputs (selected from the context menu)
* On button with CV input and pop filter

### 3. School Bus Mixer

Use this mixer strip for any sound in your mix.
With CV inputs on almost everything, you can connect LFOs, master control knobs, envelop generators, and more.

* Stereo, mono, or polyphonic input
* Constant power pan with CV input and attenuator
* Pan CV input expects bi-directional source from -5.0 to 5.0 
* Three level controls, with CV inputs, to three stereo buses
* Post fader optional on two level controls (red becomes the master bus)
* 2x or 4x preamp-style gain on inputs (selected from the context menu)
* On button with CV input and pop filter

Note that panning rolls back seamlessly when the pan CV input, attenuator, and pan knob overload the pan circuit.

### 4. Metro City Bus Mixer

This mixer takes a polyphonic input and spreads the channels across the stereo field.
The pan knob controls the pan placement of the first channel from the polyphonic input (or the last channel if the REVERSE CHANNELS button is on).
The spread knob spreads the other polyphonic channels to the left or to the right.
The LED indicators give you a visual indication of the stereo spread.

Automate the pan with an LFO (bidirectional) on the CV input for a polyphonic pan follow that can be adjusted with the attenuator and spread knobs.
In this case, the attenuator determines the stereo width of the auto pan.
The spread knob creates a follow delay for each polyphonic channel, with up to one second of delay between each channel.
The light indicators give you an indication of how the channels follow each other in the stereo field.

Pan spread and pan follow are smoothed to allow for dynamic polyphonic channels.

* One polyphonic input
* Polyphonic stereo spread and pan follow with LED visuals
* Constant power pan with CV input and attenuator
* Pan CV input expects bi-directional source from -5.0 to 5.0 
* Reverse channel order on polyphonic spread or pan follow
* Three level knobs, with CV inputs, to three stereo buses
* Post fader option on two level controls (red becomes master bus)
* 2x or 4x preamp-style gain on polyphonic input (selected the from context menu)
* On button with CV input and pop filter

### 5. Bus Route

Connects standard effect sends and returns to your buses.

Bus Route also contains an optional integrated sample delay on each send.
These delays can be used for latency compensation.
For example, most effect modules in VCV Rack will create a latency of only one sample.
If you route to a single effect on the orange bus, you could add a one sample delay to your red bus to keep your audio in perfect sync.
This latency compensation of one sample is hardly noticeable, but some effects, including external plugins, can require latency compensation to prevent unwanted phase.
When latency is not documented by a plugin, a scope can be used to detect and correct for latency.

The integrated sample delays can also be used to create effects based on millisecond delays, or subtle time shifts on a bus or a mix group.
These delays are limited to 999 samples so they can be light on resources and set with sample accuracy.

* Three stereo sends
* Three stereo returns
* Three sample delay lines, one on each stereo bus send, with up to 999 samples of delay

Note: manually connect outputs to inputs on all buses that need pass through.

### 6. Enter Bus

For advanced routing, including loop backs from send effects, this utility module merges audio onto any bus.

* Three stereo inputs to three stereo buses
* Three bus input level knobs

### 7. Exit Bus

For advanced routing or a simple extension panel to expose the outputs of any bus module or bus mix.

* Three stereo outputs from three stereo buses

### 8. Road

Road will get your buses from mix groups to master effects and final output.

* 6 bus inputs (18 stereo buses) to one bus output (3 stereo buses)

### 8. Bus Depot

This module is placed at the end of your bus mixer chains.
When it is your only Bus Depot, or the last one, the stereo outputs are typically connected to the left and right channels of your audio device.

In addition to providing a master stereo output, Bus Depot adds vu meters and faders to mix groups.
In these cases, the bus output from Bus Depot is connected to Road or to another mix chain (although the stereo output can also be connected to the aux input of another Bus Depot when bus routing is not needed).

Bus Depot's on button controls an auto fader.
By changing the fader speed knob, the fades can go from providing a 20 millisecond pop filter to a long fade up or down of up to 17 seconds.

* Master level knob (applies level to all outputs) with CV input
* Aux input that accepts stereo, mono, or polyphonic cables
* Aux level control
* Left and right stereo output from the sum of three stereo buses and aux input
* Bus output (with aux input added to red bus)
* Left and right peak meters with a brief hold on peaks over 0dB
* Each meter light represents -6dB
* Output on/off button with a linear auto fader
* Auto fader speed knob, with CV input, sets fades from 20 milliseconds to 17 seconds

## Copyrights, Trademarks, Licenses, Etc.

"Glue the Giant" is the name of a band and a software developer.
Do your best to not steal it (we know it's cool).
The designs help to give these modules a unique look.
Try to not copy them too closely.
All the code is released to use under the GPL 3 license and above.

## Acknowledgments 

Thanks to Andrew Belt for accepting a little fix I wrote for a library that VCV Rack uses.
That encouraged me to start writing C++.

Thanks to all the other Rack coders, especially those who share their code.
You are too numerous to thank individually, but I tried to star all of your repos.
Thanks for the early encouragement from users, like Aria_Salvatrice, dag2099, and chaircrusher, who displayed the use of the modular bus mixers in published songs and videos.
That early encouragement (you know who the rest of you are) made me want to make the modules more feature complete with better code under the hood.
Thanks to rsmus7 for beta testing.
Thanks to Omri Cohen for the videos that inspired these modules.
Finally, thanks always to Rebecca.

## Building from Source

To build these rack modules, see the official [VCV Rack documentation](https://vcvrack.com/manual/Building.html).

## Release Notes

v. 1.0.3 The Mix Group Release (October 18, 2019)

- Mix groups can more easily use the faders and vu meters of Bus Depot modules while still routing buses to final send effects (e.g. when you have one reverb to rule them all)
- Added bus output to Bus Depot to facilitate mix groups.
- Added the Road module to merge mix groups 
- Added three input level knobs to Enter Bus (allows, among other things, creative routing of a send effect return)

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
