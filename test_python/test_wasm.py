import time
import cflib.crtp
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.syncCrazyflie import SyncCrazyflie
from cflib.utils import uri_helper

from cflib.crtp.crtpstack import CRTPPacket
import sys

uri = uri_helper.uri_from_env(default='radio://0/80/2M/E7E7E7E7E7')

WASM_WRITE_CH = 0x0
WASM_CTRL_CH = 0x1
WASM_CTRL_RESET_FILE = 0x0
WASM_CTRL_RUN = 0x1

def wasm_send_data(cf, data):
    chunk_size = 28
    while data: 
        chunk, data = data[:chunk_size], data[chunk_size:]
        wasm_send_packet(cf, chunk)

def wasm_send_packet(cf, chunk):
    pk = CRTPPacket()
    pk.channel = WASM_WRITE_CH
    pk.port = 0x09
    pk.data = chunk
    cf.send_packet(pk)

def wasm_reset_file(cf):
    pk = CRTPPacket()
    pk.channel = WASM_CTRL_CH
    pk.port = 0x09
    pk.data = [0x0]
    cf.send_packet(pk)

def wasm_run_file(cf):
    pk = CRTPPacket()
    pk.channel = WASM_CTRL_CH
    pk.port = 0x09
    pk.data = [0x1]
    cf.send_packet(pk)

def wasm_send_file(filename):
    # Initialize the low-level drivers
    cflib.crtp.init_drivers()
    cf=Crazyflie(rw_cache='./cache')
    with SyncCrazyflie(uri, cf=cf) as scf:
        print("Connection successful")
        print("Start sending data...")
        data = []
        with open(filename, "rb") as file:
            data = file.read()

        wasm_reset_file(cf)
        wasm_send_data(cf, data)
        wasm_run_file(cf)

        print("Data has been sent!")

if __name__ == '__main__':
    filename = "test_python/lorem_ipsum.bin"

    if len(sys.argv) == 2:
        filename = sys.argv[1]

    wasm_send_file(filename)