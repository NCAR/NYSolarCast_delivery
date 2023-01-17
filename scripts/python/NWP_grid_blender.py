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
THIS SCRIPT WAS DESIGNED TO BLEND THE WRF AND HRRR GRIDDED FORECASTS
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
from netCDF4 import Dataset
from netCDF4 import chartostring, stringtoarr
import json
import numpy as np
import calendar
import wrf

REQUIRED_CONF_VARS = ["WRF_dir", "WRF_day_ahead_dir", "HRRR_dir", "WRF_blend_vars","HRRR_blend_vars",
                      "blending_weights", "WRF_pass_through_vars", "HRRR_pass_through_vars",
                      "HRRR_fall_back_vars"]

HRRR_DT_START = pd.to_datetime("1992-01-01")

MISSING = -9999

def process(fcst_dt, conf, out_file, RH, climate_zone_file, 
            hourly_output_file, logg):
    """
    Blends the WRFF and HRRR forecast based on config info
    
    Args:
        fcst_dt : pandas DateTime object
        conf : dictionary config file
        out_file : output NC file
        RH : Bool calculate RH or not
        climate_zone_file : Bool or path to climatezone file
        hourly_output_file : Bool or path to hourly output file to write
        logg : logg object

    Returns:

    """
    # df for holding site/lat info
    meta_df = pd.DataFrame()
    #
    # Read the WRF file
    #
    wrf_df = read_wrf_dir(fcst_dt, conf, RH, logg)
    if not wrf_df.empty:
        meta_df = wrf_df[['siteID','XLAT','XLON']].copy()
        meta_df.drop_duplicates(inplace=True)
        wrf_df.drop(columns=['XLAT','XLON'], inplace=True)
    #if wrf_df.empty:
    #    return -1
        
    #
    # Read the hrrr file
    #
    # For day-aheads, should use the 06z, for nowcasts should use 2hours prior
    if fcst_dt.hour != 6:
        # Hrrr gen_date should be 2 hours prior to the WRF gen date to account for latency
        hrrr_fcst_dt = fcst_dt - pd.Timedelta(hours=2)
    else:
        hrrr_fcst_dt = fcst_dt

    hrrr_df = read_hrrr_dir(hrrr_fcst_dt, conf, logg)

    if hrrr_df.empty:
        logg.write_time("Warning: No hrrr file to use for blending.  Using all WRF\n")
        if wrf_df.empty:
            logg.write_time("Error: No hrrr or wrf to use.  Exiting\n")
            return -1
        master_df = wrf_df        
        for v in conf['blending_weights']:
            master_df.rename(columns={v : v+"_wrf"}, inplace=True)
    else:    
        if meta_df.empty:
            meta_df = hrrr_df[['siteID','XLAT','XLON']].copy()
            meta_df.drop_duplicates(inplace=True)

        # Remove lat/lon columns (adding back in later)
        hrrr_df.drop(columns=['XLAT','XLON'], inplace=True)

        #
        # Do any linear interpolation for HRRR here
        #
        interp_vars = list(conf['blending_weights'].keys()) + conf['HRRR_pass_through_vars']
        # Add the pass through vars
        interp_vars += [v[1] for v in conf['HRRR_fall_back_vars']]
        hrrr_df = linear_interp_hrrr_adjusted(hrrr_df, interp_vars, logg)
        
        #
        # merge the dataframes on the index (time)
        #
        logg.write_time("Merging the dataframes\n")
        if wrf_df.empty:
            replace_cols = {}
            for v in interp_vars:
                replace_cols[v] = v+"_hrrr"

            master_df = hrrr_df.rename(columns=replace_cols)
        else:
            #master_df = pd.merge(wrf_df, hrrr_df, how='left', on=['siteID','ValTime'], suffixes=["_wrf", "_hrrr"])
            master_df = pd.merge(wrf_df, hrrr_df, how='outer', on=['siteID','ValTime'], suffixes=["_wrf", "_hrrr"])
        
    #
    # Blend the specific variables
    #
    master_df = blend_variables(master_df, fcst_dt, conf, logg)
    #master_df['GenTime'] = fcst_dt    
    master_df = pd.merge(master_df, meta_df, on=['siteID'])

    if climate_zone_file:
        logg.write_time("Reading: %s\n" % climate_zone_file)
        cz_df = pd.read_csv(climate_zone_file, dtype={"grid_id" : object})
        
        master_df = pd.merge(master_df, cz_df[['grid_id','climateZone']], left_on=['siteID'], 
                             right_on=['grid_id'], how='left')

        # Set any ghi value outside NYS to missing
        if 'ghi' in master_df.columns:
            master_df['ghi'] = np.where(master_df['climateZone'] == -9, np.nan, master_df['ghi'])

        master_df.drop(columns=['grid_id'], inplace=True)
        logg.write_time("Done\n")

    master_df.fillna(MISSING, inplace=True)

    # Write the final output (solar_mesh type)
    master_df.sort_values(['siteID','ValTime'], inplace=True)
    #master_df.to_csv("master_df.csv", index=False)

    ret = write_nc_file(master_df, fcst_dt, conf, out_file, RH, logg)

    # Write hourly output if necessary
    if hourly_output_file:
        hourly_df = avg_df(master_df, logg)
        hourly_df.sort_values(['siteID','ValTime'], inplace=True)
        ret = write_nc_file(hourly_df, fcst_dt, conf, hourly_output_file, RH, logg)


    return ret


