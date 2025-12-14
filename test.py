import mplibmad

print("start:")

mplibmad.hello("abc")

print("testing with integer instead of string:")
try:
  mplibmad.hello(42)
except TypeError:
  print(f"got expected TypeError exception, working as designed")
except Exception as e:
  print(f"got unexpected exception: {type(e).__name__}: {e.value}")

def my_callback(tmp):
    print("in my_callback")
    print(f"tmp was: {tmp}")
    return 123

mplibmad.set_cb(my_callback)
mplibmad.call_cb(42)

print("Done.")
