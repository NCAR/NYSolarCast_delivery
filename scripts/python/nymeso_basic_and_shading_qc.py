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
This script is used to perform the shading QC on the New York Mesonet data
    It also adds solar angles and TOA values at 1-min intervals
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
import numpy as np
from pvlib import solarposition

ghi_field = 'solar_insolation [watt/m**2]'

def process(in_file, shading_dir, site_list, out_file, logg):
    """ This is the main driver function, performs basic QC

    Args:
        in_file : string path to input_file
        shading_dir : string path to directory containing shading information
        site_list : string path to site list file
        out_file : string path to output file
        logg : log object

    Returns: 
    """
    # Read the obs file
    logg.write_time("Reading: %s\n" % in_file)
    df = pd.read_csv(in_file)
    df['DateTime'] = pd.to_datetime(df['datetime'], format="%Y%m%dT%H%M%S")

    # Read the shading info
    shading_df = read_shading_dir(shading_dir, logg)

    # Do the qc
    qc_df = perform_shading_qc(df, shading_df, logg)

    # Do quick bounds check on ghi  (0-1300)
    qc_df[ghi_field] = np.where(qc_df[ghi_field]<0, np.nan, qc_df[ghi_field])
    qc_df[ghi_field] = np.where(qc_df[ghi_field]>1300, np.nan, qc_df[ghi_field])        

    #
    # Add der obs fields
    #
    # First, reindex dataframe to 1min values
    end_dt = qc_df['DateTime'].max()
    start_dt = end_dt - pd.Timedelta(minutes=4)
    reind_dts = pd.date_range(start_dt, end_dt, freq='1T')
    qc_df.set_index(['station','DateTime'], inplace=True)
    min_df = qc_df.reindex(pd.MultiIndex.from_product([qc_df.index.levels[0], reind_dts],
                                                      names=['station','DateTime'])).reset_index()
    # Add lat/lon values
    site_df = pd.read_csv(site_list, usecols=['stid','lat [degrees]', 'lon [degrees]', 'int_id'])
    site_df.rename(columns={'stid' : 'station'}, inplace=True)
    min_df = pd.merge(min_df, site_df, on=['station'])
    
    # Add solar angles
    logg.write_time("Adding solar angles\n")
    df_list = []
    for site, site_df in min_df.groupby('station'):
        site_solar_df = solarposition.get_solarposition(site_df['DateTime'], site_df['lat [degrees]'],
                                                        site_df['lon [degrees]'])
        site_solar_df['station'] = site

        df_list.append(site_solar_df)

    solar_df = pd.concat(df_list)
    logg.write_time("Done\n")

    # calculate TOA for each minute
    logg.write_time("Adding TOA\n")
    solar_df['TOA'] = calculate_toa(solar_df)
    min_df.drop(columns=['lat [degrees]', 'lon [degrees]'], inplace=True)
    
    # Merge the solar dataframe back with the qc'd data
    qc_df = pd.merge(min_df, solar_df[['station','azimuth','apparent_elevation','TOA']],
                     on=['DateTime','station'])
    
    # Write the output
    logg.write_time("Writing: %s\n" % out_file)
    qc_df.to_csv(out_file, index=False)
    
    return


def calculate_toa(der_obs_df):
    """ Calculates TOA

    Args:
        der_obs_df : pandas dataframe

    Returns:
    """
    #doy = der_obs_df['DateTime'].dt.dayofyear
    doy = der_obs_df.index.dayofyear

    # Calculation from Sue D (20201005)
    b = 2 * np.pi * doy / 365

    # Degrees to radians
    elev_rad = der_obs_df['apparent_elevation'] * np.pi / 180

    ghiToa = 1367 * (1.00011 + .034221 * np.cos(b) + .00128 * np.sin(b) + .000719*np.cos(2*b) + .000077*np.sin(2*b)) * np.sin(elev_rad)

    ghiToa = np.where(ghiToa < 0, 0, ghiToa)

    return ghiToa


