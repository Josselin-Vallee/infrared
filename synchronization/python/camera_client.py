#!/usr/bin/python2

import socket
import sys
import time
import picamera

# image capture stub
def capture(capture_time):
    while time.time() < capture_time:
        pass
    print 'Capturing image at time', time.time()

    with picamera.PiCamera() as camera:
        camera.resolution = (2592, 1944)
        camera.start_preview()
        time.sleep(2)
        camera.capture('rgb.jpg', 'jpeg')
        camera.stop_preview()

    return

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = '192.168.1.13'
# host = '127.0.0.1'
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
    f = open('nir.jpg', 'wb')
    data = sock.recv(4096)
    while len(data) > 0:
        f.write(data)
        data = sock.recv(4096)

    print 'Done receiving image.'
finally:
    sock.close()
