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
THIS RUNS THE CODE TO CONVERT THE 5-MIN NYMESO DATA TO 15MIN AVERAGES
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
    
    logg_base = "%s/run_nymeso_15min_avg" % (sys_path.Log_dir)

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

    nymeso_base_dir = os.path.join(sys_path.Data_base_dir, "phase3/NYMesonet_real_time")
    nymeso_qc_dir = os.path.join(nymeso_base_dir, "qc")
    nymeso_avg_dir = os.path.join(nymeso_base_dir, "15min")    

    proc_script = os.path.join(sys_path.scripts_dir, "avgObs.py")
    static_file = os.path.join(sys_path.config_dir, "nymeso_15min_avg.json") 
    
    for dt in date_list:
        # Get the 3 files for this dt
        input_files = []
        for prev_mins in [10,5,0]:
            prev_dt = dt-pd.Timedelta(minutes=prev_mins)
        
            input_file = os.path.join(nymeso_qc_dir, prev_dt.strftime("%Y%m%d"),
                                      'nymeso_qc_obs.%s.csv' % prev_dt.strftime("%Y%m%d.%H%M"))
            if not os.path.exists(input_file):
                logg.write_time("Warning: %s doesn't exist\n" % input_file)
            else:
                input_files.append(input_file)

        if len(input_files) == 0:
            logg.write_time("Error: No previous input files.  Skipping time: %s\n" % dt.strftime("%Y%m%d.%H%M"))
            continue

        output_dt_dir = os.path.join(nymeso_avg_dir, dt.strftime("%Y%m%d"))
        if not os.path.exists(output_dt_dir):
            logg.write_time("Creating: %s\n" % output_dt_dir)
            os.makedirs(output_dt_dir)

        output_file = os.path.join(output_dt_dir, "nymeso_15min_obs.%s.csv" % dt.strftime("%Y%m%d.%H%M"))

        # Run the code
        cmd = "%s %s %s %s -l %s -k -d %s" % (proc_script, ",".join(input_files), static_file, output_file, logg_base, dt.strftime("%Y%m%d.%H%M"))
        logg.write_time("Executing: %s\n" % cmd)
        ret = os.system(cmd)
        logg.write_time("Ret: %d\n" % ret)

    logg.write_ending()

if __name__ == "__main__":
    main()