def avg_df(df, logg):
    """ Returns a time-ending hourly averaged dataframe from the input dataframe

    Args:
        df : Pandas Dataframe
        logg : Log object

    Returns:
        hourly_df : Pandas DataFrame w/ hourly data
    """
    # Set index per site, genTime, ValidTime
    df.set_index(['siteID','ValTime'], inplace=True)

    # Do averaging on the ValidTime index
    level_values = df.index.get_level_values
    hourly_df = df.groupby([level_values(0)] +
                           [pd.Grouper(freq='1H', level=-1, label='right', closed='right')]).mean()

    # Reset the indexes
    hourly_df.reset_index(inplace=True)

    return hourly_df


def linear_interp_hrrr_adjusted(hrrr_df, interp_vars, logg):
    """ This function interpolates the hourly hrrr into 15min values
           while offsetting them by 7.5 minutes

    Args :
        hrrr_df : pandas dataframe w/ hourly data
        interp_vars : list of vars to interpolate
        logg : log object

    Returns:
        hrrr_df : pandas dataframe interpolated to 15min
    """
    logg.write_time("Performing interpolation of HRRR data to 15min intervals\n")

    # Add 7.5 min so that the value at 08:00 gets set to 08:07.30 (so it will interpolate better to 08:00)
    hrrr_df['ValTime'] += pd.Timedelta(minutes=7, seconds=30)

    start_dt = hrrr_df['ValTime'].min()
    end_dt = hrrr_df['ValTime'].max()
    
    # get the times for every 7.5 minutes
    reind_dts = pd.date_range(start_dt, end_dt, freq='450S')
    
    # Set the site,time to be the index
    hrrr_df.set_index(['siteID', 'ValTime'], inplace=True)
    
    # Reindex the dataframe to 15min values
    intp_df = hrrr_df.reindex(pd.MultiIndex.from_product([hrrr_df.index.levels[0],
                                                          reind_dts]))
    
    # Do the interpolation
    hrrr_df = intp_df[interp_vars].interpolate(method='linear', limit=8, limit_direction='both')
    hrrr_df.reset_index(inplace=True)
    
    # Rename the index (lost in the intepr step)
    hrrr_df.rename(columns={"level_1" : "ValTime"}, inplace=True)
    
    #hrrr_df.sort_values(['InitTime','ValTime','siteID'], inplace=True)

    # Remove values where the second != 0 (to leave only 15, 30, 45, 00)
    hrrr_df = hrrr_df[hrrr_df['ValTime'].dt.second == 0]

    logg.write_time("Done\n")
    return hrrr_df



