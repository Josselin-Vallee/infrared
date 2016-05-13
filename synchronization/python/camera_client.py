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
#host = '192.168.1.13'
host = '127.0.0.1'
port = 1313
sock.connect((host, port))

try:
    # image capture time
    value = time.time() + 1.0
    data = repr(value)
    
    print 'Sending capture time to server:', value
    sock.sendall(data)
    
    capture(value)

    print 'Receiving image data from server...'
    f = open('received.jpg', 'wb')
    data = sock.recv(4096)
    while data:
        f.write(data)
        data = sock.recv(4096)

    print 'Done receiving image.'
finally:        
    sock.close()
