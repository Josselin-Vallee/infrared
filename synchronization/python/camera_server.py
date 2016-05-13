#!/usr/bin/python2

import socket
import sys
import time

# image capture stub
def capture(capture_time):
    while time.time() < capture_time:
        pass
    print 'Capturing image at time', time.time()
    return

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
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
                print 'Capture time received from client:', data
                
                capture(float(data))
                
                print 'Sending image data to client...'
                f = open('sent.jpg', 'rb')
                data = f.read(4096)
                while data:
                    conn.send(data)
                    data = f.read(4096)
            else:
                print 'Done sending image.'
                break

    finally:
        conn.close()
