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
This runs the conversion from the hrrr grib to netCDF
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
import NYSC_sys_path as sys_path
import hrrr_grib2nc

def main():
    """ Main driver.  Calls 'proc_script' in real-time or over a historical period
    """
    usage_str = "%prog"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-s", "--start_date", dest="start_date", help="yyyymmddhhmm")
    parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmddhhmm")
    parser.add_option("-d", "--date", dest="date", help="yyyymmddhhmm")
        
    (options, args) = parser.parse_args()
    
    logg_base = "%s/run_NWP_grid_blender" % (sys_path.Log_dir)

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
        # Add 1 hour from current time for Real-Time processing
        current_time = ptime - (ptime % file_time_res) + 3600
        current_dt = pd.to_datetime(current_time, unit='s')
        if current_dt.hour == 10:
            # At 9am localtime, process the 6z (dayahead) 
            current_dt -= pd.Timedelta(hours=4)
        date_list = [current_dt]

    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    static_file = os.path.join(sys_path.config_dir, "WRF_HRRR_blend.json")
    static_file_dayahead = os.path.join(sys_path.config_dir, "WRF_HRRR_blend_dayAhead.json")
    proc_script = os.path.join(sys_path.scripts_dir, "NWP_grid_blender.py")
    output_dir = os.path.join(sys_path.Data_base_dir, "nwp_blend")
    hourly_output_dir = os.path.join(sys_path.Data_base_dir, "nwp_blend_hourly")
    cz_file = os.path.join(sys_path.site_list_dir, "wrf_grid_site_list_climateZone.csv")

    
    for dt in date_list:
        # Only process specific hours
        if dt.hour not in [6, 11, 12, 13, 14, 15, 16, 17, 18, 19]:
            logg.write_time("Ignoring time %s because there isn't a wrf at this hour\n" % dt.strftime("%Y/%m/%d %H:%M"))
            continue

        output_dt_dir = os.path.join(output_dir, dt.strftime("%Y%m%d"))
        if not os.path.exists(output_dt_dir):
            logg.write_time("Making: %s\n" % output_dt_dir)
            os.makedirs(output_dt_dir)
        
        output_file = os.path.join(output_dt_dir, "wrf_hrrr_blend.%s.nc" % (dt.strftime("%Y%m%d.%H00")))

       
        if dt.hour == 6:
            use_static_file = static_file_dayahead
            cz_flag = "-z %s" % cz_file

            hourly_output_dt_dir = os.path.join(hourly_output_dir, dt.strftime("%Y%m%d"))
            if not os.path.exists(hourly_output_dt_dir):
                logg.write_time("Making: %s\n" % hourly_output_dt_dir)
                os.makedirs(hourly_output_dt_dir)
        
            hourly_output_file = os.path.join(hourly_output_dt_dir, "wrf_hrrr_blend_hourly.%s.nc" % (dt.strftime("%Y%m%d.%H00")))

            hourly_flag = "-H %s" % hourly_output_file
        else:
            use_static_file = static_file
            cz_flag = ""
            hourly_flag = ""
            

        cmd = "%s %s %s %s -r -l %s %s %s" % (proc_script, dt.strftime("%Y%m%d.%H00"), use_static_file, output_file, logg_base, cz_flag, hourly_flag) 
        logg.write_time("Executing: %s\n" % cmd)
        ret = os.system(cmd)
        logg.write_time("Ret: %s\n" % ret)

    logg.write_ending()

if __name__ == "__main__":
    main()
