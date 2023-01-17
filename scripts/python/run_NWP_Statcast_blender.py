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
THIS RUNS THE CODE TO BLEND THE NWP MODELS (HRRR AND WRF)
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
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
        
    (options, args) = parser.parse_args()

    if options.log:
        logg_base = options.log
    else:
        logg_base = "%s/run_NWP_Statcast_blender" % (sys_path.Log_dir)

    file_time_res = 900
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
        date_list = [current_dt]

    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    config_file = os.path.join(sys_path.config_dir, "NWP_Statcast_blend_conf_40km.json")
    site_list_file = os.path.join(sys_path.site_list_dir, "nysm.csv")
    grid_map_file = os.path.join(sys_path.config_dir, "grid_site_map_40km.json")

    proc_script = os.path.join(sys_path.scripts_dir, "NWP_Statcast_blender.py")
    statcast_fcst_dir = os.path.join(sys_path.Statcast_fcst_dir, "netcdf")
    nwp_fcst_dir = os.path.join(sys_path.Data_base_dir, "nwp_blend")
    output_dir = os.path.join(sys_path.Data_base_dir, "nwp_statcast_blend")
    output_hourly_dir = os.path.join(sys_path.Data_base_dir, "nwp_statcast_blend_hourly")
    cz_file = os.path.join(sys_path.site_list_dir, "wrf_grid_site_list_climateZone.csv")
    
    ret = 0
    for dt in date_list:
        # Only process specific hours
        if dt.hour not in [11, 12, 13, 14, 15, 16, 17, 18, 19]:
            logg.write_time("Ignoring time %s because there isn't a wrf at this hour\n" % dt.strftime("%Y/%m/%d %H:%M"))
            continue
        
        # Get input statcast file
        statcast_file = os.path.join(statcast_fcst_dir, dt.strftime("%Y%m%d"), 
                                     "ghi_fcst.nys.%s.nc" % dt.strftime("%Y%m%d.%H%M00"))
        if not os.path.exists(statcast_file):
            logg.write_time("Warning: %s doesn't exist.  Skipping time\n" % statcast_file)
            continue
        
        # Get input NWP file
        nwp_file = os.path.join(nwp_fcst_dir, dt.strftime("%Y%m%d"),
                                "wrf_hrrr_blend.%s.nc" % dt.strftime("%Y%m%d.%H00"))
        if not os.path.exists(nwp_file):
            logg.write_time("Warning: %s doesn't exist.  Skipping time\n" % nwp_file)
            continue
            
        # Make output directory
        output_dt_dir = os.path.join(output_dir, dt.strftime("%Y%m%d"))
        if not os.path.exists(output_dt_dir):
            logg.write_time("Making: %s\n" % output_dt_dir)
            os.makedirs(output_dt_dir)

        output_file = os.path.join(output_dt_dir, "NWP_statcast_blend.%s.nc" % (dt.strftime("%Y%m%d.%H%M")))

        # Add hourly output area if minute == 00
        hr_str = ""
        if dt.minute == 0:
            output_hourly_dt_dir = os.path.join(output_hourly_dir, dt.strftime("%Y%m%d"))
            if not os.path.exists(output_hourly_dt_dir):
                logg.write_time("Making: %s\n" % output_hourly_dt_dir)
                os.makedirs(output_hourly_dt_dir)

            output_hourly_file = os.path.join(output_hourly_dt_dir, "NWP_statcast_blend_hourly.%s.nc" % (dt.strftime("%Y%m%d.%H%M")))
            hr_str = "-H %s" % output_hourly_file

        cmd = "%s %s %s %s %s %s %s %s -z %s %s -l %s" % (proc_script, nwp_file, statcast_file, site_list_file, grid_map_file, 
                                                          config_file, dt.strftime("%Y%m%d.%H%M"), output_file, cz_file, hr_str, logg_base) 
        logg.write_time("Executing: %s\n" % cmd)
        ret = os.system(cmd)
        logg.write_time("Ret: %s\n" % ret)

    logg.write_ending(ret)

if __name__ == "__main__":
    main()
