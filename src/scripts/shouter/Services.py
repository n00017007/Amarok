############################################################################
# Implementation of services
# (c) 2005 James Bellenger <jamesb@squaretrade.com>
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

from Amarok import *
from sre import *
import os
import binfuncs
import urllib
from ShouterExceptions import *
import Playlist
import time
import socket
import SocketErrors

META = "%cStreamTitle='%s';StreamUrl='%s';%s"
ICYRESP = 'ICY 200 OK\r\n' + \
          'icy-notice1: %s\r\n' + \
          'icy-notice2: %s\r\n' + \
          'icy-name: %s\r\n' + \
          'icy-genre: %s\r\n' + \
          'icy-url: %s\r\n' + \
          'icy-metaint: %d\r\n\r\n'
PADDING = '\x00'*16
BURST_CREDIT_INIT = 100 * 1024
MIN_SILENT_BR = 48
MAX_STALL_INT = .4

class _Service:
    cfg = None
    server_cfg = None
    socket = None
    headers = None
    path = None
    req = None

        
class _BaseStreamService(_Service):
    byte_count = 0
    meta_is_dirty = True
    icy = False
    pl = None
    burst_credit = 0

    def __init__(self, stream_ctrl, cfg, server_cfg):
        debug('init _BaseStreamService on %s' % cfg.mount)
        self.socket = stream_ctrl.request
        
        #self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        self.headers = stream_ctrl.headers
        self.path = stream_ctrl.path
        self.req = stream_ctrl
        self.cfg = cfg
        self.server_cfg = server_cfg
        self.byte_counter = 0
        self.burst_credit = BURST_CREDIT_INIT
        if self.headers.get('Icy-Metadata'):
            self.icy = True

    def get_meta(self, fobj):
        """ Transform ID3 info into something appropriate for streams """

        if self.meta_is_dirty:
            stream_title = fobj.get_meta()
            url = self.server_cfg.url

            # The literal 28 is the number of static characters in the META string (see top)
            length = len(stream_title) + len(url) + 28
            padding = 16 - length % 16 
            meta = META % ( (length + padding)/16, stream_title, url, PADDING[:padding] )
            self.meta_is_dirty = False
            return meta
        else:
            return '\x00'

    # Merge this with the stream method
    def check_format(self, fobj):
        if fobj.url.lower().endswith('mp3'):
            return
        raise bad_format_error

    def stall(self, stall_int):
        time.sleep(stall_int)

    def stream_silence(self, br):
        fobj = Playlist.SilentFile(br)
        self.stream(fobj, 0)
            

    def stream(self, fobj, pos, condition='True'):
        debug('_BaseStreamService stream %s' % fobj.title )
        buf = 0
        bufsize = self.server_cfg.buf_size
        icy_int = self.server_cfg.icy_interval

        br = fobj.bitrate
        if not br:
           br = Amarok.get_bitrate(fobj.url)
        sleep_factor = 8.0/(br * 1024.0) 

        fname = fobj.get_fname()
        f = file(fname, 'r')
        fsize = os.stat(fname)[6]
        mp3_start = binfuncs.get_mp3_start(fname)
        f.seek(fsize * pos + mp3_start)

        self.meta_is_dirty = True
        while f.tell() < fsize and eval(condition):
            bytes_till_meta = icy_int - self.byte_count
            if bytes_till_meta == 0:
                # send meta and reset counter
                if self.icy:
                    meta = self.get_meta(fobj)
                    self.socket.send(meta)
                self.byte_count = 0
            else:
                if bytes_till_meta < bufsize :
                    # send whats left. Will update meta on next iteration
                    buf = f.read(bytes_till_meta)
                    self.socket.send(buf)
                    self.byte_count += len(buf)
                else :
                    # Send a normal buffer
                    buf = f.read(bufsize)
                    self.socket.send(buf)
                    self.byte_count += len(buf)

                if self.burst_credit > 0:
                    self.burst_credit = self.burst_credit - len(buf)
                else:
                    sleep_int = len(buf) * sleep_factor
                    time.sleep(sleep_int)

        self.meta_is_dirty = True

        
class Service0(_BaseStreamService):
    """ Live stream """
    astate = None
    astate_old = None

    def __init__(self, stream_ctrl, cfg, server_cfg):
        self.pl = Playlist.LivePlaylist()
        _BaseStreamService.__init__(self, stream_ctrl, cfg, server_cfg)

    def start(self):
        debug('Service0 start')
        if self.icy:
            header = ICYRESP % (
                        self.server_cfg.desc1,
                        self.server_cfg.desc2,
                        self.cfg.name,
                        self.cfg.genre,
                        self.server_cfg.url,
                        self.server_cfg.icy_interval)
            self.socket.send(header)
        else:
            self.req.send_response(200)
            self.req.send_header('Content-Type', 'audio/x-mpegurl')
            self.req.end_headers()

        while True:
            self.astate = Amarok.state
            if self.astate_old and (self.astate_old == self.astate):
                debug('Buffer overrun in request %s. Stalling' % str(self.socket))
                self.stall(MAX_STALL_INT)
            else: 
                try:
                    (fobj, frac) = self.pl.get_play_cursor() 
                    self.check_format(fobj)
                    condition = '%d == Amarok.state' % self.astate
                    self.stream(fobj, frac, condition)
                    self.astate_old = self.astate
                except (bad_format_error, amarok_not_playing_error, indeterminate_queue_error):
                    while Amarok.state == self.astate:
                        self.stream_silence(MIN_SILENT_BR)


class Service1(_BaseStreamService):
    """ Playlist snapshot """
    fobj = None
    fobj_old = None
    def __init__(self, stream_ctrl, cfg, server_cfg):
        self.pl = Playlist.StaticPlaylist(cfg.stream_type1_arg, cfg.random, cfg.repeat_pl)
        _BaseStreamService.__init__(self, stream_ctrl, cfg, server_cfg)

    def start(self):
        debug('Service1 start')
        if self.icy:
            header = ICYRESP % (
                        self.server_cfg.desc1,
                        self.server_cfg.desc2,
                        self.cfg.name,
                        self.cfg.genre,
                        self.server_cfg.url,
                        self.server_cfg.icy_interval)
            self.socket.send(header)
        else:
            self.req.send_response(200)
            self.req.send_header('Content-Type', 'audio/x-mpegurl')
            self.req.end_headers()

        while True:
            self.fobj, frac = self.pl.get_play_cursor()
            if self.fobj_old and (self.fobj_old is self.fobj):
                self.fobj = self.pl.get_next_file(self.fobj)
                frac = 0.0
            try:
                self.check_format(self.fobj)
                self.stream(self.fobj, frac)
                self.fobj_old = self.fobj
            except bad_format_error:
                self.stream_silence(MIN_SILENT_BR)
            #except socket.error: 
            except:
                errno = sys.exc_info()[1][0]
                if errno == SocketErrors.RESOURCE_UNAVAILBLE:
                    debug('Caught error ' + str(sys.exc_info()[1]) + ' and doing nothing')
                else:
                    raise


class Service2(_BaseStreamService):
    """ Directory """
    def __init__(self, stream_ctrl, cfg, server_cfg):
        self.pl = Playlist.DirectoryPlaylist(cfg.stream_type2_arg, cfg.random, 1 ) #cfg.repeat_pl)
        _BaseStreamService.__init__(self, stream_ctrl, cfg, server_cfg)

    def start(self):
        debug('Service1 start')

class Service3(_Service):
    """ Upload """
    def start(self):
        pass
