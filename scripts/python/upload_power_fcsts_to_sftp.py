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
THIS UPLOADS THE POWER FORECAST FILES TO THE SFTP SERVER
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import NYSC_sys_path as sys_path
import pysftp
import pandas as pd

def main():
    """ Main driver function, uploads forecasts to a hardcoded sftp site
    """
    usage_str = "%prog"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-s", "--start_date", dest="start_date", help="yyyymmddhhmm")
    parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmddhhmm")
    parser.add_option("-d", "--date", dest="date", help="yyyymmddhhmm")
        
    (options, args) = parser.parse_args()
    
    logg_base = "%s/upload_power_fcst_to_sftp" % (sys_path.Log_dir)

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
        # Look back over previous hour
        LOOKBACK = 3600
        prev_sec = file_time_res
        while prev_sec <= LOOKBACK:
            date_list.append(current_dt - pd.Timedelta(seconds=prev_sec))
            prev_sec += file_time_res

    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    power_fcst_dir = os.path.join(sys_path.Data_base_dir, "pct_power_fcst", "netcdf")
    total_power_fcst_dir = os.path.join(sys_path.Data_base_dir, "total_power_fcst")

    FTP_HOST = "transfer.data.epri.com"
    FTP_USER = "SVC_APP_NCARForecast"
    FTP_PASS = "BrCRmh>a86[xU(/p12fq"
    log = logg_base+".ftp.log"

    srv = pysftp.Connection(host=FTP_HOST, username = FTP_USER, password=FTP_PASS, log=log)
    
    sftp_fcst_dir = "/Intake/DS000087-NCAR_NYSERDA_SolarPower_Forecast/forecasts"
    if os.path.basename(sftp_fcst_dir) not in srv.listdir(os.path.dirname(sftp_fcst_dir)):
        logg.write_time("Warning: %s doesn't exist.  Making it\n" % sftp_fcst_dir)
        srv.mkdir(sftp_fcst_dir)


    # List of power sites 
    POWER_NAMES = sys_path.FARMS + ['distributed']

    for dt in date_list:
        if dt.hour < 11 or dt.hour > 19:
            continue

        dt_subdir = os.path.join(sftp_fcst_dir, dt.strftime("%Y%m%d"))
        # If the directory doesn't exist, make it
        if dt.strftime("%Y%m%d") not in srv.listdir(sftp_fcst_dir):
            logg.write_time("Making: %s\n" % dt_subdir)
            srv.mkdir(dt_subdir)

        for name in POWER_NAMES:
            #
            # Copy the percent power forecasts
            #
            input_file = os.path.join(power_fcst_dir, dt.strftime("%Y%m%d"),
                                      "power_pct_cap.%s.%s00.nc" % (name, 
                                                                    dt.strftime("%Y%m%d.%H%M")))
            # If file doesn't exist, continue
            if not os.path.exists(input_file):
                logg.write_time("%s doesn't exist.  Skipping\n" % input_file)
            else:

                # If the file already exists, don't copy again
                file_name = os.path.basename(input_file)
                if file_name in srv.listdir(dt_subdir):
                    logg.write_time("%s already exists in ftp site, skipping\n" % file_name)
                else:
                    logg.write_time("Copying %s -> %s\n" % (input_file, dt_subdir))
                    with srv.cd(dt_subdir):
                        srv.put(input_file)

            #
            # Copy the total power forecasts
            #
            if name == 'distributed':
                # Don't do distributed for total power (it's done in a separate script)
                continue
            input_file = os.path.join(total_power_fcst_dir, dt.strftime("%Y%m%d"),
                                      "total_power.%s.%s.nc" % (name, 
                                                                dt.strftime("%Y%m%d.%H%M")))
            # If file doesn't exist, continue
            if not os.path.exists(input_file):
                logg.write_time("%s doesn't exist.  Skipping\n" % input_file)
            else:
                # If the file already exists, don't copy again
                file_name = os.path.basename(input_file)
                if file_name in srv.listdir(dt_subdir):
                    logg.write_time("%s already exists in ftp site, skipping\n" % file_name)
                else:
                    logg.write_time("Copying %s -> %s\n" % (input_file, dt_subdir))
                    with srv.cd(dt_subdir):
                        srv.put(input_file)

    srv.close()
    logg.write_ending()

if __name__ == "__main__":
    main()
