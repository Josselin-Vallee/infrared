 
import time

def callAt(calltime, function, params):
    while time.time() < calltime:
        pass
    return function(*params)
