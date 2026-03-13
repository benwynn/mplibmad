

# these just happen to be the pins i have wired up on my Pico2w
def sdsetup(mount_point):
    if not mount_point:
        return None
    if mount_point[0] != '/':
        mount_point = '/' + mount_point
    
    import machine
    import os
    import sdcard
    
    SPI_BAUD = 25000000 # 25Mhz
    PIN_SCK = machine.Pin(10)
    PIN_MOSI = machine.Pin(11, machine.Pin.OUT, value=1)
    PIN_MISO = machine.Pin(8)
    PIN_CS = machine.Pin(9, mode=machine.Pin.OUT, value=1)
    
    spi = machine.SPI(1, baudrate=SPI_BAUD, sck = PIN_SCK, mosi=PIN_MOSI, miso=PIN_MISO)
    print(f"boot spi: {spi}")
    sd = sdcard.SDCard(spi, PIN_CS, baudrate = SPI_BAUD)
    print(f"boot sd: {sd}")

    files = os.listdir()
    if mount_point[1:len(mount_point)] not in files:
        print(f"mkdir({mount_point})")
        os.mkdir(mount_point)

    if sd:
        sectors = sd.ioctl(4, None)
        blocksize = sd.ioctl(5, None)
        print(f"found card: capacity = {sectors*blocksize}, sectors = {sectors}, blocksize = {blocksize}")
        os.mount(sd, mount_point)
    return sd

print("boot.py")
sd = "nope"
try:
    sd = sdsetup("sd")
except Exception as e:
    print(f"SDSetup caught {type(e).__name__}: {e}")

print(f"sd = {sd}")