def linear_interp_hrrr(hrrr_df, interp_vars, logg):
    """ This function interpolates the hourly hrrr into 15min values

    Args :
        hrrr_df : pandas dataframe w/ hourly data
        interp_vars : list of vars to interpolate
        logg : log object

    Returns:
        hrrr_df : pandas dataframe interpolated to 15min
    """
    logg.write_time("Performing interpolation of HRRR data to 15min intervals\n")
    fcst_df_list = []
    start_dt = hrrr_df['ValTime'].min()
    end_dt = hrrr_df['ValTime'].max()
    reind_dts = pd.date_range(start_dt, end_dt, freq='15T')

    # Set the site,time to be the index
    hrrr_df.set_index(['siteID', "ValTime"], inplace=True)    
    
    # Reindex the dataframe to 15min values
    intp_df = hrrr_df.reindex(pd.MultiIndex.from_product([hrrr_df.index.levels[0],
                                                          reind_dts]))

    # Do the interpolation 
    hrrr_df = intp_df[interp_vars].interpolate(method='linear', limit=4, limit_direction='both')
    hrrr_df.reset_index(inplace=True)

    # Rename the index (lost in the intepr step)
    hrrr_df.rename(columns={"level_1" : "ValTime"}, inplace=True)
    
    # Reset the genTime value
    #hrrr_df['GenTime'] = fcst_dt

    """
     Old technique that didn't work as quickly
    for grid_pt, grid_df in hrrr_df.groupby("siteID"):
        reind_df = grid_df.reindex(reind_dts)
        reind_df = reind_df[interp_vars].interpolate(method='linear', limit=4, limit_direction='both')
        reind_df['siteID'] = grid_pt
        reind_df['GenTime'] = fcst_dt
        reind_df['ValTime'] = reind_df.index

        fcst_df_list.append(reind_df)
    hrrr_df = pd.concat(fcst_df_list)
    """
#    hrrr_df = hrrr_df.groupby("siteID").apply(lambda group: group.reindex(reind_dts).interpolate(method='linear',
#                                                                                                 limit=4,
#                                                                                                 limit_direction='both'))

    logg.write_time("Done\n")
    return hrrr_df
        

