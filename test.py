import mplibmad_x64 as mplibmad # type: ignore
import time

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

def python_return():
   print("hi")

@test_decorator
def test_function_call():
    result = mplibmad.hello("abc")
    expected = python_return()
    return (result == expected)

@test_decorator
def test_type_error():
    """testing with integer instead of string"""
    try:
      r = mplibmad.hello(42)
      print(f"ERROR: expected TypeError but got result: {r}")
    except TypeError:
        return True
    except Exception as e:
      print(f"got unexpected exception: {type(e).__name__}: {e.value}")
    return False

def my_callback(tmp):
    print(f"callback got: {tmp}")
    return 123

@test_decorator
def test_callback():
    r = mplibmad.set_cb(my_callback)
    assert r is None, "set_cb should return None"
    r = mplibmad.call_cb("hi there")
    assert r is 123, "call_cb should return 123"
    return True

@test_decorator
def test_module_constants():
    assert mplibmad.MAD_FLOW_CONTINUE == 0, "MAD_FLOW_CONTINUE should be 0"
    assert mplibmad.MAD_FLOW_STOP == 16, "MAD_FLOW_STOP should be 16"
    assert mplibmad.MAD_FLOW_BREAK == 17, "MAD_FLOW_BREAK should be 17"
    assert mplibmad.MAD_FLOW_IGNORE == 32, "MAD_FLOW_IGNORE should be 32"
    return True

def input_callback():
    print("input_callback called")
    return None

def output_callback(frame):
    print(f"output_callback called with frame: {frame}")
    return None

def error_callback(errmsg):
    print(f"error_callback called with errmsg: {errmsg}")
    return None

@test_decorator
def test_new_object_should_fail():
    try:
      decoder = mplibmad.Decoder()
      print(f"ERROR: expected TypeError but got result: {decoder}")
    except TypeError:
        # expected result, Decoder() requires arguments
        return True
    except Exception as e:
      print(f"got unexpected exception: {type(e).__name__}: {e.value}")
    return False

@test_decorator
def test_new_object_with_callbacks():
    decoder = mplibmad.Decoder(
        cb_data=None,
        input=input_callback,
        output=output_callback,
        error=error_callback
    )
    print(f"Created decoder object with callbacks: {decoder}")
    assert decoder is not None, "Decoder() should return an object"
    print(dir(decoder))
    return True

def run_tests():
    print("Start Test:")
    print(dir(mplibmad))
    #test_function_call()
    #test_type_error()
    #test_callback()
    test_module_constants()
    test_new_object_should_fail()
    test_new_object_with_callbacks()
    print("Done.")
    
if __name__ == "__main__":
    run_tests()
