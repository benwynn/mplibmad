hardware = False
try:
    import mplibmad_x64 as mplibmad # type: ignore
except ImportError:
    import mplibmad
    hardware = True

import time, os
import struct
import wave

if hardware:
    from machine import SPI, Pin
    import sdcard
    SPI_BAUD = 25000000 # 25Mhz

    # these just happen to be the pins i have wired up
    def sdsetup():
        spi = SPI(1, baudrate=SPI_BAUD, sck = Pin(10), mosi=Pin(11, Pin.OUT, value=1), miso=Pin(8))
        cs = Pin(9, mode=Pin.OUT, value=1)
        sd = sdcard.SDCard(spi, cs, baudrate = SPI_BAUD)

        if False:
            sectors = sd.ioctl(4, None)
            blocksize = sd.ioctl(5, None)
            print(f"capacity = {sectors*blocksize}, sectors = {sectors}, blocksize = {blocksize}")

        files = os.listdir()
        if "test" not in files:
            os.mkdir("/test")

        test_files = os.listdir('/test')
        print(f"before: {test_files}")
        os.mount(sd, "/test")
        test_files = os.listdir('/test')
        print(f"after: {test_files}")
else:
    def sdsetup():
        print("Not running on hardware, skipping SD card setup")
    

class EnterExitLog():
    def __init__(self, name):
        self.name = name
        self.result = None

    def __enter__(self):
        #print(f"Entering {self.name}")
        self.init_time = time.time()
        return self

    def set_result(self, result):
        self.result = result

    def __exit__(self, exc_type, exc_value, traceback):
        if (self.result and isinstance(self.result, Exception)):
            print(f"Test: {self.name} got unexpected exception: {type(self.result).__name__}: {self.result.value}")
        print(f"Test: {self.name} is {'OK' if (self.result==True) else'FAILED'} in {time.time() - self.init_time} seconds")

def test_decorator(func):
    def func_wrapper(*args, **kwargs):
        with EnterExitLog(func.__name__) as eel:
            try:
                result = func(*args, **kwargs)
                eel.set_result(result)
                return result
            except Exception as e:
                eel.set_result(e)
                return False
    return func_wrapper

input_state = 0
def input_callback(decoder, data, buffer):
    global input_state

    print(f"buffer: len = {len(buffer)}")
    source = data['source']

    bytes_read = 0
    room = len(buffer)
    print(f"input_callback buffer size: {room}")
    mv = memoryview(buffer)

    while room > 0:
      retval = source.readinto(mv[bytes_read:])
      bytes_read += retval
      room -= retval
      print(f"bytes_read: {bytes_read}")
      if retval == 0:
          # we've reached EOF, return partial buffer
          return bytes_read
      
    input_state += bytes_read
    if (input_state % 4096) == 0:
        print(f"input_callback total bytes read: {input_state}")

    if not bytes_read:
        print("input_callback reached EOF")
        return 0

    #decoder.stream_buffer(data['filebuf'], bytes_read)
    return bytes_read

def scale_sample(sample):
    # round 
    MAD_F_ONE = 0x10000000
    MAD_F_FRACBITS = 28
    sample += (1 << (MAD_F_FRACBITS - 16))

    # clip
    if sample >= MAD_F_ONE:
        sample = MAD_F_ONE - 1
    elif sample < -MAD_F_ONE:
        sample = -MAD_F_ONE

    # quantize
    return (sample >> (MAD_F_FRACBITS + 1 - 16))


first_write = True
def output_callback(decoder, data):
    global first_write
    dest = data['dest']
    pcm = decoder.get_pcm()

    if first_write:
        dest.setnchannels(pcm['channels'])
        dest.setframerate(pcm['samplerate'])
        dest.setsampwidth(2) # 16-bit samples
        print(f"width: array len={len(pcm['left'])} sample count={pcm['length']} width={pcm['width']}")
        print(f"channels={pcm['channels']} samplerate={pcm['samplerate']} length={pcm['length']}")
    
    frame = bytearray(pcm['channels'] * 2 * pcm['length'])
    sample_num = 0 # increments per sample, (source is 4 bytes, dest is 2 bytes)
    qfac = 2**(28-24)
    while sample_num < pcm['length']:
        pcm_idx = sample_num * 4
        frame_idx = sample_num * 2 * pcm['channels']

        sample_raw = struct.unpack("<i", pcm['left'][pcm_idx:pcm_idx+4])[0]
        sample_scaled = mplibmad.scale(sample_raw)
        sample = struct.pack(">h", sample_scaled)

        if sample_num%500==0:
            print(f"orig={pcm['left'][pcm_idx:pcm_idx+4].hex()} sample_raw={sample_raw} sample_scaled={sample_scaled} -> sample: {sample.hex()} or {(sample_scaled>>8) & 0xFF}:{(sample_scaled>>0) & 0xFF}")

        sample_num += 1

        # write out 16-bit little endian pcm samples
        frame[frame_idx+0] = sample[1]
        frame[frame_idx+1] = sample[0]

        # if stereo, write right channel sample
        if pcm['channels'] > 1:
            sample_raw = struct.unpack("<i", pcm['right'][pcm_idx:pcm_idx+4])[0]
            sample_scaled = mplibmad.scale(sample_raw)
            sample = struct.pack(">h", sample_scaled)
            frame[frame_idx+2] = sample[1]
            frame[frame_idx+3] = sample[0]

    dest.writeframes(frame)
    first_write = False

    return mplibmad.MAD_FLOW_CONTINUE

@test_decorator
def decode_file(source, dest):
    print(f"decode_file called with source: {source}, dest: {dest}")
    cb_data = {
        'source': source,
        'dest': dest,
        'mp3buf': bytearray(4096),
        'filebuf': bytearray(1024)
        }
    decoder = mplibmad.Decoder(
        cb_data=cb_data,
        input=input_callback,
        output=output_callback,
        #error=error_callback
    )
    print(f"Created decoder object with callbacks: {decoder}")
    assert decoder is not None, "Decoder() should return an object"
    time.sleep(1)
    result = decoder.run()
    return not result

def main():
    print("Setting up SD card...")
    sdsetup()

    print("Start decode...")
    print(dir(mplibmad))
    print(os.listdir())
    input_name = "test/test.mp3"
    output_name = "test/output.wav"
    with open(input_name, "rb") as input_file:
        #with open(output_name, "wb") as output_file:
        with wave.open(output_name, "w") as output_file:
            print(f"Decoding from {input_name} to {output_name}")
            result = decode_file(input_file, output_file)
            print(f"decode_file result: {result}")
    print("Done.")
    
if __name__ == "__main__":
    main()
