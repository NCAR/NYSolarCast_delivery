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
THIS SCRIPT WAS DESIGNED TO PRODUCE THE TOTAL POWER FORECAST BASED ON THE PCT POWER FORECAST
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
from netCDF4 import Dataset, stringtoarr
import numpy as np

ZIP_REPLACE_MAP = {'100' : '100_102',
                   '102' : '100_102',
                   '105' : '105_106_107_108',
                   '106' : '105_106_107_108',
                   '107' : '105_106_107_108',
                   '108' : '105_106_107_108',
                   '117' : '117_118',
                   '118' : '117_118',
                   '120' : '120_121',
                   '121' : '120_121',
                   '130' : '130_131',
                   '131' : '130_131',
                   '133' : '133_134',
                   '134' : '133_134',
                   '137' : '137_138',
                   '138' : '137_138',
                   '140' : '140_141_143',
                   '141' : '140_141_143',
                   '143' : '140_141_143',
                   '144' : '144_145',
                   '145' : '144_145'}
                   
ZIP_LEN = 15

def process(in_file, site_list, out_file, logg):
    """ This is the main driver function

    Args:
        in_file : string path to input NC file
        site_list : string path to site list
        out_file : string path to output NC file
        logg : log object

    Returns:
        ret : 0 (success) or -1 (failure)
    """
    # First, read the input forecast file
    fcst_df = read_fcst_file(in_file, logg)

    # Read the site list
    logg.write_time("Reading: %s\n" % site_list)
    site_df = pd.read_csv(site_list, dtype={"Zip Code" : object})
    # Calculate regional zip
    site_df['regional_zip'] = site_df['Zip Code'].astype(str).str[:3]
    # Combine the regional zips based on the ZIP_REPLACE_MAP
    site_df['regional_zip'].replace(ZIP_REPLACE_MAP, inplace=True)
    # Calculate total capacity per regional zip
    tot_cap_df = site_df.groupby(['regional_zip'])['Capacity_kW'].sum().reset_index()

    # Map forecasts to installations
    fcst_df = pd.merge(fcst_df, site_df[['Site','Capacity_kW','Closest_WRF_grid','Zip Code','regional_zip']], 
                       left_on=['gridId'], right_on=['Closest_WRF_grid'], how='right')
    fcst_df['power_forecast'] = fcst_df['power_percent'] * fcst_df['Capacity_kW']

    # Get forecasted power for each date and regional zip code
    logg.write_time("Calculating power forecast for each date and regional zip code\n")
    tot_fcst_df = fcst_df.groupby(['DT','regional_zip'])['power_forecast'].sum().reset_index()

    # Merge the capacity and forecast dataframe
    tot_fcst_df = pd.merge(tot_fcst_df, tot_cap_df, on=['regional_zip'])

    # Write the output file
    write_output(tot_fcst_df, out_file, logg)

    return 0


def write_output(tot_fcst_df, out_file, logg):
    """ Writes the dataframe to an NC file
    
    Args:
        tot_fcst_df : Pandas dataframe
        out_file : string path to output file
        logg : log object

    Returns:
    """
    # Sort the dataframe by site and time
    tot_fcst_df.sort_values(['regional_zip','DT'], inplace=True)

    # Create the output NC file
    logg.write_time("Writing: %s\n" % out_file)
    fid = Dataset(out_file, "w")

    times = tot_fcst_df['DT'].unique()
    sites = tot_fcst_df['regional_zip'].unique()

    num_times = len(times)
    num_sites = len(sites)
    

    # Create the dimensions
    fid.createDimension("scaler_dim", 1)
    fid.createDimension("fcst_num", num_times)
    fid.createDimension("num_zips", num_sites)
    fid.createDimension("zip_len", ZIP_LEN)

    # Create the variables
    create_time_var = fid.createVariable("creation_time", "d", ("scaler_dim"))
    create_time_var.long_name = "time at which forecast file was created"
    create_time_var.units = "seconds since 1970-1-1 00:00:00"
    create_time_var[:] = int(time.time())

    valid_time_var = fid.createVariable("valid_time", "d", ("fcst_num"))
    valid_time_var.long_name = "time at which forecast file was created"
    valid_time_var.units = "seconds since 1970-1-1 00:00:00"
    valid_time_var[:] = times.astype(np.int64) // 10**9

    zip_var = fid.createVariable("regional_zip", "c", ("num_zips", "zip_len"))
    zip_var.long_name = "Regional zip code (first 3 digits of zip code(s))"
    zip_var[:] = [stringtoarr(s, ZIP_LEN) for s in sites]
    
    cap_var = fid.createVariable("zip_capacity", "f", ("num_zips"))
    cap_var.long_name = "Total capacity of PV installations within this regional zip"
    cap_var.unites = "kW"
    cap_var[:] = tot_fcst_df['Capacity_kW'].unique()
  
    zip_fcst_var = fid.createVariable("zip_power", "f", ("num_zips", "fcst_num"))
    zip_fcst_var.long_name = "Total forecasted power"
    zip_fcst_var.units = "kW"
    zip_fcst_var[:] = tot_fcst_df['power_forecast'].values.reshape(num_sites, num_times)

    zip_pct_capacity_var = fid.createVariable("zip_pct_capacity", "f", ("num_zips", "fcst_num"))
    zip_pct_capacity_var.long_name = "Percent capacity fcst for the entire regional zip"
    zip_pct_capacity_var.units = "Percent"
    zip_pct_capacity_var[:] = (tot_fcst_df['power_forecast'] / tot_fcst_df['Capacity_kW']).values.reshape(num_sites, num_times) * 100

    fid.close()

    logg.write_time("Done\n")
    
    return
    

def read_fcst_file(in_file, logg):
    """ This function reads the input NC file

    Args:
        in_file : string path to input file
        logg : log object

    Returns:
        df : Pandas dataframe
    """
    logg.write_time("Reading: %s\n" % in_file)
    fid = Dataset(in_file, "r")
    fcst_times = fid.variables['valid_times'][:].data
    sites = fid.variables['siteId'][:]
    num_sites = len(sites)
    num_times = len(fcst_times)
    
    # Read the power forecasts
    fcsts = fid.variables['power_percent_capacity'][:] / 100
    fid.close()

    # Create lists for the dataframe
    fcst_times_arr = np.array([fcst_times] * num_sites).flatten()
    sites_arr = np.array([[s] * num_times for s in sites]).flatten()

    df = pd.DataFrame()
    df['DT'] = pd.to_datetime(fcst_times_arr, unit='s')
    df['gridId'] = sites_arr
    df['power_percent'] = fcsts.flatten()

    # Any potential QC (likely not needed but good to be safe) 
    df['power_percent'] = np.where(df['power_percent'] > 100, np.nan, df['power_percent']) 
    df['power_percent'] = np.where(df['power_percent'] < 0, np.nan, df['power_percent']) 

    return df

def main():
    usage_str = "%prog pct_power_file site_list output_file"
    usage_str += "\n pct_power_file\t path to input percent power forecast NC file"
    usage_str += "\n site_list\t path to site list file mapping installations to grid points"
    usage_str += "\n output_file\t path to output NC file to write"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
        
    (options, args) = parser.parse_args()
    
    if len(args) < 3:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  
    in_file = args[0]
    site_list = args[1]
    out_file = args[2]

    for f in [in_file, site_list]:
        if not os.path.exists(f):
            logg.write_time("Error: %s doesn't exist\n" % f)
            sys.exit(-1)

    logg.write_starting()

    ret = process(in_file, site_list, out_file, logg)

    logg.write_ending()


if __name__ == "__main__":
    main()
