import mplibmad_x64 as mplibmad # type: ignore
import time, os

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
def input_callback(decoder, data):
    global input_state
    #print(f"input_callback({input_state}) called with decoder({decoder}, {data})")
    source = data['source']
    bytes_read = source.readinto(data['buffer'])
    #print(f"input_callback read {bytes_read} bytes")
    if input_state > 50:
        print("input_callback reached max calls")
        return mplibmad.MAD_FLOW_STOP
    input_state += 1
    if not bytes_read:
        print("input_callback reached EOF")
        return mplibmad.MAD_FLOW_STOP

    decoder.stream_buffer(data['buffer'], bytes_read)
    return mplibmad.MAD_FLOW_CONTINUE

def output_callback(decoder, data):
    print(f"output_callback called with {decoder} and {data}")

    return mplibmad.MAD_FLOW_CONTINUE

@test_decorator
def decode_file(source, dest):
    print(f"decode_file called with source: {source}, dest: {dest}")
    cb_data = {
        'source': source,
        'dest': dest,
        'buffer': bytearray(1024),
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
    output_name = "output.pcm"
    with open(input_name, "rb") as input_file:
        with open(output_name, "wb") as output_file:
            print(f"Decoding from {input_name} to {output_name}")
            result = decode_file(input_file, output_file)
            print(f"decode_file result: {result}")
    print("Done.")
    
if __name__ == "__main__":
    main()