def write_nc_file(master_df, fcst_dt, conf, out_file, RH, logg):
    """  Writes a netCDF file 
    
    Args:
        master_df : pandas DataFrame object
        fcst_dt : DateTime object
        conf : config object (dictionary)
        out_file : string path to output file
        RH : Bool output RH or not
        logg : log object

    Returns:
    
    """
    all_cz = []
    for vt, valTime_df in master_df.groupby("ValTime"):
        all_sites = valTime_df['siteID'].values
        all_lats = valTime_df['XLAT'].values
        all_lons = valTime_df['XLON'].values
        if 'climateZone' in master_df.columns:
            all_cz = valTime_df['climateZone'].values
        break

    #all_sites = list(master_df['siteID'].unique())
    num_sites = len(all_sites)
    logg.write_time("Creating: %s\n" % out_file)

    out_nc = Dataset(out_file, "w")
    out_nc.createDimension("max_site_num", num_sites)

    lead_times = list(master_df['LeadTime'].unique())
    num_fcst_times = len(lead_times)
    out_nc.createDimension("fcst_times", num_fcst_times)
    out_nc.createDimension("name_strlen", 10)

    val_times = master_df['ValTime'].unique()
    master_df['ValSecs'] = master_df['ValTime'].view('int64')//10**9
    val_secs = master_df['ValSecs'].unique()

    # Create the variables
    gen_time_var = out_nc.createVariable("gen_time", 'd')
    gen_time_var.long_name = "Generation time for blended fcst (wrf @ this time, hrrr @ this time minus 2 hours)"
    gen_time_var.units = "seconds since 1970-1-1 00:00:00"
    gen_time_var[:] = calendar.timegm(fcst_dt.timetuple())

    valid_time_var = out_nc.createVariable("valid_time", "d", ('fcst_times'))
    valid_time_var.long_name = "valid time of forecast"
    valid_time_var.units = "seconds since 1970-1-1 00:00:00"
    valid_time_var[:] = val_secs

    num_sites_var = out_nc.createVariable("num_sites", "i")
    num_sites_var.long_name = "Number of forecast sites"
    num_sites_var[:] = num_sites

    siteID_var = out_nc.createVariable("siteId", "i", "max_site_num")
    siteID_var.description = "3 digit YIndex 0 3digit XIndex. 10134 = (1,134)"
    siteID_var.long_name = "station Id"
    siteID_var.standard_name = "station_id"
    siteID_var[:] = [int(s) for s in all_sites]
    
    latitude_var = out_nc.createVariable("XLAT", "f", ("max_site_num"))
    latitude_var.long_name = "Latitude, South is negative"
    latitude_var[:] = all_lats

    longitude_var = out_nc.createVariable("XLONG", "f", ("max_site_num"))
    longitude_var.long_name = "Longitude, West is negative"
    longitude_var[:] = all_lons

    if 'climateZone' in master_df.columns:
        cz_var = out_nc.createVariable("ClimateZone", "i", ("max_site_num"), fill_value = -9)
        cz_var.long_name = "Climate zone associated with grid point"
        cz_var[:] = all_cz


    # Write the blended variables
    out_vars = list(conf['blending_weights'].keys())

    for v in out_vars:
        blnd_var = out_nc.createVariable(v, "f", ("max_site_num", "fcst_times"), fill_value = MISSING)

        ## Copy the variable attributes from the solar mesh file
        #in_v = out_nc.variables[v]
        #for attr in in_v.ncattrs():
        #    blnd_var.setncattr(attr, in_v.getncattr(attr))

        blnd_var[:] = master_df[v].values.reshape(num_sites, num_fcst_times)

    # Write the pass-through variables
    for v in conf['WRF_pass_through_vars'] + conf['HRRR_pass_through_vars']:
        pass_var = out_nc.createVariable(v, "f", ("max_site_num", "fcst_times"), fill_value = MISSING)

        ## Copy the variable attributes from the solar mesh file
        #in_v = out_nc.variables[v]
        #for attr in in_v.ncattrs():
        #    pass_var.setncattr(attr, in_v.getncattr(attr))
        v_out = 'missing'
        if v not in master_df.columns:
            # Check if it exists as a fall back var
            for var_tup in conf['HRRR_fall_back_vars']:
                if var_tup[0] == v:
                    v_out = var_tup[1]+"_hrrr"
                    logg.write_time("Couldn't find %s, falling back to HRRR %s\n" % (v, v_out))
            if v_out == 'missing':
                logg.write_error("Cant find %s in input and no fall back variable. skipping\n" % v)
        else:
            v_out = v
        
        if v_out != 'missing':
            pass_var[:] = master_df[v_out].values.reshape(num_sites, num_fcst_times)

    if RH:
        rh_var = out_nc.createVariable('RH', "f", ("max_site_num", "fcst_times"), fill_value = MISSING)

        v_out = 'missing'
        
        rh_name = 'RH'
        # If wrf data exists, this var will be called 'RH_wrf'
        if 'RH_wrf' in master_df.columns:
            rh_name = 'RH_wrf'

        if rh_name not in master_df.columns:
            # Check if it exists as a fall back var
            for var_tup in conf['HRRR_fall_back_vars']:
                if var_tup[0] == 'RH':
                    v_out = var_tup[1]+"_hrrr"
                    logg.write_time("Couldn't find 'RH', falling back to HRRR %s\n" % (v_out))
            if v_out == 'missing':
                logg.write_error("Cant find RH in input and no fall back variable. skipping\n")
        else:
            v_out = rh_name

        if v_out != 'missing':
            rh_var[:] = master_df[v_out].values.reshape(num_sites, num_fcst_times)

    out_nc.close()

    return 0

    

