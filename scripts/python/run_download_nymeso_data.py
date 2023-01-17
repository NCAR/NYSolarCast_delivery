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
THIS IS USED TO DOWNLOAD THE REAL-TIME NYMESONET DATA
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import NYSC_sys_path as sys_path
import pandas as pd


def main():
    """ Main driver.  Calls 'proc_script' in real-time
    """

    usage_str = "%prog"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-s", "--start_date", dest="start_date", help="yyyymmddhhmm")
    parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmddhhmm")

    (options, args) = parser.parse_args()


    if options.start_date and options.end_date:
        log_base = "%s/download_nymeso_data_pb" % sys_path.Log_dir
        start_dt = pd.to_datetime(options.start_date, format="%Y%m%d%H%M")
        end_dt = pd.to_datetime(options.end_date, format="%Y%m%d%H%M")
        date_range = pd.date_range(start_dt, end_dt, freq='5T')
        date_list = []
        for dt in date_range:
            date_list.append(dt.strftime("%Y%m%d.%H%M"))
    else:
        log_base = "%s/download_nymeso_data" % sys_path.Log_dir

        curr_dt = time.time()
        # Round down to nearest 5-min
        curr_dt = curr_dt - (curr_dt % 300)
        date_str = time.strftime("%Y%m%d.%H%M", time.gmtime(curr_dt))
        date_list = [date_str]

    logg = log_msg.LogMessage(log_base)
    logg.set_suffix(".pyl")

    logg.write_starting()
    
    proc_script = os.path.join(sys_path.scripts_dir, "download_nymeso_data.py")
    output_dir = os.path.join(sys_path.Data_base_dir, "phase3/NYMesonet_real_time/ascii/")

    for date_str in date_list:
        output_dt_dir = os.path.join(output_dir, date_str[:8])
        if not os.path.exists(output_dt_dir):
            cmd = "mkdir -p %s" % output_dt_dir
            run_cmd(cmd, logg)        

        output_file = os.path.join(output_dt_dir, "nymeso_obs.%s.csv" % date_str)
        
        date_arg_str = ""
        if options.start_date and options.end_date:
            date_arg_str = "-s %sT%s -e %sT%s" % (date_str[:8], date_str[9:], date_str[:8], date_str[9:])
 
        cmd = "%s %s -l %s %s" % (proc_script, output_file, log_base, date_arg_str)
        run_cmd(cmd, logg)

    logg.write_ending()

def run_cmd(cmd, logg):
    logg.write_time("Executing: %s\n" % cmd)
    ret = os.system(cmd)
    if ret != 0:
        logg.write_time("Error: %s\n" % ret)
        sys.exit()
    logg.write_time("Success\n")
    return ret

if __name__ == "__main__":
    main()
