### mplibmad

A Project to enable the use of libmad in MicroPython.

### Building

Dependencies:
- 'MPY_DIR' needs to be set to the location of micropython, or will assume it's relative location
- 'PICO_SDK_PATH' needs to be set, if you are building for rp2
- 'ARCH' you will need to set an appropriately defined

Once the dependencies are in place, you could use `make`, or a build script for your architecture:
- build_rp2.sh


### libmad
libmad is a mp3 decoder, that ceased development in 2004,
archived at https://www.underbit.com/products/mad/

Many people have ported it to different environments over the years.
This is my attempt to make it suitable for MicroPython, to run on microcontrollers.

