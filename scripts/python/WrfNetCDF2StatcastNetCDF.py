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
THIS SCRIPT IS USED TO CONVERT WRF NC FILES TO A STATCAST-LIKE FORMAT FOR STATCAST
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
from netCDF4 import Dataset, chartostring, stringtoarr
import numpy as np
from pvlib import solarposition

def process(wrf_dir, fcst_init_date, site_list, variables, out_file, day_ahead, logg):
    """
    Args:
        wrf_dir : string path to wrf directory
        fcst_init_date : pandas datetime
        site_list : string path to site list file
        variables : list of variables to output
        out_file : string path to output NC file
        day_ahead : False or path to day-ahead dir
        logg : log object

    Returns:
    """
    # Read the site list
    logg.write_time("Reading: %s\n" % site_list)
    site_df = pd.read_csv(site_list)
    site_df.rename(columns={'stid' : 'MesoSite',
                            'lat [degrees]' : 'MESO_LAT',
                            'lon [degrees]' : 'MESO_LON'}, inplace=True)
    
    # Read the wrf data
    (wrf_df, attr_map) = read_wrf_data(wrf_dir, site_df, fcst_init_date, variables, logg)
    if (wrf_df.empty):
        if day_ahead:
            logg.write_time("Looking for WRF data in day-ahead area\n")
            (wrf_df, attr_map) = read_wrf_day_ahead_data(day_ahead, site_df, fcst_init_date, variables, logg)
            
    if (wrf_df.empty):
        logg.write_time("No WRF data, exiting\n")
        sys.exit(-1)

    # Add some derived fields (wind speed/Dir, TOA, Solar angles)
    wrf_df['TOA'] = wrf_df['SWDOWN']/wrf_df['CLRNIDX']
    wrf_df['TOA'] = np.where(wrf_df['CLRNIDX'] == 0, -9999, wrf_df['TOA'])
    attr_map['TOA'] = {"description" : "Top of Atmospher Irradiance (SWDOWN/CLRNIDX)",
                       "units" : "W m-2",
                       "_fillValue" : -9999.}

    wrf_df['WSPD10'] = np.sqrt(wrf_df['U10']**2 + wrf_df['V10']**2)
    attr_map['WSPD10'] = {"description" : "Wind Speed calculated from U10/V10",
                          "units" : "m s-1"}
    wrf_df['WDIR10'] = (180/np.pi) * (np.arctan2(wrf_df['U10'],wrf_df['V10'])) + 180
    attr_map['WDIR10'] = {"description" : "Wind Dir calculated from U10/V10",
                          "units" : "degrees"}
    # Set dir to missing anywhere speed is 0
    wrf_df['WDIR10'] = np.where(wrf_df['WSPD10']==0, np.nan, wrf_df['WDIR10'])


    # Add solar angles at 1min intervals then merge back into the dataframe
    reind_dts = pd.date_range(wrf_df['ValidTime'].min() - pd.Timedelta(minutes=15),
                              wrf_df['ValidTime'].max(), freq='1T')

    # Reindex to 1min
    wrf_df.set_index(["MesoSite", "ValidTime"], inplace=True)
    min_df = wrf_df.reindex(pd.MultiIndex.from_product([wrf_df.index.levels[0], reind_dts],
                                                       names=['MesoSite','ValidTime']), method='bfill').reset_index()[['MesoSite','ValidTime','MESO_LAT','MESO_LON']]

    # Add solar angles
    logg.write_time("Adding solar angles at 1min intervals\n")
    df_list = []
    for site, t_site_df in min_df.groupby('MesoSite'):
        site_solar_df = solarposition.get_solarposition(t_site_df['ValidTime'], t_site_df['MESO_LAT'],
                                                        t_site_df['MESO_LON'])
        site_solar_df['MesoSite'] = site

        df_list.append(site_solar_df)

    solar_df = pd.concat(df_list)
    logg.write_time("Done\n")

    solar_df['custom_TOA'] = calculate_toa(solar_df)
    # Add TOA using our custom calculation
    attr_map['custom_TOA'] = {"descripion" : "TOA using custom calculation",
                              "units" : "W m-2",
                              "_fillValue" : -9999.}
    
    # Calculate 15-min time-ending avg of solar angles
    avg_solar_df = solar_df.groupby('MesoSite').resample('15T', label='right',closed='right').mean().reset_index()
    # Merge solar angle info back into the dataframe
    wrf_df = pd.merge(wrf_df, avg_solar_df[['ValidTime','MesoSite','apparent_elevation','azimuth',
                                            'custom_TOA']],
                      on=['ValidTime','MesoSite'])
    attr_map['apparent_elevation'] = {"description" : "Apparent solar elevation at meso site",
                                      "units" : "degrees"}
    attr_map['azimuth'] = {"description" : "Azimuth at meso site",
                           "units" : "degrees"}
    wrf_df['custom_KT'] = wrf_df['SWDOWN']/wrf_df['custom_TOA']
    wrf_df['custom_KT'] = np.where(wrf_df['SWDOWN']==0, -9999, wrf_df['custom_KT'])
    attr_map['custom_KT'] = {"description" : "KT calculated using SWDOWN/custom_TOA",
                             "units" : "percent",
                             "_fillValue" : -9999}
    

    # Write the output nc
    write_nc(wrf_df, attr_map, out_file, logg)

    return

