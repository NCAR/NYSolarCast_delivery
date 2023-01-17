#!/usr/bin/env python

# ============================================================================== #
#                                                                                #
#   (c) Copyright, 2013 University Corporation for Atmospheric Research (UCAR).  #
#       All rights reserved.                                                     #
#                                                                                #
#       File: $RCSfile: fileheader,v $                                           #
#       Version: $Revision: 1.1 $  Dated: $Date: 2013/09/30 14:44:18 $           #
#                                                                                #
# ============================================================================== #

"""
This runs ghi_fcst.py 
"""

import time, os, sys
import NYSC_sys_path as sys_path
from datetime import datetime
from optparse import OptionParser
import log_msg
import errno
import pandas as pd

def main():
    """
    This is a wrapper script for ghi_fcst.py. It does the following:
    1) sets up and performs logging 
    2) defines run time mode depending on user specified options: time interval, i
       single date, or real-time (no time options), 
    3) passes in arguments to ghi_fcst.py that are relatively static for this project:
       number of forecasts to be computed(24), the forecast time resolution(15 min), 
       the time resolution of the input observation data (15min), the statistical model 
       basename (nys), application debug level ( 4 = high)
    4) executes ghi_fcst.py, logs result
   
    Args:
       none
    Returns:
       none
    """

    usage_str = "%prog"

    parser = OptionParser(usage = usage_str)

    #
    # Run options for ghi_fcst.py :  -b and -e for begin and end interval
    # and -d for a particular time
    #
    parser.add_option("-b", "--begin_date", dest="begin_date", help="yyyymmdd")
    parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmdd")
    parser.add_option("-d", "--date", dest="date", help="yyyymmddhhmm")

    (options, args) = parser.parse_args()
    
    if len(args) < 0:
        parser.print_help()
        sys.exit(2)

    # 
    # Create log structure. Dated directory is created corresponding to 
    # time processed.
    # 
    date_str_option = ""

    #
    # Configure the logg object
    # 
    logg_base = "%s/ghi_fcst" % (sys_path.Log_dir) 

    #
    # If processing interval, or processing single time set those run options 
    # and the sppropriate log directory date
    #
    if options.begin_date and options.end_date:
        date_str_option = "-b %s -e %s" % (options.begin_date, options.end_date)
        logg_base += "_pb"
    elif options.date:
        date_str_option = "-d %s" % options.date
        logg_base += "_pb"
    else:
        # Added to handle latency in the real time system
        ptime = time.time()
        # Subtract a day (Timing not important and the WRF files are often late)
        current_time = ptime - (ptime % 900) - 86400
        date_str_option = "-d %s" % pd.to_datetime(current_time, unit='s').strftime("%Y%m%d%H%M")
   
    logg = log_msg.LogMessage(logg_base)

    logg.set_suffix(".pyl")

    #
    # Execution of ghi_fcst.py
    #
    logg.write_starting()

    proc_script = os.path.join(sys_path.scripts_dir, "ghi_fcst.py")

    #cmd = "%s %s -l %s -g 4 -t 15 -r 15 24 nys " % (proc_script,  date_str_option, logg_base)
    cmd = "%s %s -l %s -t 15 -r 15 24 nys " % (proc_script,  date_str_option, logg_base)

    logg.write_time("Executing: %s\n" % cmd)

    ret = os.system(cmd)
    if ret == 0:
        logg.write_time("Success: %d\n" % ret)
    else:
        logg.write_time("Error: Ret: %d\n" % ret)
    

    logg.write_ending(ret)

if __name__ == "__main__":
    main()
