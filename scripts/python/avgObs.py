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


import time, os, sys, math
from datetime import datetime, timedelta
from optparse import OptionParser
import log_msg
import json
import pandas as pd
import numpy as np

CONFIG_FIELDS = ["date_field", "date_format", "avg_interval",
                 "avg_type", "ignore_fields", "pass_through_fields", "site_field"]

OPTIONAL_CONF_FIELDS = ["avg_bounds", "wind_dir_avg"]

U_FIELD = "__U"
V_FIELD = "__V"

#
# avg_bounds : specifies how many seconds before and after the time-center to use for averaging
#

def process(in_files, config_file, out_file, options, logg):
    """ The main driver function 

    Args:
        in_files : list of input files
        config_file : string path to config file
        out_file : string path to output file
        options : options object
        logg : log object

    Returns:
        ret : int exit status
    """
    # First, read the config file info
    conf = read_verify_config(config_file, logg)
    if conf == -1:
        logg.write_time("Error: Reading config file.  Exiting\n")
        return -1
        
    # Read the input files into a dataframe
    for in_f in in_files:
        if not os.path.exists(in_f):
            logg.write_time("Error: '%s' doesn't exist.  Exiting\n" % in_f)
            return -1
    logg.write_time("Reading input files\n")
    df = pd.concat([pd.read_csv(f) for f in in_files])

    # Remove any columns listed in config file
    if len(conf['ignore_fields']) != 0:
        df.drop(conf['ignore_fields'], axis=1, inplace=True)

    # Convert the time field to a datetime and set it as the index
    ret = set_datetime_index(df, conf, logg)
    if ret != 0:
        return ret

    # If datetime passed, remove times more than 'avg_interval' before datetime
    if options.datetime:
        # Only set up for time_ending avg right now.  May need to update more for other data types 
        t_dt = pd.to_datetime(options.datetime, format="%Y%m%d.%H%M")
        lim_secs = conf['avg_interval']
        lim_dt = t_dt - pd.Timedelta(seconds=lim_secs)
        logg.write_time("Limiting data to %s - %s\n" % (lim_dt.strftime("%Y%m%d.%H%M"), t_dt.strftime("%Y%m%d.%H%M")))
        df = df[df.index > lim_dt]
        df = df[df.index <= t_dt]

    # Handle a site field
    if conf['site_field'] != 'None':
        site_field = conf['site_field']
        all_sites = df[site_field].unique().tolist()
        df_list = []

        # Do the remaining steps on a per-site basis
        for s in all_sites:
            # Get a dataframe for a single site
            site_df = df[df[site_field]==s].copy()

            # Process this specific site
            avg_site_df = do_df_processing(site_df, conf, logg)
            
            # Add the site back
            avg_site_df[site_field] = s
            df_list.append(avg_site_df)

        avg_df = pd.concat(df_list)
    else:
        # Pull out the 'pass_through_fields' and add back in after averaging is done
        avg_df = do_df_processing(df, conf, logg)

    # Custom for epri
    if 'int_id' in avg_df.columns:
        avg_df['int_id'] = avg_df['int_id'].astype(np.int)
        

    if options.Kt:
        avg_df['Kt'] = avg_df['solar_insolation [watt/m**2]'] / avg_df['TOA']
        # Set Kt to missing anywhere TOA = 0
        avg_df['Kt'] = np.where(avg_df['TOA']<=0, np.nan, avg_df['Kt'])
        # Set KT to 1 anywhere it's greater than 1
        avg_df['Kt'] = np.where(avg_df['Kt']>1, 1, avg_df['Kt'])

        
    logg.write_time("Writing: %s\n" % out_file)
    avg_df.to_csv(out_file, date_format = '%Y-%m-%d %H:%M:%S', float_format = "%.3f")

    return 0