def calculate_toa(wrf_df):
    """ calculates TOA using custom calculation
    
    Args:
        wrf_df : Pandas dataframe with datetime index
    """
    #doy = wrf_df['ValidTime'].dt.dayofyear
    doy = wrf_df.index.dayofyear

    # Calculation from Sue D (20201005)
    b = 2 * np.pi * doy / 365

    # Degrees to radians
    elev_rad = wrf_df['apparent_elevation'] * np.pi / 180

    ghiToa = 1367 * (1.00011 + .034221 * np.cos(b) + .00128 * np.sin(b) + .000719*np.cos(2*b) + .000077*np.sin(2*b)) * np.sin(elev_rad)

    ghiToa = np.where(ghiToa < 0, 0, ghiToa)

    return ghiToa


def write_nc(wrf_df, attr_map, out_file, logg):
    """ Writes a statcast-like NC file

    Args:
        wrf_df : Pandas dataframe
        attr_map : dictionary mapping variable to attributes
        out_file : path to output file to create
        logg : log object
    """

    logg.write_time("Creating: %s\n" % out_file)
    out_nc = Dataset(out_file, "w")    

    num_sites = len(wrf_df['MesoSite'].unique())
    num_fcsts = len(wrf_df['ValidTime'].unique())

    # Get list of site/lat/lon
    site_list = []
    int_sites = []
    wlat_list = []
    wlon_list = []
    mlat_list = []
    mlon_list = []
    for val_t, val_t_df in wrf_df.groupby("ValidTime"):
        site_list = val_t_df['MesoSite'].tolist()
        int_sites = val_t_df['int_id'].tolist()
        wlat_list = val_t_df['WRF_LAT'].tolist()
        wlon_list = val_t_df['WRF_LON'].tolist()
        mlat_list = val_t_df['MESO_LAT'].tolist()
        mlon_list = val_t_df['MESO_LON'].tolist()
        break

    if len(wlat_list) != num_sites:
        logg.write_time("Warning: Mismatch in number of datapoints for %s and number of sites(%d)\n" % (val_t, num_sites))
        sys.exit()

    # Create the dimensions
    out_nc.createDimension("max_site_num", num_sites)
    out_nc.createDimension("fcst_times", num_fcsts)
    out_nc.createDimension("name_strlen", 10)

    # Create the variables:
    create_time_var = out_nc.createVariable("creation_time", "d")
    create_time_var.long_name = "time at which forecast file was created"
    create_time_var.units = "seconds since 1970-1-1 00:00:00"
    create_time_var[:] = int(time.time())

    valid_time_var = out_nc.createVariable("valid_time", "d", ("fcst_times"))
    valid_time_var.long_name = "valid time of forecast"
    valid_time_var.units = "seconds since 1970-1-1 00:00:00"
    valid_time_var.missing_value = -999.9
    valid_times = wrf_df['ValidTime'].unique()
    valid_time_var[:] = valid_times.astype(np.int64) // 10**9
    
    num_sites_var = out_nc.createVariable("num_sites", "i")
    num_sites_var.long_name = "Number of forecast sites"
    num_sites_var[:] = num_sites

    wlon_var = out_nc.createVariable('wrf_lon', 'f', ('max_site_num'))
    wlon_var.long_name = 'longitude associated with closest wrf grid point'
    wlon_var.units = 'degrees_east'
    wlon_var[:] = wlon_list

    wlat_var = out_nc.createVariable('wrf_lat', 'f', ('max_site_num'))
    wlat_var.long_name = 'latitude associated with closest wrf grid point'
    wlat_var.units = 'degrees_north'
    wlat_var[:] = wlat_list

    mlon_var = out_nc.createVariable('site_lon', 'f', ('max_site_num'))
    mlon_var.long_name = 'longitude associated with mesonet site'
    mlon_var.units = 'degrees_east'
    mlon_var[:] = mlon_list

    mlat_var = out_nc.createVariable('site_lat', 'f', ('max_site_num'))
    mlat_var.long_name = 'latitude associated with mesonet site'
    mlat_var.units = 'degrees_north'
    mlat_var[:] = mlat_list

    station_name_var = out_nc.createVariable('StationName', 'c', ('max_site_num', 'name_strlen'))
    station_name_var.long_name = 'Mesonet Station Name'
    station_name_var.standard_name = 'stid'
    station_name_var[:] = [stringtoarr(s, 10) for s in site_list]

    station_id_var = out_nc.createVariable('StationID', 'i', ('max_site_num'))
    station_id_var.long_name = 'Mesonet integer id'
    station_id_var.standard_name = 'int_id'
    station_id_var[:] = int_sites

    # Create all of the variables from the input list
    for v in attr_map:
        if "_fillValue" in attr_map[v]:
            this_var = out_nc.createVariable(v, 'f', ('max_site_num','fcst_times'),
                                             fill_value=attr_map[v]['_fillValue'])
        else:
            this_var = out_nc.createVariable(v, 'f', ('max_site_num','fcst_times'))
        for att in attr_map[v]:
            if att == '_fillValue':
                continue
            this_var.setncattr(att, attr_map[v][att])
        this_var[:] = wrf_df[v].values.reshape(num_sites, num_fcsts)

    out_nc.close()
    return


