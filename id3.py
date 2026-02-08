#!/usr/bin/env python
"""
little program to read how long a ID3 header is going to be,
so i know how many initial frames are going to be discarded
"""

def main():
  buf = bytearray(1024)
  with open('test.mp3', 'rb') as source:
    numbytes = source.readinto(buf)
    print(f"read {numbytes} bytes")
    if numbytes <= 0:
      print("EOF")
      return

    if numbytes >= 10:
      header = buf[:10]
      print(f"header: {header}")

      if header[0] == ord('I') and header[1] == ord('D') and header[2] == ord('3'):
        print("Found ID3 TAG")
      else:
        return

      vers_high = header[3]
      vers_low = header[4]
      print(f"Version: 2.{vers_high}.{vers_low}")

      flags = header[5]
      print(f"flags: {flags}")
      if (flags & 0x80):
          print("unsynch is used")
      if (flags & 0x40):
          print("extended header")
      if (flags & 0x20):
          print("experimental tag")

      # 28 bit tag length:
      tag_len = ((header[6] & 0x7f) << 21)
      tag_len += ((header[7] & 0x7f) << 14)
      tag_len += ((header[8] & 0x7f) << 7)
      tag_len += ((header[9] & 0x7f))
      print(f"Tag Length: (10 byte header) + {tag_len}")


if __name__ == "__main__":
  main()
