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
THIS RUNS THE CODE TO CONVERT WRF NETCDF FILES TO A FORMAT USABLE BY STATCAST
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
    
    logg_base = "%s/run_WrfNetCDF2StatcastNetCDF" % (sys_path.Log_dir)

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
        # For non DST times  
        #current_time = ptime - (ptime % file_time_res) + 3600
        #current_time = ptime - (ptime % file_time_res)
        # Subtract a day (Timing not important and the WRF files are often late)
        current_time = ptime - (ptime % file_time_res) - 86400 + 3600
        current_dt = pd.to_datetime(current_time, unit='s')
        date_list = [current_dt]

    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    wrf_fcst_base = os.path.join(sys_path.WRF_fcst_dir, "nowcast")
    wrf_day_ahead_fcst_base = os.path.join(sys_path.WRF_fcst_dir, "dayahead")
    site_list = os.path.join(sys_path.site_list_dir, "nymeso_match_wrf.csv")

    proc_script = os.path.join(sys_path.scripts_dir, "WrfNetCDF2StatcastNetCDF.py")
    output_dir = os.path.join(sys_path.Data_base_dir, "wrf_fcst_4_statcast")
    variables = "Q2,SWDDNI,SWDDIF,SWDOWN,TAOD5502D,CLDFRAC2D,WVP,WP_TOT_SUM,TAU_QC_TOT,TAU_QS,T2,PSFC,CLRNIDX,U10,V10,TAU_QI_TOT"
    

    ret = 0
    for dt in date_list:
        logg.write_time("Processing wrf for datetime: %s\n" % (dt.strftime("%y-%m-%d_%H")))
        # Skip hours before 11 or after 19 because wrf isn't ran
        if dt.hour < 11 or dt.hour > 19:
            logg.write_time("No wrf for hour: %s\n" % dt.hour)
            continue

        output_dt_dir = os.path.join(output_dir, dt.strftime("%Y%m%d"))
        if not os.path.exists(output_dt_dir):
            logg.write_time("Creating: %s\n" % output_dt_dir)
            os.makedirs(output_dt_dir)

        output_file = os.path.join(output_dt_dir,
                                   "wrf_nc.%s.nc" % dt.strftime("%Y%m%d.%H%M"))
        cmd = "%s %s %s %s %s %s -d %s -l %s" % (proc_script, wrf_fcst_base, dt.strftime("%Y%m%d.%H%M"), site_list, variables, output_file, 
                                                 wrf_day_ahead_fcst_base, logg_base)
        logg.write_time("Executing: %s\n" % cmd)
        ret = os.system(cmd)
        logg.write_time("Ret: %d\n" % ret)

    logg.write_ending(ret)

if __name__ == "__main__":
    main()