def perform_shading_qc(df, shading_df, logg):
    """ Removes values in 'df' that are impacting by shading.  Requires 'DateTime' and 'MesoSite' columns

    Args:
        df : pandas dataframe with 'DateTime' datetime object
        shading_df : Pandas Dataframe containing shading information
        logg : logg object

    Returns:
        qc_df : Pandas dataframe with shaded values set to np.nan            
    """
    # Add a julianDay column to the df for merging
    df['Jday'] = df['DateTime'].dt.dayofyear
    df['hour_decimal'] = df['DateTime'].dt.hour + (df['DateTime'].dt.minute/60)

    df = pd.merge(df, shading_df, on=['Jday', 'station'], how='left')

    # Replace the values before sunrise_shaded or after sunset_shaded with nan
    df.loc[df['hour_decimal'] <= df['sunrise_shaded'], ghi_field] = np.nan
    df.loc[df['hour_decimal'] >= df['sunset_shaded'], ghi_field] = np.nan    

    # Clean up the dataframe
    df.drop(columns=['Jday','hour_decimal','sunrise_shaded','sunset_shaded'], inplace=True)
    return df
    

def read_shading_dir(shading_dir, logg):
    """ Reads multiple shading files and creates a pandas dataframe

    Args:
        shading_dir : string path to directory containing shading files
        logg : log object

    Returns:
        qc_df : pandas dataframe with 'Jday' and 'MesoSite' along with hours that are impacted by shading
    """
    sunrise_file = os.path.join(shading_dir, 'Sunrise.UTCs.17sites.csv')
    sunrise_flag_file = os.path.join(shading_dir, 'Srad.Flag.mins.sunrise.a10m.20200804-17sites.csv')
    sunset_file = os.path.join(shading_dir, 'Sunset.UTCs.17sites.csv')
    sunset_flag_file = os.path.join(shading_dir, 'Srad.Flag.mins.sunset.a10m.20200804-17sites.csv')

    logg.write_time("Reading shading files\n")
    sunrise_df = pd.read_csv(sunrise_file)
    sunset_df = pd.read_csv(sunset_file)
    sunrise_flag_df = pd.read_csv(sunrise_flag_file)
    sunset_flag_df = pd.read_csv(sunset_flag_file)

    sunrise_qc_df = pd.merge(sunrise_df, sunrise_flag_df, on=['Jday'],
                             suffixes=['_sunrise', '_mins_after'])
    sunset_qc_df = pd.merge(sunset_df, sunset_flag_df, on=['Jday'],
                             suffixes=['_sunset', '_mins_before'])

    # Get the sites
    shading_sites = []        
    for col in sunrise_df:
        if col != 'Jday':
            shading_sites.append(col)
    for col in sunset_df:
        if col != 'Jday':
            if col not in shading_sites:
                logg.write_time("Warning: Site %s in sunset df but not sunrise df\n" % col)
    
        
    # Calculate the hour when to qc data
    for col in sunrise_df:
        if col == 'Jday':
            continue
        sunrise_qc_df[col+"_sunrise_shaded"] = sunrise_qc_df[col+"_sunrise"] + (sunrise_qc_df[col+"_mins_after"]/60)
    for col in sunset_df:
        if col == 'Jday':
            continue        
        sunset_qc_df[col+"_sunset_shaded"] = sunset_qc_df[col+"_sunset"] - (sunset_qc_df[col+"_mins_before"]/60)
            
    # Create the dataframe containing jdays and sites
    logg.write_time("Creating shading df\n") 
    jdays = sunrise_qc_df['Jday'].append(sunset_qc_df['Jday']).unique().tolist()
    jday_list = jdays * len(shading_sites)
    sites_list = np.array([[s] * len(jdays) for s in shading_sites]).flatten()
    qc_df = pd.DataFrame()

    qc_df['Jday'] = jday_list
    qc_df['station'] = sites_list

    sunrise_shade = []
    sunset_shade = []
    sunrise_qc_df.set_index('Jday', inplace=True)
    sunset_qc_df.set_index('Jday', inplace=True)
    for ind, row in qc_df.iterrows():
        site = row['station']
        sunrise_shade.append(sunrise_qc_df.loc[row['Jday']]['%s_sunrise_shaded' % site])
        sunset_shade.append(sunset_qc_df.loc[row['Jday']]['%s_sunset_shaded' % site])

    qc_df['sunrise_shaded'] = sunrise_shade
    qc_df['sunset_shaded'] = sunset_shade
    logg.write_time("Done\n")
    
    return qc_df

    
def main():
    usage_str = "%prog input_obs_file shading_dir site_list output_obs_file"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
        
    (options, args) = parser.parse_args()
    
    if len(args) < 4:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  
    in_file = args[0]
    shading_dir = args[1]
    site_list = args[2]
    out_file = args[3]

    logg.write_starting()

    process(in_file, shading_dir, site_list, out_file, logg)

    logg.write_ending()


if __name__ == "__main__":
    main()
