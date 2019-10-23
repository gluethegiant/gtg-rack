# Glue the Giant's Modular Bus Mixers for VCV Rack

To use these mixers, simply connect them together with the BUS IN and BUS OUT ports.
The audio is routed along three stereo buses: red, orange, and blue.
The Bus Depot module combines the buses to create a stereo mix out.

The bus design makes it easy to patch together modules while creating simple or complex routing.
With features like auto faders, polyphonic stereo spread, mix groups, and integrated latency compensation, they're also fun to use.

![Modular bus mixers from Glue the Giant](https://github.com/gluethegiant/gtg-rack/blob/master/design/screenshot.png)

## User's Guide

A [User's Guide](https://github.com/gluethegiant/gtg-rack/wiki) for these modules, including a VCV Rack tutorial, is available in the [Wiki](https://github.com/gluethegiant/gtg-rack/wiki).

## Licenses, Copyrights, Etc.

"Glue the Giant" is the name of a band and a software developer.
Do your best to not steal it (we know it's cool).
The designs under the res/ and designs/ directories help to give these modules a unique look and are automatically copyrighted (c) 2019 in some jurisdictions.
Try to not copy them too closely.
All the code is released to use under the GPL 3 license and above.

## Acknowledgments 

Thanks to Andrew Belt for accepting a tiny page of code for a library that VCV Rack uses.
That encouraged the coding of these modules.

Thanks to all the other Rack coders, especially those who share their code.
You are too numerous to thank individually, but I tried to star all of your repos.
Thanks for the early encouragement from users, like Aria_Salvatrice, dag2099, and chaircrusher, who displayed the use of the modular bus mixers in published songs and videos.
That early encouragement made me want to make the modules more feature complete with better code under the hood.
Thanks to rsmus7 for beta testing.
Thanks to Omri Cohen for the videos that helped inspire these modules.
Finally, thanks always to Rebecca.

## Building from Source

To build these rack modules, see the official [VCV Rack documentation](https://vcvrack.com/manual/Building.html).

## Release Notes

v. 1.0.3 The Mix Group Release (October 18, 2019)

- Mix groups can more easily use the faders and vu meters of Bus Depot modules while still routing buses to final send effects (e.g. when you have one reverb to rule them all)
- Added bus output to Bus Depot to facilitate mix groups
- Added the Road module to merge parallel mix groups 
- Added three input level knobs to Enter Bus (allows, among other things, creative routing of a send effect return)
- Wrote an initial User's Guide, including a tutorial

v. 1.0.2 The Tick Release (October 12, 2019)

- Added 2x or 4x preamp-style gain on all four mixer strips (selected from the context menu)
- New sample delay lines provide latency compensation on Bus Route
- New fade automation with speed control on Bus Depot
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
