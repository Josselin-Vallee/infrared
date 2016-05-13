#!/usr/bin/python2

import socket
import sys
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#host = '192.168.1.13'
host = '127.0.0.1'
port = 1313
sock.bind((host, port))
sock.listen(10)

while True:
    print 'Listening for client...'

    conn, addr = sock.accept()

    try:
        print 'Accepted connection from', addr

        while True:
            data = conn.recv(256)
            if data:
                print 'Time received from client:', data

                value = time.time()
                data = repr(value)
                
                print 'Sending time to client:', data
                conn.sendall(repr(value))                
            else:
                print 'No more data received from client'
                break

    finally:
        conn.close()
