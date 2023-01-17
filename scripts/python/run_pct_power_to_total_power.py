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
THIS RUNS THE CODE TO CREATE TOTAL POWER FORECASTS FROM PERCENT POWER
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
import NYSC_sys_path as sys_path

MODEL_BASES = sys_path.FARMS

def main():
    """ Main driver.  Calls 'proc_script' in real-time or over a historical period
    """
    usage_str = "%prog"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-s", "--start_date", dest="start_date", help="yyyymmddhhmm")
    parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmddhhmm")
    parser.add_option("-d", "--date", dest="date", help="yyyymmddhhmm")
        
    (options, args) = parser.parse_args()

    if len(args) != 0:
        parser.print_help()
        sys.exit(2)

    
    logg_base = "%s/run_pct_power_to_total_power" % (sys_path.Log_dir)

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
        # If current hour == 9, set it to 6 (because day-ahead is a few hours late)
        if current_dt.hour == 9:
            current_dt = current_dt.replace(hour=6, minute=0)

        date_list = [current_dt]

    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    proc_script = os.path.join(sys_path.scripts_dir, "pct_power_to_total_power.py")
    capacity_list = os.path.join(sys_path.site_list_dir, "farm_capacity_list.csv")
    input_dir = os.path.join(sys_path.Fcst_dir, "netcdf")
    output_dir = sys_path.Total_fcst_dir

    # Get the capacity map  (Site -> Capacity)
    capacity_df = pd.read_csv(capacity_list)
    capacity_map = {}
    for ind, row in capacity_df.iterrows():
        capacity_map[row['Site']] = row['Estimated_Capacity_kW']

   
    for dt in date_list:
        # Only process specific hours
        if dt.hour not in [6, 11, 12, 13, 14, 15, 16, 17, 18, 19]:
            logg.write_time("Ignoring time %s because there isn't a fcst at this hour\n" % dt.strftime("%Y/%m/%d %H:%M"))           
            continue

        if dt.hour == 6 and dt.minute != 0:
            logg.write_time("Ignoring time %s because there isn't a fcst at this hour\n" % dt.strftime("%Y/%m/%d %H:%M"))           
            continue            

        # Run the code for each model base
        for model_base in MODEL_BASES:
            input_file = "%s/%s/power_pct_cap.%s.%s.nc" % (input_dir, dt.strftime("%Y%m%d"), model_base, 
                                                           dt.strftime("%Y%m%d.%H%M%S"))
            if not os.path.exists(input_file):
                logg.write_time("Error: %s doesn't exist.  Skipping\n" % input_file)
                continue
           
            output_dt_dir = os.path.join(output_dir, dt.strftime("%Y%m%d"))
            if not os.path.exists(output_dt_dir):
                logg.write_time("Making: %s\n" % output_dt_dir)
                os.makedirs(output_dt_dir)
        
            output_file = os.path.join(output_dt_dir, "total_power.%s.%s.nc" % (model_base, 
                                                                            dt.strftime("%Y%m%d.%H%M")))


            if model_base not in capacity_map:
                logg.write_time("Error: %s not listed in capacity file: %s\n" % (model_base, capacity_list))
                continue
            else:
                capacity = capacity_map[model_base]

            cmd = "%s %s %s %s -l %s" % (proc_script, input_file, capacity, output_file, logg_base) 
            logg.write_time("Executing: %s\n" % cmd)
            ret = os.system(cmd)
            logg.write_time("Ret: %s\n" % ret)

    logg.write_ending()

if __name__ == "__main__":
    main()
