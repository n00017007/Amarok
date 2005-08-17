############################################################################
# Debug output
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


import sys
import traceback
import datetime

DEBUG_FNAME = 'shouter.debug'
debug_prefix = "[Shouter] "
debug_h = open(DEBUG_FNAME, 'a')
def debug( message ):
    dt = datetime.datetime.now()
    now = '%s:%s:%s.%s' % (str(dt.hour).zfill(2), str(dt.minute).zfill(2), str(dt.second).zfill(2), str(dt.microsecond / 1000).zfill(3))
    debug_h.write( '%s %s %s\n' % (debug_prefix.strip(), now, str(message)))
    debug_h.flush()
