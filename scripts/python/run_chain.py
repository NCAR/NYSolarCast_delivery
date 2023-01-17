#!/usr/bin/env python

"""Run chain of python scripts, most often used in crontab calls"""

# ============================================================================== #
#                                                                                #
#   (c) Copyright, 2012 University Corporation for Atmospheric Research (UCAR).  #
#       All rights reserved.                                                     #
#                                                                                #
#       File: $RCSfile: run_chain.py,v $                                           #
#       Version: $Revision: 1.1 $  Dated: $Date: 2015/03/18 15:56:30 $           #
#                                                                                #
# ============================================================================== #

import log_msg
import os
import sys
import tim
import time
from optparse import OptionParser

def main():

    usage_str = "%prog"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log message to specified file")

    (options, args) = parser.parse_args()

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".pyl")
    else:
        logg = log_msg.LogMessage("")
        
    logg.write_starting("invocation command: %s" %  " ".join(sys.argv))
    
    total_ret = 0
    for script in args:
        logg.write_info("executing: %s" % script)
        ret = os.system(script)
        logg.write_info("return: %d" % ret)
        if ret < 0:
            total_ret = ret
    logg.write_ending(exit_status=total_ret)
    sys.exit(total_ret)
    
if __name__ == "__main__":

   main()
       
