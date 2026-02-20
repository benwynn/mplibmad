### mplibmad

A Project to enable the use of libmad in MicroPython.

### Building

Dependencies:
- 'MPY_DIR' needs to be set to the location of micropython, or will assume it's relative location
- 'PICO_SDK_PATH' needs to be set, if you are building for rp2

Commands:
- make 

#### Architecture
you will need to set an appropriately defined 'ARCH' variable,

I have tested building using ARCH=...
- `x64` (unix port on ubuntu WSL)
- `armv7emsp` (Pi Pico 2 W)
- ...

Once the dependencies are in place, you could use `make`, or a build script for your architecture:
- build_rp2.sh


### libmad
libmad is a mp3 decoder, that ceased development in 2004,
archived at https://www.underbit.com/products/mad/

Many people have ported it to different environments over the years.
This is my attempt to make it suitable for MicroPython, to run on microcontrollers.

### Test Results

#### Stats

currently, on a pico2 aka armv7emsp:
- code size: 66438 up 100 from 66358
- decode time: 182 seconds, down from 480 seconds

#### Results

as of this commit, the code:
- compiles clean
- successfully runs test.py on unix and pico2
- successfully runs decode.py on unix and pico2
- the output.wav file generated is byte-for-byte the same on the unix port and pico2

