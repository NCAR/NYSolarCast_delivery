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
    """ Main driver.  Runs hrrr_grib2nc.process() in real-time or over a historical period
    """
    usage_str = "%prog"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-s", "--start_date", dest="start_date", help="yyyymmddhhmm")
    parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmddhhmm")
    parser.add_option("-d", "--date", dest="date", help="yyyymmddhhmm")
        
    (options, args) = parser.parse_args()
    
    logg_base = "%s/run_hrrr_grib2nc" % (sys_path.Log_dir)

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
        # Subtract 1 hour from current time for Real-Time processing
        current_time = ptime - (ptime % file_time_res) - 3600
        current_dt = pd.to_datetime(current_time, unit='s')
        # if the current hour is 7 (current local hour = 08), look for the 06z day-ahead
        if current_dt.hour == 7:
            current_dt -= pd.Timedelta(hours=1) 
        date_list = [current_dt]

    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    hrrr_fcst_base = "/glade/scratch/jaredlee/EPRI/Phase3/wps/grib/hrrr/"
    output_dir = os.path.join(sys_path.Data_base_dir, "hrrr_nc")

    for dt in date_list:
        if dt.hour not in [6,9,10,11,12,13,14,15,16,17]:
            logg.write_time("Info:  No hrr to process for hour: %d\n" % dt.hour)
            continue
        logg.write_time("Processing hrrr for datetime: %s\n" % (dt.strftime("%Y%m%d %H")))

        ret = hrrr_grib2nc.process(hrrr_fcst_base, dt.strftime("%Y%m%d%H"), output_dir, logg)

        logg.write_time("Ret: %d\n" % ret)

    logg.write_ending()

if __name__ == "__main__":
    main()