def blend_variables(master_df, fcst_dt, conf, logg):
    """ Blends the variables and adds them back to 'master_df'

    Args:
        master_df : pandas dataframe
        fcst_dt : DateTime object
        conf : config object (dictionary)
        logg : log object

    Returns:
        master_df : pandas dataframe after blending
    """
    blending_map = conf['blending_weights']

    # Add lead-time (Used in blending script)
    master_df['LeadTime'] = (master_df['ValTime'] - fcst_dt).view(np.int64) // 10**9


    # Blend vars
    for var in blending_map:
        logg.write_time("Blending: %s\n" % var)
        wrf_var = "%s_wrf" % var
        hrrr_var = "%s_hrrr" % var                

        # Set data to hrrr var if wrf var doesn't exist.
        if wrf_var not in master_df:
            if hrrr_var not in master_df:
                logg.write_time("Error: Didn't find data for %s in either wrf or hrrr file\n" % hrrr_var)
                continue
            else:
                logg.write_time("Warning: Didn't find data for %s\n" % wrf_var)
                master_df[var] = master_df[hrrr_var]
                continue
        elif hrrr_var not in master_df:
            master_df[var] = master_df[wrf_var]
            continue

        weights_df = pd.DataFrame(blending_map[var], columns=['LeadTime','wrf_weight','hrrr_weight'])
        
        master_df = pd.merge(master_df, weights_df, on=['LeadTime'])                
        
        master_df[var] = ((master_df[wrf_var] * master_df['wrf_weight']) + 
                          (master_df[hrrr_var] * master_df['hrrr_weight']))

        # Remove the added weight columns
        master_df.drop(columns=['wrf_weight', 'hrrr_weight'], inplace=True)

        # Set the blend value to the hrrr val wherever wrf is missing
        master_df[var] = np.where(pd.isnull(master_df[wrf_var]), 
                                  master_df[hrrr_var], master_df[var])
        
        # Set the blend value to the wrf wherever hrrr is missing
        master_df[var] = np.where(pd.isnull(master_df[hrrr_var]), 
                                  master_df[wrf_var], master_df[var])


    return master_df



def read_hrrr_dir(fcst_dt, conf, logg):
    """ Reads a hrrr file
    
    Args:
        fcst_dt : DateTime object for the fcst gen time to read
        conf : config object (dictionary)
        logg : log object

    Returns:
        df : pandas dataframe containing hrrr data
    """
    file_path = "%s/%s/hrrr_ghi.%s" % (conf['HRRR_dir'], fcst_dt.strftime("%Y%m%d"), fcst_dt.strftime("%Y%m%d.i%H%M"))
    if not os.path.exists(file_path):
        logg.write_time("Warning: %s doesn't exist, looking for file from previous hour\n" % file_path)
        file_path = "%s/%s/hrrr_ghi.%s" % (conf['HRRR_dir'], fcst_dt.strftime("%Y%m%d"), (fcst_dt-pd.Timedelta(hours=1)).strftime("%Y%m%d.i%H%M"))
        if not os.path.exists(file_path):
            logg.write_time("Warning: %s doesn't exist, ignoring hrrr data\n" % file_path)
            return pd.DataFrame()
    
    logg.write_time("Reading: %s\n" % file_path)
    fid = Dataset(file_path, "r")

    xlats = fid['lat'][:]
    xlons = fid['lon'][:]

    ref_hours = fid['reftime'][:]
    val_hours = fid['valtime'][:]

    num_sites = len(fid.dimensions['max_site_num'])
    num_times = len(val_hours)

    # Format the site list so that it is x_ind0y_ind (ie. 2630260, or 0000001)
    site_list = list(fid['site_list'][:])
    for x in range(0, len(site_list)):
        site_list[x] = str(site_list[x]).zfill(7)

    df = pd.DataFrame()

    # Read the variable data from the dataframe
    for v in conf['HRRR_blend_vars'].keys():
        df[v] = fid[v][:].flatten()
    for v in conf['HRRR_pass_through_vars']:
        df[v] = fid[v][:].flatten()
    for v in conf['HRRR_fall_back_vars']:
        df[v[1]] = fid[v[1]][:].flatten()

    fid.close()

    # Set the lat/lon points in the dataframe
    df['XLAT'] = list(xlats) * num_times
    df['XLON'] = list(xlons) * num_times
    df['siteID'] = list(site_list) * num_times
    
    #df['GenTime'] = fcst_dt
    
    val_times = [HRRR_DT_START + pd.Timedelta(hours=v) for v in val_hours]
    df['ValTime'] = np.array([[v]*num_sites for v in val_times]).flatten()

    # Rename the blend variables
    df.rename(columns=conf['HRRR_blend_vars'], inplace=True)

    logg.write_time("Done\n")
    return df
     

def get_lead_times(conf):
    """ Gets all of the lead times from the config file (From any listed var)

    Args:
        conf : config object (dictionary)

    Returns:
        lead_times : list of lead-times
    """
    blend_vars = conf['blending_weights'].keys()
    
    lead_times = []
    for v in blend_vars:
        t_lead_times = [x[0] for x in conf['blending_weights'][v]]
        for t in t_lead_times:
            if t not in lead_times:
                lead_times.append(t)

    return lead_times
        

