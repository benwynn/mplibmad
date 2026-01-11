import mplibmad # type: ignore
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

def run_tests():
    print("Start Test:")
    test_function_call()
    test_type_error()
    test_callback()
    print("Done.")
    
if __name__ == "__main__":
    run_tests()
