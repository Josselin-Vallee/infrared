#!/usr/bin/python3

import binascii
import socket
import struct
import sys
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = '192.168.1.13'
port = 1313
sock.connect((host, port))

packer = struct.Struct('f')

try:
    print('Sending time to server...')
    
    value = (time.clock())
    packed_data = packer.pack(*value)
    sock.sendall(packed_data)

    data = sock.recv(256)
    unpacked_data = packer.unpack(data)

    print('Response from server: ', unpacked_data)

finally:        
    sock.close()
