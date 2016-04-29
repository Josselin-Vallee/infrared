#!/usr/bin/python2

import time
import timedfunction

def testfunction(s1, s2):
    print 'Starttime:', s1
    print 'Alarmtime:', s2
    print 'Currenttime:', time.time()
    return

current = time.time()
alarm = current + 1.0

timedfunction.callAt(alarm, testfunction, (repr(current), repr(alarm)))
