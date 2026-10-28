# Neo-GPS
Neo-GPS is my second take on creating a custom GPS receiver, taking into account the lessons I learned previously and seeking to do everything as custom as possible.

## Disclaimer
This project was made for my own learning and interest. It is NOT a professional implementation of a GPS receiver nor is it ever likely to be in a complete or reliable state.
It should be used for your learning and interest too, not for any real-world use.

## Goals
* Fully-functioning GPS L1 C/A receiver.
* Custom implementation of HDL and firmware modules wherever practical.
* Testing infrastructure with testbenches for all modules.
* Use of open-source software and tools (e.g. avoid Vivado until synthesis, instead using Verilator for simulation).

## Basic Implementation Concepts
To demodulate and track, we need fast real-time hardware, so an FPGA is the best choice. However, decoding ephemeris and solving the navigation equations requires memory and complex
calculations so this should be done on a microcontroller. This means we need the microcontroller to control the tracking channels in the FPGA to start tracking satellites and to get
timing and navigation data to do its decoding and solving. This will happen through SPI, with the FPGA presenting registers to be read/written.

This split architecture was used by [Andrew Holme](http://www.aholme.co.uk/GPS/Main.htm) whose custom GPS receiver project is a major source of inspiration and knowledge for this one.

The FPGA itself receives samples of an IF downconverted from the L1 band (1.57542 GHz). Because I am neither smart enough nor brave enough to attempt my own GPS RF frontend design,
I am using the [MAX2769](https://www.analog.com/en/products/max2769.html) GPS receiver IC. This chip contains an LNA, PLL, ADCs, and the other associated components that downconvert 
and then sample the signal. Up to 2-bit I and 2-bit Q samples are then presented to the FPGA along with the sampling clock.

I will also implement a micro-SD logger on the FPGA to log raw samples and/or other data for testing purposes.

## Usage
This project is split into several directories:

### Hardware
This folder contains KiCAD projects and libraries. There is a frontend board used with the FPGA dev board, an SD card board also used with the FPGA, and a project for the full custom board.
As of writing, only the frontend is in use. The SD card board was not ordered, and the full custom board is in development still. Use KiCAD 10.

### Simulation
This is a C++ simulation of a GPS receiver supporting GPS L1 C/A and Galileo E1 currently. It takes in a binary signal recording file which is just 1-bit I samples packed little-endian.
Samples are not included, but I may add a link to some at some point. The core of the simulator in `src/` and `inc/` is compiled as a library. The examples in `tests/` use it, including
`solve` which is a full simulation of acquisition, tracking, and solving for position using GPS and Galileo. The simulation depends on FFTW (included) and Eigen (installed by CMake).

To build and run, make sure you have a modern C++ compiler and CMake installed. Use CMake to build and run on of the test targets. It should install the needed dependencies automatically.
This software has only been tested on Windows but should work elsewhere.

### RTL
The RTL directory contains the RTL for the FPGA which, for testing, was a Spartan-7 on an Arty S7-50 board. `src/` contains modules. `testbench/` contains top-level testbenches for simulation. 
`top/` contains top-level files used with Vivado for on-board testing. `constraints/` contains .xdc constraints files for Vivado. `verilator/` contains CMake files and C++ testbenches for 
simulating with Verilator. `scripts/` contains TCL scripts for running Vivado.

To simulate with Verilator, first make sure you have a working installation of Verilator and CMake. Make sure the VERILATOR_ROOT environment variable is set. Then use the `verilator.py` script. 
This script helps to build and run the testbenches. Setting up a new testbench requires making one in `testbenches/` and a corresponding `CMakeLists.txt` and `sim_main.cpp` in the `verilator/`
folder. Use the examples given as a template.

To synthesize using Vivado, first make sure yuou have Vivado installed. I then added it to my PATH, but you could use the path to it directly in `vivado.py`. If you are making a new top-level
file, add it to `top/` and make a corresponding TCL script in `scripts/`. Then you can use the python script to initialize Vivado with the project. You will probably want to use the `-g` option
to actually open the Vivado GUI, unless you set it up to do everything automatically using the TCL script.

### RP2350
This folder contains the code used with Raspberry Pi Pico 2 board for testing. The Pi Pico SDK is pulled in as a Git submodule. Make sure you pull that and also install the Arm GNU Toolchain.
Then, you can modify `CMakeUserPresets.json` with your path to the toolchain or setup your own CMake configuration. Then just use CMake to build.