def average_dataframe(df, conf, logg):
    """ This function does the averaging

    Args:
        df : pandas dataframe
        conf : dictionary containing config information
        logg : log object

    Returns:
        avg_df : Pandas dataframe containing averaged data
    """
    avg_str = '%dS' % conf['avg_interval']

    # First, need to make sure the dataframe start/end times are correct
    if conf['avg_type'] == 'time_centered':
        # Time-Centered.  
        
        (first_avg_dt, last_dt) = get_df_time_range(df, conf)

        rows = []
        # Loop through all times between start_ind and last index            
        for avg_dt in pd.date_range(first_avg_dt, last_dt, freq=avg_str):            
            # Get the starting and ending time for this avg interval
            avg_start_dt = avg_dt - pd.Timedelta(seconds=start_int)
            avg_end_dt = avg_dt + pd.Timedelta(seconds=end_int)

            ## If the end period is more than interval/2 after the end of the dataframe, ignore
            #if (avg_end_dt-last_dt).total_seconds() >= (conf['avg_interval']/2):
            #    # Data ends in first half of the average period, skip it
            #    continue

            # If the end avg interval is after the end of the dataframe, skip it
            if avg_end_dt > last_dt:
                continue

            # Average all of the data between avg_start_dt and avg_end_dt
            this_row = df[(df.index>=avg_start_dt) & (df.index<=avg_end_dt)].mean()

            # Set the 'DATETIME' to be the avg center time
            this_row['DATETIME'] = avg_dt

            # Add this average onto the rows list to be combined into the last dataframe
            rows.append(pd.DataFrame([this_row]))
        if len(rows) == 0:
            logg.write_time("Warning: No data found in date range\n")
            return pd.DataFrame()

        avg_df = pd.concat(rows)
        avg_df['DATETIME'] = pd.to_datetime(avg_df['DATETIME'] // 10**9, unit='s')
        avg_df.set_index('DATETIME', inplace=True)


    elif conf['avg_type'] == 'time_ending':
        # Time-ending
        avg_df = df.resample(avg_str, label='right', closed='right').mean()
        count_df = df.resample(avg_str, label='right', closed='right').count()

        # Remove times when less than 2 entries made it into the average
        inval = count_df[count_df[conf['date_field']]<2]
        avg_df.drop(inval.index, inplace=True)
                    
    else:
        logg.write_time("Need to add code to handle average type: %s\n" % conf['avg_type'])
        return pd.DataFrame()

    # Reset the time field
    if conf['date_format'] == 'epoch':
        avg_df[conf['date_field']] = avg_df.index.astype(np.int64) // 10**9
    else:
        avg_df[conf['date_field']] = avg_df.index.astype(np.int64) // 10**9
#    else:
#        logg.write_time("Not sure how to recompute date_type of '%s' after avg is done.  Add code\n" % conf['date_format'])
#        sys.exit()

    return avg_df


def get_df_time_range(df, conf):
    """ For time-centered averages, gets the start and end date

    Args:
        df : pandas dataframe with DateTimeIndex 
        conf : Dictionary containing config information

    Returns:
        first_avg_dt : datetime corresponding to start of average interval
        last_dt : datetime corresponding to end of average interval
    """
    start_int = conf['avg_interval']/2
    end_int = conf['avg_interval']/2
    if 'avg_bounds' in conf:
        (start_int, end_int) = conf['avg_bounds']
        
    # Get the first time in the file
    start_dt = df.index[0]
    start_epoch = start_dt.value // 10**9

    # Find the time associated with first average period
    first_avg_time = start_epoch - (start_epoch%conf['avg_interval']) + conf['avg_interval']
    first_avg_dt = pd.to_datetime(first_avg_time, unit='s')
    
    # Get the first time that should be included in this average
    avg_start_int = first_avg_time - start_int
    start_dt = pd.to_datetime(avg_start_int, unit='s')
    
    # Get the last time in the dataframe
    last_dt = df.index[-1]

    return (first_avg_dt, last_dt)


def set_datetime_index(df, conf, logg):
    """ Sets the date field as the dataframe(df) index

    Args:
        df : Pandas dataframe
        conf : dictionary containing config information

    Returns:
       ret : int exit status
    """
    # Check to make sure the date column exists
    if conf['date_field'] not in df:
        logg.write_time("Error: date_field: '%s' not in input files\n" % conf['date_field'])
        return -1

    # Set the date field as the index
    if conf['date_format'] == 'epoch':        
        df['DATETIME'] = pd.to_datetime(df[conf['date_field']], unit='s')
    else:
        try:
            df['DATETIME'] = pd.to_datetime(df[conf['date_field']], format=conf['date_format'])
        except:
            logg.write_time("Error: Not sure yet how to handle date_format: %s\n" % conf['date_format'])
            return -1

    df.set_index('DATETIME', inplace=True)
    df.sort_index(inplace=True)
    return 0

def read_verify_config(config_file, logg):
    """ Makes sure the config file has no unexpected fields, and make sure the essential fields exist

    Args:
        config_file : string path to config file
        logg : log object

    Returns:
        config_info : -1 if failure, else dictionary containing configuration information 
    """
    if not os.path.exists(config_file):
        logg.write_time("Error: %s doesn't exist\n" % config_file)
        return -1

    logg.write_time("Reading: %s\n" % config_file)
    json_file = open(config_file)
    json_str = json_file.read()
    conf = json.loads(json_str)

    if 'Info' not in conf:
        logg.write_time("Error: No 'Info' listed in your config file.  Exiting\n")
        return -1
    conf_info = conf['Info']
    # Make sure expecting keywords exist
    for key in conf_info:
        if (key not in CONFIG_FIELDS) and (key not in OPTIONAL_CONF_FIELDS):
            logg.write_time("Warning: '%s' isn't recognized as a config field.  Ignoring\n" % key)
            
    for key in CONFIG_FIELDS:
        if key not in conf_info:
            logg.write_time("Error: '%s' isn't listed in your config file.\n" % key)
            return -1

    return conf_info

def do_df_processing(df, conf, logg):
    """ Calls the functions to do the averaging

    Args:
        df : pandas DataFrame with DateTimeIndex
        conf : dictionary containing configuration information
        logg : log object

    Returns:
        avg_df : -1 if failure, else dataframe containing averaged data
    """
    # Pull out the 'pass_through_fields' and add back in after averaging is done
    #pass_df = df[[conf['date_field']] + conf['pass_through_fields']]
    pass_df = df[conf['pass_through_fields']]    
    df.drop(conf['pass_through_fields'], axis=1, inplace=True)

    # If any wind-direction processing needs to be done --
    #  create the corresponding U and V fields so that they can be averaged
    #
    if "wind_dir_avg" in conf:
        wind_dir_fields = []
        wind_speed_fields = []
        for dct in conf['wind_dir_avg']:
            out_u_field = dct['WindDirColumn'] + U_FIELD
            out_v_field = dct['WindDirColumn'] + V_FIELD
            # Add the u and v fields for this column to the dataframe
            df[out_u_field] = df.apply(calculate_U, args=(dct['WindSpeedColumn'], dct['WindDirColumn']), axis=1)
            df[out_v_field] = df.apply(calculate_V, args=(dct['WindSpeedColumn'], dct['WindDirColumn']), axis=1)

    # Average the dataframe
    #logg.write_time("Calculating averages\n")
    avg_df = average_dataframe(df, conf, logg)
    if avg_df.empty:
        return -1

    #
    # If wind dir processing took place.  Pull the wind direction back out of the U and V components
    #
    if "wind_dir_avg" in conf:
        for dct in conf['wind_dir_avg']:
            u_name = dct['WindDirColumn'] + U_FIELD
            v_name = dct['WindDirColumn'] + V_FIELD
            avg_df[dct['WindDirColumn']] = avg_df.apply(calculate_dir, args=([u_name, v_name]),
                                                        axis=1)
            # Remove the U and V components
            avg_df.drop([u_name, v_name], axis=1, inplace=True)

    # Add the pass_through_fields back in and add the index back on at the end
    #avg_df = avg_df.reset_index().merge(pass_df, on=conf['date_field'], how='left').set_index("DATETIME")
    avg_df = avg_df.merge(pass_df, left_index=True, right_index=True, how='left')
    # Remove any duplicate rows
    avg_df.drop_duplicates(inplace=True)

    return avg_df

def calculate_dir(row, u, v):
    """ Calculates direction from u and v components
    
    Args:
        row : Pandas series
        u : string name corresponding to U field
        v : string name corresponding to V field

    Returns:
        value : float direction value   
    """
    return (180/math.pi) * (math.atan2(row[u], row[v])) + 180

def calculate_U(row, strSpd, strDir):
    """ Calculates U component from speed and direction
        
    Args:
        row : Pandas series
        strSpd : string name corresponding to speed field
        strDir : string name corresponding to direction field

    Returns:
        value : float U component of wind
    """
    return -row[strSpd]*math.sin(2*math.pi*row[strDir]/360)

def calculate_V(row, strSpd, strDir):
    """ Calculates V component from speed and direction
        
    Args:
        row : Pandas series
        strSpd : string name corresponding to speed field
        strDir : string name corresponding to direction field

    Returns:
        value : float V component of wind
    """
    return -row[strSpd]*math.cos(2*math.pi*row[strDir]/360)

def main():
    usage_str = "%prog in_file(s) config_file output_file"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
    parser.add_option("-k", "--Kt", dest="Kt", action="store_true", help="Add a Kt column from hardcoded input vars (for epri solar)")
    parser.add_option("-d", "--datetime", dest="datetime", help="Limit output to just this datetime (YYYYmmdd.HHMM)")
            
    (options, args) = parser.parse_args()
    
    if len(args) < 3:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  
    in_files = args[0].split(",")
    config_file = args[1]
    out_file = args[2]

    logg.write_starting()

    ret = process(in_files, config_file, out_file, options, logg)

    logg.write_ending(ret)

    sys.exit(ret)

if __name__ == "__main__":
    main()
