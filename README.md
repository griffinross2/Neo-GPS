# Neo-GPS
Neo-GPS is my second take on creating a custom GPS receiver, taking into account the lessons I learned previously and seeking to do everything as custom as possible.

## Goals
* Fully-functioning GPS L1 C/A receiver.
* Custom implementation of HDL and firmware modules wherever practical.
* Testing infrastructure with testbenches for all modules.
* Use of open-source software and tools (e.g. avoid Vivado until synthesis, instead using Verilator for simulation.

## Basic Implementation Concepts
To demodulate and track, we need fast real-time hardware, so an FPGA is the best choice. However, decoding ephemeris and solving the navigation equations requires memory and complex
calculations so this should be done on a microcontroller. This means we need the microcontroller to control the tracking channels in the FPGA to start tracking satellites and to get
timing and navigation data to do its decoding and solving. This will happen through SPI, with the FPGA presenting registers to be read/written.

This split architecture was used by [Andrew Holme](http://www.aholme.co.uk/GPS/Main.htm) whose custom GPS receiver project is a major source of inspiration and knowledge for this one.

The FPGA itself receives samples of an IF downconverted from the L1 band (1.57542 GHz). Because I am neither smart enough nor brave enough to attempt my own GPS RF frontend design,
I am using the [MAX2769](https://www.analog.com/en/products/max2769.html) GPS receiver IC. This chip contains an LNA, PLL, ADCs, and the other associated components that downconvert 
and then sample the signal. Up to 2-bit I and 2-bit Q samples are then presented to the FPGA along with the sampling clock.

I will also implement a micro-SD logger on the FPGA to log raw samples and/or other data for testing purposes.

## Progress
### Week of 5/4/26
Exams are finishing up and I can begin the project. I've started by implementing some automation to compile and run testbench simulations using Verilator in RTL/Simulated. I am also
starting to think about how I want the SD controller to work and start writing it.
