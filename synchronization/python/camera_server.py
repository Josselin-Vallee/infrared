#!/usr/bin/python2

import picamera
import signal
import socket
import sys
import time

# constants
master = '192.168.1.13'
slave = '192.168.1.14'
host = slave
port = 1313
camera_resolution_horizontal = 2592
camera_resolution_vertical = 1944
rgb_image_file = 'rgb.jpg'

def sigint_handler(signal, frame):
    print('You pressed Ctrl+C!')
    sys.exit(0)

# image capture stub
def capture(capture_time):
    while time.time() < capture_time:
        pass

    print 'Capturing image at time', time.time()

    with picamera.PiCamera() as camera:
        camera.resolution = (camera_resolution_horizontal, camera_resolution_horizontal)
        camera.start_preview()
        time.sleep(2)
        camera.capture(rgb_image_file, 'jpeg')
        camera.stop_preview()

    return

# register signal handler
signal.signal(signal.SIGINT, sigint_handler)

# create server socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((host, port))
sock.listen(10)

while True:
    print 'Listening for client...'

    conn, addr = sock.accept()

    try:
        print 'Accepted connection from', addr

        while True:
            data = conn.recv(256)
            print 'Capture time received from client:', data

            capture(float(data))

            print 'Sending image data to client...'
            f = open(rgb_image_file, 'rb')
            data = f.read(4096)
            while data:
                conn.send(data)
                data = f.read(4096)
            print 'Done sending image.'
            break

    finally:
        conn.close()
