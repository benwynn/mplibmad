import mplibmad_x64 as mplibmad # type: ignore
import time, os
import struct
import wave

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
    mv = memoryview(buffer)

    while room > 0:
      bytes_read = source.readinto(mv[bytes_read:])
      room -= bytes_read
      print(f"bytes_read: {bytes_read}")
      if bytes_read == 0:
          # nothing new read? EOF
          return 0
      
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
        dest.setsampwidth(pcm['width'])
        print(f"width: len={len(pcm['left'])} length={pcm['length']} width={pcm['width']}")
        print(f"channels={pcm['channels']} samplerate={pcm['samplerate']} length={pcm['length']}")
    
    frame = bytearray(pcm['channels'] * pcm['width'] * pcm['length'])
    idx = 0
    while idx < pcm['length']:
        frame_idx = idx * pcm['channels']

        frame[frame_idx+0] = pcm['left'][idx+3]
        frame[frame_idx+1] = pcm['left'][idx+2]
        frame[frame_idx+2] = pcm['left'][idx+1]
        frame[frame_idx+3] = pcm['left'][idx+0]
        if pcm['channels'] > 1:
            frame[frame_idx+4] = 0 #pcm['right'][idx+0]
            frame[frame_idx+5] = 0 #pcm['right'][idx+0]
            frame[frame_idx+6] = 0 #pcm['right'][idx+0]
            frame[frame_idx+7] = 0 #pcm['right'][idx+0]

        if idx%1000==0:
            sample_left = struct.unpack("<f", pcm['left'][idx:idx+4])[0]
            sample_frame = struct.unpack("<f", frame[frame_idx:frame_idx+4])
            print(f"sample{idx}: {sample_left:02.4f}")
            print(f" left: {sample_left}={idx:04}={hex(pcm['left'][idx]):04}|{hex(pcm['left'][idx+1]):04}|{hex(pcm['left'][idx+2]):04}|{hex(pcm['left'][idx+3]):04}")
            print(f"frame: {sample_frame}={frame_idx:04}={hex(frame[frame_idx]):04}|{hex(frame[frame_idx+1]):04}|{hex(frame[frame_idx+2]):04}|{hex(frame[frame_idx+3]):04}")

        idx += pcm['width'] * pcm['channels']
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
    return result

def main():
    print("Start decode...")
    print(dir(mplibmad))
    print(os.listdir())
    input_name = "test.mp3"
    output_name = "output.wav"
    with open(input_name, "rb") as input_file:
        with wave.open(output_name, "w") as output_file:
            print(f"Decoding from {input_name} to {output_name}")
            result = decode_file(input_file, output_file)
            print(f"decode_file result: {result}")
    print("Done.")
    
if __name__ == "__main__":
    main()
