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

### 8/18/26
After a lot of work, I was able to complete the l1ca_ac_pca_search and l1ca_channel modules which handle acquiring and tracking satellites, respectively, and the accompanying registers. 
The search module mostly uses a 50MHz clock (core_clk) to speed up the FFT, and I decided to just keep all of the registers in the core_clk domain, so there is a lot of CDC to handle that.
All of the registers and CDC logic are in l1ca_search_unit and l1ca_channel_unit modules that can be duplicated easily to add more search and channel units. The gps_tb Verilator testbench 
provides an emulated SPI host. Claude figured out how to make a thread in the C++ code where we can just perform blocking SPI write and read calls to really easily access the registers. 
After this, I was able to port over the l1ca_track.cpp and some other code from the C++ simulation in my old repo and run a full simulation of acquiring, then locking a channel. HUGE.

After that, I started writing a skeleton for and ESP32 to be able to access the registers. I got the RTL to synthesize and go on to the FPGA tonight and ran a test with the ESP32 searching
every GPS satellite. It worked and I got strong signals from 4 satellites and a weaker one from a 5th. Also HUGE. Next, I can try tracking a channel using the ESP32. To keep going though,
I don't want to use an SPI connection. We will be pushing it with latency and throughput when adding more channels, especially Galileo. There aren't many choices for parellel interfaces with
common microcontrollers. Since I will be using an STM32 for a custom board version of this, I want to try to make an interface compatible with the FMC (flexible memory controller)
available on the STM. We will have at least 16 data lines in parallel, and I will probably change all of the registers to be 32-bit. I might also just choose to use an Octo-SPI instead
depending on how things look with the FMC.

I will have to pause for a bit as I move back to school. The next steps after that are the ESP32 tracking test and working on designing a custom board. I will also start to implement Galileo
search and tracking. This should go much smoother as it is pretty similar on the hardware side to GPS.

### 10/23/26
I tested using an ESP32, but it was slow and I decided to move on to a Pi Pico 2 instead. I wrote a PIO implementation of a simple parallel synchronous interface similar to QSPI. It is 16 bits
wide and 4 clock cycles: 1 for address and R/W bit, 1 for bus turnaround, and 2 for data. All of the FPGA side registers are 32-bits with a 15-bit address. After a lot of rewriting the GPS
simulator code to implement the tracking functions in an interrupt on the Pico, I was able to successfully track 6 channels and once and get the first real-time hardware position solution!
There remain many bugs in the Pico code, but the main goal of the project is finally achieved. Now, I will likely step back from the project for a while. When I come back, some next steps will
be fixing those bugs as well as a custom PCB implementation and Galileo support.