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
This script is designed to convert hrrr grib data to nc
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
import NYSC_sys_path as sys_path

grib2site_cdl = os.path.join(sys_path.cdl_dir, "hrrr_ghi.cdl")
grib2site_site_list = os.path.join(sys_path.site_list_dir, "wrf_grid_site_list.asc")

def process(in_dir, date, output_dir, logg):
    """ Main driver function

    Args:
        in_dir : string path to input directory
        date : string date (YYYYmmDDHH)
        output_dir : string path to output directory
        logg : log object

    Returns:
        ret : int exit status
    """
    dt = pd.to_datetime(date, format="%Y%m%d%H")
            
    ret = 0
    output_dt_dir = os.path.join(output_dir, dt.strftime("%Y%m%d"))
    if not os.path.exists(output_dt_dir):
        os.makedirs(output_dt_dir)

    # Add 2 hours to input date dir because Jared stores them this way (14z hrrr in 16z date dir) (except for day-ahead 06)
    if dt.hour != 6:
        in_dt_dir = os.path.join(in_dir, (dt+pd.Timedelta(hours=2)).strftime("%Y-%m-%d_%H"))
        lead_times = list(range(2,9))
        day_ahead = False
    else:
        in_dt_dir = os.path.join(in_dir, dt.strftime("%Y-%m-%d_%H"))
        lead_times = list(range(0,43)) 
        day_ahead = True

    if not os.path.exists(in_dt_dir):
        logg.write_time("Input dir doesn't exist: %s\n" % in_dt_dir)
        return 0

    # Look for files corresponding to the current gen time 
    in_file_list = []
    init_hr = dt.hour
    prev_init = False
    for l in lead_times:
        in_file = os.path.join(in_dt_dir, "hrrr.t%02dz.wrfprsf%02d.grib2" % (init_hr, l))
        if os.path.exists(in_file):
            in_file_list.append(in_file)
        else:
            logg.write_time("Warning: %s doesn't exist.  Looking at files for previous init time\n" % in_file)
            prev_init = True
            break

    if prev_init and not day_ahead:
        # Look for files at the previous hour init time,  Exit if not all correct files are found
        in_file_list = []
        lead_times = list(range(3, 10))
        init_hr = dt.hour - 1
        for l in lead_times:
            in_file = os.path.join(in_dt_dir, "hrrr.t%02dz.wrfprsf%02d.grib2" % (init_hr, l))
            if os.path.exists(in_file):
                in_file_list.append(in_file)
            else:
                logg.write_time("Warning: %s doesn't exist.  Exiting\n" % in_file)
                return -1

    # Create the output file
    output_nc_file = "%s/hrrr_ghi.%s.i%02d00" % (output_dt_dir, dt.strftime("%Y%m%d"), init_hr)
    if not os.path.exists(output_nc_file):
        # Create the nc file
        cmd = "ncgen -b -o %s %s" % (output_nc_file, grib2site_cdl)
        ret = run_cmd(cmd, logg)
        if ret != 0:
            logg.write_time("Error: ret: %d\n" % ret)
            return -1
    else:
        logg.write_time("Output file exists (%s).  Returning\n" % output_nc_file)
        return 0

    # Convert the grib files to netCDF using grib2site
    for in_file in in_file_list:
        cmd = "/glade/u/home/brummet/bin/grib2site %s %s %s < %s" % (grib2site_cdl, grib2site_site_list,
                                                                     output_nc_file, in_file)
        ret = run_cmd(cmd, logg)
        if ret != 0:
            logg.write_time("Warning: Ret: %d\n" % ret)                       
    
    return ret

def run_cmd(cmd, logg):
    """ Uses os to execute a specific command

    Args:
        cmd : string command to execute
        logg : log object

    Returns:
        ret : int return status from 'cmd'
    """
    logg.write_time("Executing: %s\n" % cmd)
    ret = os.system(cmd)
    return ret

def main():
    usage_str = "%prog input_dir date(yyyymmddhh) output_dir"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
        
    (options, args) = parser.parse_args()
    
    if len(args) < 2:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  
    input_dir = args[0]
    date = args[1]
    output_dir = args[2]

    logg.write_starting()

    process(input_dir, date, output_dir, logg)

    logg.write_ending()


if __name__ == "__main__":
    main()