def read_wrf_dir(fcst_dt, conf, RH, logg):
    """  Reads the WRF grid NC file into a dataframe

    Args:
        fcst_dt : DateTime object
        conf : config object (dictionary)
        RH : Bool calculate RH or not
        logg : log object

    Returns:        
        df : Pandas Dataframe containing WRF data
    """
    wrf_base_dir = "%s/%s/wrfout/" % (conf['WRF_dir'], fcst_dt.strftime("%Y-%m-%d_%H"))

    # Read a separate file for each lead-time
    lead_times = get_lead_times(conf)
    nowcast_lead_times = lead_times[:24]
    dayahead_lead_times = lead_times

    df_list = []
    for lt in nowcast_lead_times:
        file_dt = fcst_dt + pd.Timedelta(seconds=lt)
        
        fcst_file_path = "%s/wrfout_d01_%s" % (wrf_base_dir, file_dt.strftime("%Y-%m-%d_%H:%M:%S"))

        if not os.path.exists(fcst_file_path):
            logg.write_time("Warning: %s doesn't exist\n" % fcst_file_path)
            continue
        if os.path.getsize(fcst_file_path) < 4080:
            continue

        # Read the wrf nc file
        this_df = read_wrf_file(fcst_file_path, file_dt, conf, RH, logg)
        if this_df.empty:
            continue

        df_list.append(this_df)
        
    if len(df_list) == 0:
        logg.write_time("No wrf nowcast data, looking for recent day-ahead fcst\n")
        # Need to get offset for the recent day-ahead fcst
        day_ahead_dt = fcst_dt.replace(hour=6)
        wrf_base_dir = "%s/%s/wrfout/" % (conf['WRF_day_ahead_dir'], day_ahead_dt.strftime("%Y-%m-%d_%H"))
        lt_offset = (fcst_dt.hour - 6) * 3600
        for lt in dayahead_lead_times:
            day_ahead_lt = day_ahead_dt + pd.Timedelta(seconds=lt+lt_offset)
            #logg.write_time("Wrf day-ahead dt: %s\n" % day_ahead_lt.strftime("%Y%m%d.%H%M"))
            
            fcst_file_path = "%s/wrfout_d01_%s" % (wrf_base_dir, day_ahead_lt.strftime("%Y-%m-%d_%H:%M:%S"))
            if not os.path.exists(fcst_file_path):
                logg.write_time("Warning: %s doesn't exist\n" % fcst_file_path)
                continue
            
            # Read the wrf nc file
            this_df = read_wrf_file(fcst_file_path, day_ahead_lt, conf, RH, logg)
            if this_df.empty:
                continue

            df_list.append(this_df)
                                             

    # If no WRF's / return nothing
    if len(df_list) == 0:
        return pd.DataFrame()
    df = pd.concat(df_list)
    #df['GenTime'] = fcst_dt

    # Rename the blending vars
    df.rename(columns=conf['WRF_blend_vars'], inplace=True)
    logg.write_time("Done\n")

    return (df)
    
def read_wrf_file(in_file, file_dt, conf, RH, logg):
    """ Reads the wrf input file

    Args:
        in_file : string path to input file
        file_dt : DateTime object for the file date
        conf : config object
        RH : Bool calculate RH or not
        logg : log object

    Returns:
        df : pandas dataframe object
    """
    # Variable for filling in input data
    data = {}

    # Read the netCDF file
    logg.write_time("Reading: %s\n" % in_file)
    fid = Dataset(in_file, "r")
    
    num_times = len(fid.dimensions['Time'])
    if num_times == 0:
        logg.write_time("Error: Incomplete wrf file.  Skipping\n")
        fid.close()
        return pd.DataFrame()

    xlats = fid["XLAT"][0].flatten()
    xlons = fid["XLONG"][0].flatten()
    
    num_x = len(fid.dimensions['south_north'])
    num_y = len(fid.dimensions['west_east'])
    
    # Create the 'siteID' for matching with HRRR 
    site_ids = []
    for x in range(0, num_x):
        for y in range(0, num_y):
            site_ids.append("%03d0%03d" % (x, y))
        
    # Get the blend variables
    for v in conf['WRF_blend_vars'].keys():
        data[v] = fid[v][0].flatten()
        
    # Get the pass through variables
    for v in conf['WRF_pass_through_vars']:
        data[v] = fid[v][0].flatten()
       
    rh_data = np.array([])
    if RH:
        rh_data = wrf.getvar(fid, 'rh2', squeeze=False)

    fid.close()
 
    # Create the dataframe
    this_df = pd.DataFrame()
    this_df['XLAT'] = xlats
    this_df['XLON'] = xlons
    this_df['siteID'] = site_ids
    for v in data.keys():
        this_df[v] = data[v]
    this_df['ValTime'] = file_dt
    
    if RH:
        this_df['RH'] = rh_data.values.flatten()

    return this_df
    
