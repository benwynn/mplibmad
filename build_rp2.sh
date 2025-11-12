#!/bin/sh

# Settings for building for the Pico 2 W
ARCH=armv7emsp

if [ -z "$PICO_SDK_PATH" ]; then
  echo "error: no PICO_SDK_PATH set"
  exit 1
fi

make
