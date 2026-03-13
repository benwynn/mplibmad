# python built-in
import os

# micropython built-in
from machine import I2S
from machine import Pin

# local library
import wave


# Function     | I2S Label | FreeNove Label
# ------------------------------------------
# Master Clock | MCK       | SCK (System Clock)
# Serial Clock | SCK       | BCK (Bit Clock)
# Data In      | SD        | DIN (Data In)
# Word Select  | WS        | LCK (Left/Right Clock)

# I2S PIN CONFIG
SCK_PIN = 16 # bit clock (aka serial clock)
WS_PIN  = 17 # word select (aka left/right clock)
SD_PIN  = 18 # sound data


def main():
    with wave.open('/sd/output.wav', 'r') as src:
        nchannels = src.getnchannels()
        framerate = src.getframerate()
        sample_width = src.getsampwidth()
        #comptype = src.getcomptype()
        #compname = src.getcompname()
        params = src.getparams()
        print(f"params: {params}")

        # how many frames can we read into a buffer of 4000 bytes?
        read_frames = 4000 // (sample_width * nchannels) 
        # setting the buflen from the math above incase there were any rounding issues
        buflen = read_frames * sample_width * nchannels
        print(f"read_frames: {read_frames}, buflen: {buflen}")

        audio_out = I2S(0,
            sck    = Pin(SCK_PIN, Pin.OUT, value = 0),
            ws     = Pin(WS_PIN, Pin.OUT, value = 0),
            sd     = Pin(SD_PIN, Pin.OUT, value = 0),
            mode   = I2S.TX,
            bits   = sample_width * 8,
            format = I2S.MONO if nchannels == 1 else I2S.STEREO,
            rate   = framerate,
            ibuf   = buflen,
        )

        print("Starting...")
        try:
            while True:
                samples = src.readframes(read_frames)
                #print(".", end="")
                if not samples:
                    break
                samples = bytearray(samples)
                I2S.shift(buf=samples, bits=sample_width * 8, shift=-3) # volume control
                #print("+", end="")
                num_written = audio_out.write(samples)
                #print("=", end="")

        except (KeyboardInterrupt, Exception) as e:
            print("caught exception {} {}".format(type(e).__name__, e))

    # cleanup
    audio_out.deinit()
    print("Done")

if __name__ == "__main__":
    main()
