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
THIS RUNS THE CODE TO PERFORM THE PERCENT POWER ROLLUP
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
import NYSC_sys_path as sys_path

def main():
    """ Main driver.  Calls 'proc_script' in real-time or over a historical period
    """
    usage_str = "%prog"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-s", "--start_date", dest="start_date", help="yyyymmddhhmm")
    parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmddhhmm")
    parser.add_option("-d", "--date", dest="date", help="yyyymmddhhmm")
        
    (options, args) = parser.parse_args()
    
    logg_base = "%s/run_pct_power_rollup" % (sys_path.Log_dir)

    file_time_res = 3600
    date_list = []
    if options.start_date and options.end_date:
        start_dt = pd.to_datetime(options.start_date, format="%Y%m%d%H%M")
        end_dt = pd.to_datetime(options.end_date, format="%Y%m%d%H%M")
        date_list = pd.date_range(start_dt, end_dt, freq=str(file_time_res)+'S')
        logg_base += "_pb"
    elif options.date:
        date_list = [pd.to_datetime(options.date, format="%Y%m%d%H%M")]
        logg_base += "_pb"
    else:
        ptime = time.time()
        current_time = ptime - (ptime % file_time_res)
        current_dt = pd.to_datetime(current_time, unit='s')
        # If current hour == 9, set it to 6 (because day-ahead is a few hours late)
        if current_dt.hour == 9:
            current_dt = current_dt.replace(hour=6)

        date_list = [current_dt]

    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    proc_script = os.path.join(sys_path.scripts_dir, "pct_power_rollup.py")
    site_list = os.path.join(sys_path.site_list_dir, "pv_match_grid_within_NYS.csv")
    input_dir = os.path.join(sys_path.Fcst_dir, "netcdf")
    output_dir = sys_path.Total_fcst_dir
    
    for dt in date_list:
        # Only process specific hours
        if dt.hour not in [6, 11, 12, 13, 14, 15, 16, 17, 18, 19]:
            logg.write_time("Ignoring time %s because there isn't a fcst at this hour\n" % dt.strftime("%Y/%m/%d %H:%M"))           
            continue
            

        input_file = "%s/%s/power_pct_cap.distributed.%s.nc" % (input_dir, dt.strftime("%Y%m%d"), dt.strftime("%Y%m%d.%H%M%S"))
        if not os.path.exists(input_file):
            logg.write_time("Error: %s doesn't exist.  Skipping\n" % input_file)
            continue
           
        output_dt_dir = os.path.join(output_dir, dt.strftime("%Y%m%d"))
        if not os.path.exists(output_dt_dir):
            logg.write_time("Making: %s\n" % output_dt_dir)
            os.makedirs(output_dt_dir)
        
        output_file = os.path.join(output_dt_dir, "total_power_forecast.%s.nc" % (dt.strftime("%Y%m%d.%H%M")))

        cmd = "%s %s %s %s -l %s" % (proc_script, input_file, site_list, output_file, logg_base) 
        logg.write_time("Executing: %s\n" % cmd)
        ret = os.system(cmd)
        logg.write_time("Ret: %s\n" % ret)

    logg.write_ending()

if __name__ == "__main__":
    main()