def read_wrf_data(wrf_dir, site_df, fcst_init_date, variables, logg):
    """ Reads the wrf forecast data

    Args:
        wrf_dir : string path to wrf directory
        site_df : Pandas dataframe with site info
        fcst_init_date : pandas datetime
        variables : list of variables to output
        logg : log object  
    
    Returns:
        wrf_df : Pandas dataframe w/ wrf data
        attr_map : Mapping of variable to attributes
    """
    # Get the grid points to read the from the NC file
    meso_sites = []
    grid_xs = []
    grid_ys = []
    for ind, row in site_df.iterrows():
        parts = row['Closest_WRF'].split('_')
        grid_xs.append(int(parts[0][3:]))
        grid_ys.append(int(parts[1]))
        meso_sites.append(row['MesoSite'])

    # Get the input
    wrf_dt_dir = "%s/%s/wrfout" % (wrf_dir, fcst_init_date.strftime("%Y-%m-%d_%H"))
    if not os.path.exists(wrf_dt_dir):
        logg.write_time("Error: %s doesn't exist.\n" % wrf_dt_dir)
        return (pd.DataFrame(), {})
        #sys.exit()
    
    wrf_df_list = []
    # Start reading the wrf files at lead-time = 1 (lt = 0 has missing GHI fields)
    for dt in pd.date_range(fcst_init_date+pd.Timedelta(minutes=15),
                            fcst_init_date+pd.Timedelta(hours=6), freq='15T'):
        wrf_nc_file = os.path.join(wrf_dt_dir, "wrfout_d01_%s" % dt.strftime("%Y-%m-%d_%H:%M:%S"))
        if not os.path.exists(wrf_nc_file):
            logg.write_time("Warning: %s doesn't exist.\n" % wrf_nc_file)
            continue
        (t_wrf_df, attr_map) = read_nc_file(wrf_nc_file, meso_sites, grid_xs, grid_ys, variables, logg)
        wrf_df_list.append(t_wrf_df)
    
    if len(wrf_df_list) == 0:
        logg.write_time("Error: No wrf files to ingest\n")
        return (pd.DataFrame(), {})
        #sys.exit(-1)

    wrf_df = pd.concat(wrf_df_list)
    wrf_df['InitTime'] = fcst_init_date    
    wrf_df.sort_values(['MesoSite','ValidTime'], inplace=True)

    # Add wrf lat/lon to the dataframe
    wrf_df = pd.merge(wrf_df, site_df[['MesoSite','int_id','MESO_LAT','MESO_LON','WRF_LAT','WRF_LON']], on=['MesoSite'])

    return (wrf_df, attr_map)

