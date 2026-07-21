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

### Week of 7/20/26
The SD controller is done. It buffers sample data and then burst writes to the SD card. It can occasionally lag due to internal processes in the SD card, causing the buffer to overrun.
However, with a few attempts I can get recordings several minutes long. After the controller was finished I started to work on the L1 C/A acquisition. I started by bringing in the
module from my old repository. Because I don't want to rely on Vivado IP this time, I had to spend some time writing my own FFT. The design is simple, set at length 4096, and
can be configured for forward or inverse at runtime, as well as no scaling or 1-bit per stage scaling. It uses a pretty small amount of BRAM and DSP resources on my FPGA, and should be
fast enough for what I need it for. After some trouble, I got it working and adapted the acquisition module to use it instead of the Xilinx IP. The simulation for the whole module was
slow, but I was able to greatly speed it up by compiling the verilated RTL in release mode. However, this revealed another problem which is that the full wavefile can't be opened by
GTKWave, presumably because the file is too big. I may try breaking the wavefile at certain points to get multiple smaller ones instead.