def read_verify_config(config_file, logg):
    """ Makes sure the config file has all expected attributes
    
    Args:
        config_file : string path to config file
        logg : log object

    Returns:
        conf : config object (dictionary) 
    """
    if not os.path.exists(config_file):
        logg.write_time("Error: %s doesn't exist\n" % config_file)
        return -1

    logg.write_time("Reading: %s\n" % config_file)
    json_file = open(config_file)
    json_str = json_file.read()
    conf = json.loads(json_str)   

    # Verify that all required variables exist
    for v in REQUIRED_CONF_VARS:
        if v not in conf:
            logg.write_time("Error: %s doesn't exist in config file.  Exiting\n" % v)
            return -1
    
    # Make sure all of the weights add up to 1.0
    for var_obj in conf['blending_weights']:
        for arr in conf['blending_weights'][var_obj]:
            if arr[1] + arr[2] != 1.0:
                logg.write_time("Error: Weights (lead-time: %d) don't add up to 1.0\n" % arr[0])
                return -1

    # Make sure the output blend vars are listed in blending weights
    if len(conf['WRF_blend_vars']) != len(conf['HRRR_blend_vars']):
        logg.write_time("Error: length of 'WRF_blend_vars'(%d) must equal length of 'HRRR_blend_vars'(%d)\n" %
                        (len(conf['WRF_blend_vars']), len(conf['HRRR_blend_vars'])))
        return -1
    for d in conf['WRF_blend_vars']:
        if conf['WRF_blend_vars'][d] not in conf['blending_weights']:
            logg.write_time("Error: Variable %s not listed in the blending weights dictionary\n" % conf['WRF_blend_vars'][d])
            return -1
    for d in conf['HRRR_blend_vars']:
        if conf['HRRR_blend_vars'][d] not in conf['blending_weights']:
            logg.write_time("Error: Variable %s not listed in the blending weights dictionary\n" % conf['HRRR_blend_vars'][d])
            return -1

    return conf



def main():
    usage_str = "%prog forecast_gen_date config_file output_file"
    usage_str += "\n   forecast_gen_date\t YYYYmmdd.HHMM"
    usage_str += "\n   config_file\t json config file"
    usage_str += "\n   output_file\t output netCDF file"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
    parser.add_option("-r", "--RH", dest="RH", action="store_true", help="Calculate RH from the wrf input and pass it through")
    parser.add_option("-z", "--climate_zone_file", dest="climate_zone_file", help="Write Climate zone of each grid point to the output file (from this input file)")
    parser.add_option("-H", "--hourly_output_file", dest="hourly_output_file", help="Write time-ending hourly averaged data to this output file")
    
        
    (options, args) = parser.parse_args()
    
    if len(args) < 3:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  
    forecast_date = args[0]
    config_file = args[1]
    out_file = args[2]

    # Verify the forecast date argument
    try:
        fcst_dt = pd.to_datetime(forecast_date, format='%Y%m%d.%H%M')
    except:
        logg.write_time("Error: %s doesn't match format %Y%m%d.%H%M\n" % forecast_date)

    # Verify the config file
    conf = read_verify_config(config_file, logg)
    if conf == -1:
        sys.exit(-1)

    logg.write_starting()

    ret = process(fcst_dt, conf, out_file, options.RH, options.climate_zone_file,
                  options.hourly_output_file, logg)

    logg.write_ending(ret)


if __name__ == "__main__":
    main()
