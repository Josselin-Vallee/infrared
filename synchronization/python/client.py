#!/usr/bin/python2

import socket
import sys
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#host = '192.168.1.13'
host = '127.0.0.1'
port = 1313
sock.connect((host, port))

try:
    value = time.time()
    data = repr(value)
    
    print 'Sending time to server:', value
    sock.sendall(data)

    data = sock.recv(256)
    print 'Response received from server:', data

finally:        
    sock.close()
