import mplibmad # type: ignore
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
def input_callback(stream):
    print(f"input_callback called with stream: {stream}")
    global input_state
    input_state += 1
    if input_state > 15:
        print("input_callback returning EOF")
        return mplibmad.MAD_FLOW_STOP
    return mplibmad.MAD_FLOW_CONTINUE

def output_callback(frame):
    print(f"output_callback called with frame: {frame}")
    return None

def error_callback(errmsg):
    print(f"error_callback called with errmsg: {errmsg}")
    return None

@test_decorator
def decode_file(source, dest):
    print(f"decode_file called with source: {source}, dest: {dest}")
    decoder = mplibmad.Decoder(
        input=input_callback,
        output=output_callback,
        error=error_callback
    )
    print(f"Created decoder object with callbacks: {decoder}")
    assert decoder is not None, "Decoder() should return an object"
    time.sleep(1)
    result = decoder.run()
    return True

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
