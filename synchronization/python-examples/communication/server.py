#!/usr/bin/python2

import binascii
import socket
import struct
import sys
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = '192.168.1.13'
port = 1313
sock.bind((host, port))
sock.listen(10)

packer = struct.Struct('f')

while True:
    print('Listening for client...')

    conn, addr = sock.accept()

    try:
        print('Accepted connection from ', addr)

        while True:
            data = conn.recv(256)
            if data:
                unpacked_data = packer.unpack(data)
                print('Time received from client: ', unpacked_data)

                value = (time.clock())
                packed_data = packer.pack(*value)
                conn.sendall(packed_data)
            else:
                print('No more data received from client')
                break

    finally:
        conn.close()
