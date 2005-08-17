#!/usr/bin/python

############################################################################
# Tests that meta data is being inserted correctly
# (c) 2005 James Bellenger <jbellenger@pristine.gm>
#
# Depends on: Python 2.2, PyQt
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

import socket
import sys
from sre import *

REQ = """
GET /%s HTTP/1.0 
Host: 192.168.0.100 
User-Agent: TESTER
Accept: */* 
Icy-MetaData:1 

""".lstrip()
BUF_SIZE = 4096

addr = sys.argv[1]
host, port, mount = findall(r'http://(.*):(\d{4})/(.*)', addr)[0]
port = int(port)
print 'host = %s \tport = %d\tmount=%s' % (host, port, mount)
s = socket.socket()
s.connect( (host, port) )
s.send( REQ % mount)

fd = s.makefile('r')
headers = ''
line = fd.readline()
while len(line.strip()):
    headers += line
    line = fd.readline()
print headers

meta_interval = int(findall( r'icy-metaint:\s*(\d*)\r\n', headers.lower() )[0])
print 'icy-metaint = %d'  % meta_interval
print '\n'

buf = s.recv(BUF_SIZE)
byte_counter = len(buf)
while True:
    amt_to_recv = BUF_SIZE
    if byte_counter == meta_interval:
        meta_len = 16 * ord(s.recv(1))
        meta = s.recv(meta_len)
        if meta_len > 0: print '[%d bytes] %s' % (meta_len, meta)
        byte_counter = 0
    else:
        if byte_counter + BUF_SIZE > meta_interval:
            amt_to_recv = meta_interval - byte_counter
    buf = s.recv(amt_to_recv)
    byte_counter += len(buf)
