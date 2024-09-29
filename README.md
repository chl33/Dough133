# Dough133

Dough133 is a temperature-controlled container for proofing and fermenting sourdough. It uses a small cooler and a PTC heater with a fan to heat the inside of the cooler. A PCBA with an ESP32 regulates the power supplied to the heater to regulate the temperature of the enclosure.

![Doughh133](images/cooler-ebox-scaled-1400x1187.webp)

This repository is a work in progress.  It contains:
- KiCAD design for the PCB.
- Source code for the device firmware.
- OpenSCAD design for 3D printed components.

[Instructions for the assembly](https://selectiveappeal.org/posts/dough133-assembly/) of the device are in this are at my blog.

Please also see the other entries describing this project:
- [Dough133](https://selectiveappeal.org/posts/dough133/)
- [Dough133 Electronics Design](https://selectiveappeal.org/posts/dough133-electronics/)

## KiCAD

The circuit board design is in [KiCAD/](KiCAD/).

![Circuit board](images/Dough133-PCB-components-1400x1050.webp)


## OpenSCAD

Designs for 3D-printable components are in [scad/](scad/).

The EBox is based on the [ProjectBox](https://github.com/chl33/ProjectBox) OpenSCAD system.

The heater mount secures the heater and temperature/humidity sensor inside the cooler.

![Heater mount](images/heater-holder2-1400x1203.webp)

The Cooler plug helps avoid heat leaking through the hole through which wires run from the heater mount through the wall of the cooler to the EBox.

![Cooler plug](images/cooler-plug-1400x858.webp)


## Software

The software is based on the [og3](https://github.com/chl33/og3) library.
