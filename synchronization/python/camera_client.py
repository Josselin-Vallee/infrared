#!/usr/bin/python2

import cv2
import picamera
import signal
import socket
import subprocess
import sys
import time

import registration
import merging
import shadow_detection

# constants
master = '192.168.1.13'
slave = '192.168.1.14'
host = slave
port = 1313
op_skin_smoothing = 'OP_SKIN_SMOOTHING'
op_shadow_detection = 'OP_SHADOW_DETECTION'
camera_resolution_horizontal = 2592
camera_resolution_vertical = 1944
nir_image_file = 'nir.jpg'
rgb_image_file = 'rgb.jpg'
rgb_registered_image_file = 'rgb_registered.jpg'
skin_smoothing_image_file = 'skin_smoothing.jpg'
shadow_detection_image_file = 'shadow_detection.jpg'

def sigint_handler(signal, frame):
    print('You pressed Ctrl+C!')
    sys.exit(0)

# image capture stub
def capture(capture_time, filename):
    while time.time() < capture_time:
        pass

    print 'Capturing image at time', time.time()

    with picamera.PiCamera() as camera:
        camera.resolution = (camera_resolution_horizontal, camera_resolution_horizontal)
        camera.start_preview()
        time.sleep(2)
        camera.capture(filename, 'jpeg')
        camera.stop_preview()

    return

def get_images():
    # connect to server (who has the rgb camera)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))

    try:
        # image capture time (in future)
        capture_time = time.time() + 1.0

        # sending capture time to server
        sock.sendall(repr(capture_time))

        # capture local image (in future as well)
        capture(capture_time, nir_image_file)

        print 'Receiving image data from server...'
        f = open(rgb_image_file, 'wb')
        data = sock.recv(4096)
        while len(data) > 0:
            f.write(data)
            data = sock.recv(4096)

        print 'Done receiving image.'

    finally:
        sock.close()


# register signal handler
signal.signal(signal.SIGINT, sigint_handler)

# start pan-tilt subprocess and wait for completion
pan_tilt_stdout, pan_tilt_stderr = subprocess.Popen(["/home/alarm/pan-tilt"], stdout=subprocess.PIPE).communicate()
print "operation requested = " + pan_tilt_stdout

get_images()

rgb_registered = registration.register(rgb_image_file, nir_image_file)
cv2.imwrite(rgb_registered_image_file, rgb_registered)

if pan_tilt_stdout == op_skin_smoothing:
    final_image = merging.merge(rgb_registered_image_file, nir_image_file)
    cv2.imwrite(skin_smoothing_image_file, final_image)

elif pan_tilt_stdout == op_shadow_detection:
    final_image = shadow_detection.shadowDetection(rgb_registered_image_file, nir_image_file)