def read_wrf_day_ahead_data(wrf_dir, site_df, fcst_init_date, variables, logg):
    """ Reads the wrf day ahead forecast data

    Args:
        wrf_dir : string path to wrf directory
        site_df : Pandas dataframe with site info
        fcst_init_date : pandas datetime
        variables : list of variables to output
        logg : log object  
    
    Returns:
        wrf_df : Pandas dataframe w/ wrf data
        attr_map : Mapping of variable to attributes
    """
    # Get the grid points to read the from the NC file
    meso_sites = []
    grid_xs = []
    grid_ys = []
    for ind, row in site_df.iterrows():
        parts = row['Closest_WRF'].split('_')
        grid_xs.append(int(parts[0][3:]))
        grid_ys.append(int(parts[1]))
        meso_sites.append(row['MesoSite'])

    # Get the input
    wrf_dt_dir = "%s/%s_06/wrfout" % (wrf_dir, fcst_init_date.strftime("%Y-%m-%d"))
    if not os.path.exists(wrf_dt_dir):
        logg.write_time("Error: %s doesn't exist.\n" % wrf_dt_dir)
        return (pd.DataFrame(), {})
        #sys.exit()
    
    wrf_df_list = []
    # Start reading the wrf files at lead-time = 1 (lt = 0 has missing GHI fields)
    for dt in pd.date_range(fcst_init_date+pd.Timedelta(minutes=15),
                            fcst_init_date+pd.Timedelta(hours=6), freq='15T'):
        wrf_nc_file = os.path.join(wrf_dt_dir, "wrfout_d01_%s" % dt.strftime("%Y-%m-%d_%H:%M:%S"))
        if not os.path.exists(wrf_nc_file):
            logg.write_time("Warning: %s doesn't exist.\n" % wrf_nc_file)
            continue
        (t_wrf_df, attr_map) = read_nc_file(wrf_nc_file, meso_sites, grid_xs, grid_ys, variables, logg)
        wrf_df_list.append(t_wrf_df)
    
    if len(wrf_df_list) == 0:
        logg.write_time("Error: No wrf files to ingest\n")
        return (pd.DataFrame(), {})
        #sys.exit(-1)

    wrf_df = pd.concat(wrf_df_list)
    wrf_df['InitTime'] = fcst_init_date    
    wrf_df.sort_values(['MesoSite','ValidTime'], inplace=True)

    # Add wrf lat/lon to the dataframe
    wrf_df = pd.merge(wrf_df, site_df[['MesoSite','int_id','MESO_LAT','MESO_LON','WRF_LAT','WRF_LON']], on=['MesoSite'])

    return (wrf_df, attr_map)

def read_nc_file(file_path, meso_sites,  grid_xs, grid_ys, variables, logg):
    """ Reads a statcast-like nc file
    Assumes the time field is 'forc_time'

    Args:
        file_path : Path to input wrf NC file
        meso_sites : list of mesonet sites
        grid_xs : list of x(lat) points from the grid to read
        grid_ys : list of y(lon) points from the grid to read
        variables : list of variables to read
        logg : log object

    Returns:
        this_df : pandas dataframe
        var_attr_map : dict mapping variable to attributes
    """
    this_df = pd.DataFrame()

    # Get the file size to make sure it is read-able
    f_size = os.path.getsize(file_path)
    if f_size < 4000:
        logg.write_time("Warning: %s is incomplete.  Ignoring\n" % file_path)
        return this_df

    logg.write_time("Reading: %s\n" % file_path)
    nc_in = Dataset(file_path)

    # Get the times
    times = nc_in.variables['Times'][:]
    if len(times) != 1:
        logg.write_time("Error: Num of times in %s != 1\n" % file_path)
        return this_df

    var_attr_map = {}
    for v in variables:
        var_data = nc_in.variables[v][:]

        # Read the data at each grid point all at once
        this_df[v] = var_data[0,grid_xs, grid_ys]

        var_attr_map[v] = {}
        for attr in nc_in.variables[v].ncattrs():
            var_attr_map[v][attr] = getattr(nc_in.variables[v], attr)

    nc_in.close()

    dt_str = chartostring(times[0])
    val_date = pd.to_datetime(dt_str, format="%Y-%m-%d_%H:%M:%S", utc=True)

    this_df['MesoSite'] = meso_sites
    this_df['ValidTime'] = val_date

    return (this_df, var_attr_map)


def main():
    usage_str = "%prog wrf_dir fcst_init_date site_list variables output_file"
    usage_str += "\n wrf_dir\t path to input directory containing wrf files"
    usage_str += "\n fcst_init_date\t YYYYmmdd.hhmm corresponding to init time"
    usage_str += "\n site_list\t Path to csv file containing list of sites to use"
    usage_str += "\n variables\t Comma separated list of variables to use"
    usage_str += "\n output_file\t path to output NC file"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
    parser.add_option("-d", "--day_ahead", dest="day_ahead", help="Path to day-ahead dir to fall back on if the nowcast isn't available")
        
    (options, args) = parser.parse_args()
    
    if len(args) < 5:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  
    wrf_dir = args[0]
    fcst_init_date = pd.to_datetime(args[1], format="%Y%m%d.%H%M")
    site_list = args[2]
    variables = args[3].split(',')
    out_file = args[4]

    if not os.path.exists(wrf_dir):
        logg.write_time("Error: %s doesn't exist.  Exiting\n" % wrf_dir)
        sys.exit()
    if not os.path.exists(site_list):
        logg.write_time("Error: %s doesn't exist.  Exiting\n" % site_list)
        sys.exit()
    
    logg.write_starting()

    process(wrf_dir, fcst_init_date, site_list, variables, out_file, options.day_ahead, logg)

    logg.write_ending()


if __name__ == "__main__":
    main()
