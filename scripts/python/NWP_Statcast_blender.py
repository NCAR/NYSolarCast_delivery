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
THIS SCRIPT COMBINES THE NWP GRIDDED MODEL WITH A POINT BASED STATCAST MODEL
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import json
from netCDF4 import Dataset, chartostring, stringtoarr
import pandas as pd
import numpy as np
import calendar
from scipy.ndimage import gaussian_filter
import math


MISSING = -9999

def process(nwp_file, statcast_file, site_list, grid_site_map, conf, fcst_dt, output_file, 
            climate_zone_file, hourly_output_file, logg):
    """ This function does the main processing for this script

    Args:
        nwp_file : string path to input NWP forecast file
        statcast_file : string path to input statcast forecast file
        site_list : string path to file containing mapping from int_id to stationID
        grid_site_map : string path to gridPT -> Obs site map
        conf : config object (dictionary)
        output_file : string path to output NC file
        fcst_dt : Datetime for the gen date
        climate_zone_file : Bool or path to climatezone file
        hourly_output_file : Bool or path to hourly output file to write
        logg : log object

    Returns:
        ret : integer exit status (0 = success)
    """    
    # Read the grid_site_map file
    grid_to_point_map = read_grid_site_map(grid_site_map, logg)

    # Read the input NWP file
    (nwp_df, grid_site_df) = read_nwp_file(nwp_file, logg)

    # Read the statcast forecast file
    statcast_df = read_statcast_file(statcast_file, logg)

    # Subset the nwp_df to only times that correspond to statcast times
    #     statcast is every 15min, NWP is every hour, so statcast will start later than NWP at times (ie. 18:45 blend w/ 18:00)
    nwp_df = nwp_df[nwp_df['ValidTime']>=statcast_df['ValidTime'].min()]

    logg.write_time("Reindexing nwp dataframe\n")
    # Reindex nwp_df to have same validTimes as the statcast df (missing if not available)
    statcast_valid_times = statcast_df['ValidTime'].unique()
    nwp_sites = nwp_df['GridSiteList'].unique()
    nwp_df.set_index(['GridSiteList','ValidTime'], inplace=True)
    nwp_df = nwp_df.reindex(pd.MultiIndex.from_product([nwp_sites,statcast_valid_times],
                                                       names=['GridSiteList','ValidTime'])).reset_index()
    
    # Get the names of the statcast sites from the site list
    logg.write_time("Reading: %s\n" % site_list)
    site_df = pd.read_csv(site_list)
    site_df.rename(columns={'stid' : 'StationID'}, inplace=True)
    statcast_df = pd.merge(statcast_df, site_df[['int_id','StationID']], on='int_id', how='left')
    
    # Make sure the correct valid times are in the config and input files
    if not verify_lead_secs(conf, nwp_df, statcast_df, logg):
        return -1

    # Blend the forecast data based on lead-time and spatial distance
    blended_df = grid_point_blend(nwp_df, statcast_df, conf, grid_to_point_map, logg)

    blended_df.sort_values(['GridSiteList','ValidTime'], inplace=True)    
    grid_site_df.sort_values('GridSiteList', inplace=True)

    # Limit the dataframe to only times after the forecast date
    blended_df = blended_df[blended_df['ValidTime'] > fcst_dt]
    blended_df['GenTime'] = fcst_dt

    # Apply a gaussian filter to the data
    #   Removed for now to keep mae low. 
    #num_times = len(blended_df['ValidTime'].unique())
    #blended_df.sort_values(['ValidTime','GridSiteList'], inplace=True)
    #blended_df['ghi'] = gaussian_filter(blended_df['ghi'].values.reshape(num_times, 264, 264),
    #                                    sigma=2).flatten()

    if climate_zone_file:
        logg.write_time("Reading: %s\n" % climate_zone_file)
        cz_df = pd.read_csv(climate_zone_file)       

        grid_site_df = pd.merge(grid_site_df, cz_df[['grid_id','climateZone']], left_on=['GridSiteList'], 
                                right_on=['grid_id'], how='left')

        # Set any ghi value outside NYS to missing
        blended_df = pd.merge(blended_df, grid_site_df[['GridSiteList','climateZone']], on='GridSiteList')
        blended_df['ghi'] = np.where(blended_df['climateZone'] == -9, np.nan, blended_df['ghi'])
        
        grid_site_df.drop(columns=['grid_id'], inplace=True)
        logg.write_time("Done\n")


    # Write the output
    blended_df.fillna(MISSING, inplace=True)
    ret = write_output(blended_df, grid_site_df, output_file, logg) 

    # Write hourly output if necessary
    if hourly_output_file:
        hourly_df = avg_df(blended_df, logg)
        hourly_df.sort_values(['GridSiteList','ValidTime'], inplace=True)
        ret = write_output(hourly_df, grid_site_df, hourly_output_file, logg)

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
    df.set_index(['GridSiteList','GenTime','ValidTime'], inplace=True)

    # Do averaging on the ValidTime index
    level_values = df.index.get_level_values
    hourly_df = df.groupby([level_values(i) for i in [0,1]] +
                        [pd.Grouper(freq='1H', level=-1, label='right', closed='right')]).mean()

    # Reset the indexes
    hourly_df.reset_index(inplace=True)

    return hourly_df


def write_output(blended_df, grid_site_df, output_file, logg):
    """ Writes the output NC file

    Args:
        blended_df : Pandas Dataframe containing data
        grid_site_df : Pandas Dataframe containing only gridSite,lat,lon 
        output_file : Path to output NC file
        logg : Log object

    Returns:
        Exit status : 0=Success, -1=Failure
    """
    all_sites = grid_site_df['GridSiteList'].values.tolist()
    all_lats = grid_site_df['XLAT'].values
    all_lons = grid_site_df['XLONG'].values
    num_sites = len(all_sites)
    valid_times = list(blended_df['ValidTime'].unique())
    num_fcst_times = len(valid_times)

    logg.write_time("Writing: %s\n" % output_file)
    out_nc = Dataset(output_file, "w")
    out_nc.createDimension("max_site_num", num_sites)
    out_nc.createDimension("fcst_times", num_fcst_times)
    out_nc.createDimension("name_strlen", 10)
        
    val_secs = [v.astype('int64') //10**9 for v in valid_times]
    fcst_dt = blended_df.iloc[0]['GenTime']

    # Create the variables
    gen_time_var = out_nc.createVariable("gen_time", 'd')
    gen_time_var.long_name = "Generation time of fcst"
    gen_time_var.units = "seconds since 1970-1-1 00:00:00"
    gen_time_var[:] = calendar.timegm(fcst_dt.timetuple())

    valid_time_var = out_nc.createVariable("valid_time", "d", ('fcst_times'))
    valid_time_var.long_name = "valid time of forecast"
    valid_time_var.units = "seconds since 1970-1-1 00:00:00"
    valid_time_var[:] = val_secs

    num_sites_var = out_nc.createVariable("num_sites", "i")
    num_sites_var.long_name = "Number of forecast sites"
    num_sites_var[:] = num_sites

    #station_name_var = out_nc.createVariable("StationName", "c", ("max_site_num","name_strlen"))
    #station_name_var.long_name = "station name"
    #station_name_var.standard_name = "station_id"
    #station_name_var[:] = [stringtoarr(s.item(), 10) for s in all_sites]

    siteId_var = out_nc.createVariable("siteId", "i", ("max_site_num"))
    siteId_var.description = "3 digit YIndex 0 3digit XIndex. 10134 = (1,134)"
    siteId_var.long_name = "station Id"
    siteId_var.standard_name = "station_id"
    #siteId_var[:] = [int(s) for s in all_sites]
    siteId_var[:] = all_sites


    latitude_var = out_nc.createVariable("XLAT", "f", ("max_site_num"))
    latitude_var.long_name = "Latitude, South is negative"
    latitude_var[:] = all_lats

    longitude_var = out_nc.createVariable("XLONG", "f", ("max_site_num"))
    longitude_var.long_name = "Longitude, West is negative"
    longitude_var[:] = all_lons

    if 'climateZone' in grid_site_df.columns:
        cz_var = out_nc.createVariable("ClimateZone", "i", ("max_site_num"), fill_value = -9)
        cz_var.long_name = "Climate zone associated with grid point"
        cz_var[:] = grid_site_df['climateZone'].values

    blend_var = out_nc.createVariable("ghi", "f", ("max_site_num", "fcst_times"), fill_value = MISSING)
    blend_var.long_name = "Global Horizontal Irradiance"
    blend_var.units = 'W/m^2'
    blend_var[:] = blended_df['ghi'].values.reshape(num_sites, num_fcst_times)

    # Output any other vars which were passed through
    for v in blended_df.columns:
        if v not in ['GridSiteList','ValidTime','ghi','GenTime','climateZone']:
            out_var = out_nc.createVariable(v, "f", ("max_site_num", "fcst_times"), fill_value = MISSING)
            if v == 'T2':
                out_var.long_name = "WRF Temp (2m)"
                out_var.units = "K"
            elif v == 'RH':
                out_var.long_name = "WRF Relative humidity"
                out_var.units = "percent"
            out_var[:] = blended_df[v].values.reshape(num_sites, num_fcst_times)

    out_nc.close()
    return 0
    

def verify_lead_secs(conf, nwp_df, statcast_df, logg):
    """ Makes sure the lead times match between config, NWP and Statcast

    Args:
        conf : config object
        nwp_df : pandas Dataframe
        statcast_df : pandas Dataframe
        logg : log object

    Returns:
        bool : True = success, False = failure
    """
    nwp_valids = nwp_df['ValidTime'].unique()
    #nwp_gen = nwp_df.iloc[0]['GenTime']
    statcast_valids = statcast_df['ValidTime'].unique()
    statcast_gen = statcast_df.iloc[0]['GenTime']

    # NWP lead-times are shifted to align w/ statcast lead times (statcast 18:45 lead 1 should align w/ NWP 18:00 lead 4)
    nwp_lead_secs = [(v-statcast_gen).total_seconds() for v in nwp_valids]
    statcast_lead_secs = [(v-statcast_gen).total_seconds() for v in statcast_valids]
    conf_leads = list(conf['lead_time_blending_weights'].keys())

    for x in range(0, len(conf_leads)):
        if (int(conf_leads[x]) != statcast_lead_secs[x]):
            logg.write_time("Error: Config lead-time(%d) doesn't match statcast lead-time(%d)\n" % (int(conf_leads[x]),
                                                                                                    statcast_lead_secs[x]))
            return False
        if (int(conf_leads[x]) != nwp_lead_secs[x]):
            logg.write_time("Error: Config lead-time(%d) doesn't match nwp lead-time(%d)\n" % (int(conf_leads[x]),
                                                                                               nwp_lead_secs[x]))
            return False
    return True


def grid_point_blend(nwp_df, statcast_df, conf, grid_to_point_map, logg):
    """ Performs the temporal and spatial blend of the NWP and Statcast forecasts

    Args:
        nwp_df : pandas dataframe containing NWP forecast data
        statcast_df : pandas dataframe containing Statcast forecast data
        conf : conf object (dictionary)
        grid_to_point_map : dictionary mapping grid points to nearby sites
        logg: log object

    Returns:
        nwp_df : pandas dataframe with blended forecast values (where applicable)
    """
    logg.write_time("Performing temporal and spatial blending\n")
    # Get a dataframe with only sites that need to be modified
    mod_grid_sites = grid_to_point_map.keys()    
    mod_grid_df = nwp_df[nwp_df['GridSiteList'].isin(mod_grid_sites)]
    keep_grid_df = nwp_df[~nwp_df['GridSiteList'].isin(mod_grid_sites)]

    # Set the statcast site and time as the index for quick lookup
    statcast_df.set_index(["StationID",'ValidTime'], inplace=True)

    sbw = conf['spatial_blending_weights']
    slope_list = []
    for x in range(1, len(sbw)):
        lower = sbw[x-1]
        higher = sbw[x]
        slope_list.append((higher[1] - lower[1])/(higher[0] - lower[0]))



    # Get weight based on lead-time (W3)
    statcast_lt_weights = np.array([w[1] for w in conf['lead_time_blending_weights'].values()])

    blnd_df_list = [keep_grid_df]

    for grid_site, nwp_site_df in mod_grid_df.groupby("GridSiteList"):
        site_str = "%07d" % grid_site
        blend_info = grid_to_point_map[site_str]

        # Could subset statcast df here to only sites included in the blend (may save time?)
        # Do the blending for this grid point
        blended_df = blend_site_df(nwp_site_df, statcast_df, blend_info, statcast_lt_weights,
                                   sbw, slope_list, logg)
        blnd_df_list.append(blended_df)

    # Recreate the full dataframe
    full_grid_df = pd.concat(blnd_df_list)
    
    logg.write_time("Done\n")

    return full_grid_df

    

def blend_site_df(nwp_site_df, statcast_df, blend_info, statcast_lt_weights, sbw, slope_list, logg):
    """ Blends two dataframes based on lead-time and distance

    Args:
        nwp_site_df : pandas dataframe with 1 NWP grid site
        statcast_df : pandas dataframe containing Statcast forecast data
        blend_info : dictionary containing nearby sites and distance
        statcast_lt_weights : np array of weights per lead-time 
        sbw : list of [[dist, weight],[dist, weight]]
        slope_list : list with slopes between the weights/distances
        logg : Log object
    
    Returns: 
        blended_df : pandas dataframe containing blended forecast
    """
    statcast_sites = blend_info['ObsSites']
    statcast_dists = blend_info['ObsDistance']
    #
    # First determine the statcast value
    #
    if len(statcast_sites) == 1:
        # If only one nearby statcast site, use that value
        statcast_site_df = statcast_df.loc[statcast_sites[0]]
    else:
        # Get the weights to apply to each site based on distance to the grid point
        spatial_weights = get_spatial_weights(np.array(statcast_dists))

        # Need to blend the statcast sites based on distance to grid point
        statcast_site_df = statcast_df.loc[statcast_sites[0]].copy()
        statcast_site_df['ghi'] *= spatial_weights[0]
        for x in range(1, len(spatial_weights)):
            statcast_site_df['ghi'] += statcast_df.loc[statcast_sites[x]]['ghi'] * spatial_weights[x]


    # Determine the slope for this distance
    this_dist = statcast_dists[0]
    for x in range(0, len(sbw)-1):
        if this_dist >= sbw[x][0] and this_dist <= sbw[x+1][0]:
            slope = slope_list[x]
            dist_lower, weight_lower = sbw[x]
            break

    #if weight_lower == -1:
    #    logg.write_time("Error: Distance to site (%f) is outside of the spatial_blending_weights object\n" % this_dist)
    #    sys.exit()


    # Get weight for statcast value based on distance to grid point (W1)
    statcast_spatial_weight = (weight_lower + (this_dist - dist_lower) * slope)
    #logg.write_time("Spatial weight to apply to nearby statcast site: %f\n" % statcast_spatial_weight)

    #nwp_lt_weights = np.array([w[0] for w in conf['lead_time_blending_weights'].values()])

    statcast_weights = (statcast_lt_weights * statcast_spatial_weight)
    nwp_weights = (1-statcast_weights)

    blended_df = nwp_site_df.copy()

    #if len(statcast_sites) > 3:
    #    print (nwp_site_df.iloc[0]['GridSiteList'])
    #    print (statcast_lt_weights)
    #    print (statcast_spatial_weight)
    #    print (statcast_weights)
    #    print (nwp_weights)        
    #    sys.exit()

    blended_df['ghi'] = np.array(blended_df['ghi']*nwp_weights) + np.array(statcast_site_df['ghi']*statcast_weights)

    # Set blended ghi value to the statcast value wherever the grid value is missing
    blended_df['ghi'] = np.where(pd.isnull(nwp_site_df['ghi']), statcast_site_df['ghi'], blended_df['ghi'])

    return blended_df

   
def get_spatial_weights(distances):
    """ Returns the weights to use for each distance
    
    Args:
        distances : Numpy array containing distances to sites

    Returns:
        weights : Numpy array containing weights
    """
    # Divide each distance by the minimum (sets min distance = 1)
    arr = np.array(distances / np.amin(distances))

    # Divide each weight by the max value(1)
    arr1 = 1/arr

    # Make sure all weights add up to 1
    weights = arr1/np.sum(arr1)

    return weights
    
def read_statcast_file(statcast_file, logg):
    """ Reads the specific statcast file

    Args:
        statcast_file : string path to input statcast NC file
        logg : logg object

    Returns:
        statcast_df : Pandas DataFrame
    """
    logg.write_time("Reading: %s\n" % statcast_file)
    fid = Dataset(statcast_file, "r")

        # Read the times array (seconds)
    valid_times = fid.variables['valid_times'][:]
    valid_dts = pd.to_datetime(valid_times, unit='s')

    # Read the sites
    sites = fid.variables['siteId'][:]

    # Create the sites and valid times array
    sites_arr = np.array([[s] * len(valid_dts) for s in sites]).flatten()
    valid_dts_arr = np.array([valid_dts] * len(sites)).flatten()

    # Create the dataframe
    fcst_df = pd.DataFrame()
    fcst_df['int_id'] = sites_arr
    fcst_df['ValidTime'] = valid_dts_arr
    
    fcst_df['ghi'] = fid.variables['GHI'][:].flatten()

    # Do basic sanity check on statcast GHI value
    fcst_df['ghi'] = np.where(fcst_df['ghi']>2000, np.nan, fcst_df['ghi'])
    fcst_df['ghi'] = np.where(fcst_df['ghi']<0, np.nan, fcst_df['ghi'])

    # Get GenTime from the input file name
    gen_time_str = statcast_file[-18:-3]
    fcst_df['GenTime'] = pd.to_datetime(gen_time_str, format="%Y%m%d.%H%M%S")
    return (fcst_df)

def read_nwp_file(nwp_file, logg):
    """ Reads the NWP file
    
    Args:
        nwp_file : string path to input NWP file
        logg : log object
    
    Returns:
        nwp_df : pandas dataframe containing NWP forecast data
        site_df : Pandas dataframe containing only site,lat,lon for the NWP grid
    """
    logg.write_time("Reading: %s\n" % nwp_file)
    fid = Dataset(nwp_file)

    # Read the input variables
    gen_time = fid.variables['gen_time'][0]
    valid_times = list(fid.variables['valid_time'][:])
    #sites = fid.variables['StationName'][:]
    #site_list = [int(chartostring(s)) for s in sites]
    site_list = fid.variables['siteId'][:]
    lats = fid.variables['XLAT'][:]
    lons = fid.variables['XLONG'][:]
    
    ghi = fid.variables['ghi'][:]

    # Pass any other variables into the output file
    extra_var_map = {}
    for v in fid.variables:
        if v not in ['gen_time','valid_time','num_sites','siteId','XLAT','XLONG','ghi']:
            extra_var_map[v] = fid.variables[v][:]
    fid.close()

    # Build up the dataframe
    site_list_array = np.array([[s]*len(valid_times) for s in site_list]).flatten()
    valid_time_array = valid_times * len(site_list)

    nwp_df = pd.DataFrame()
    nwp_df['GridSiteList'] = site_list_array
    nwp_df['ValidTime'] = pd.to_datetime(valid_time_array, unit='s')
    nwp_df['ghi'] = ghi.flatten()
    nwp_df['GenTime'] = pd.to_datetime(gen_time, unit='s')
    for v in extra_var_map.keys():
        nwp_df[v] = extra_var_map[v].flatten()

    site_df = pd.DataFrame()
    site_df['GridSiteList'] = site_list
    site_df['XLAT'] = lats
    site_df['XLONG'] = lons

    return (nwp_df, site_df)

def read_grid_site_map(grid_site_map, logg):
    """ Reads the grid site map
    
    Args:
        grid_site_map : Path to json file
        logg : lob object

    Returns:
        grid_to_point_map : Dictionary mapping grid point to nearby obs site
    """
    logg.write_time("Reading: %s\n" % grid_site_map)
    json_file = open(grid_site_map)
    json_str = json_file.read()
    grid_to_point_map = json.loads(json_str)
    
    return grid_to_point_map

def read_verify_config(config_file, logg):
    """ Reads the config file and makes sure necessary variables exist

    Args:
        config_file : Path to json config file
        logg : log object

    Returns:
        conf : python dictionary or -1 if failure
    """
    logg.write_time("Reading: %s\n" % config_file)
    json_file = open(config_file)
    json_str = json_file.read()
    conf = json.loads(json_str)

    for v in ['lead_time_blending_weights', 'spatial_blending_weights']:
        if v not in conf:
            logg.write_time("Error: '%s' not in the config file\n" % v)
            return -1

    lt_weights = conf['lead_time_blending_weights']
    for lt in lt_weights:
        if len(lt_weights[lt]) != 2:
            logg.write_time("Error:  Must specify two weights(found %d) for lead-time: %d\n" % (len(lt_weights[lt]), lt))
            return -1
        if (lt_weights[lt][0] + lt_weights[lt][1]) != 1.0:
            logg.write_time("Error: Weights for lead-time: %d must add up to 1 (not %f)\n" % (lt, (lt_weights[lt][0]+lt_weights[lt][1])))
            return -1

    spatial_weights = conf['spatial_blending_weights']
    for x in range(0, len(spatial_weights)):
        dist, weight = spatial_weights[x]
        if weight > 1 or weight < 0:
            logg.write_time("Error: Weight %f must be between 0 and 1\n" % weight)
            return -1
        if x > 0:
            if dist <= spatial_weights[x-1][0]:
                logg.write_time("Error: Distances must increase (%d smaller than %d)\n" % (dist, spatial_weights[x-1][0]))
                return -1


    return conf
                            

def main():
    usage_str = "%prog nwp_forecast statcast_forecast site_list grid_to_site_map config_file gen_date output_file"
    usage_str += "\n nwp_forecast\t Path to input NWP(netCDF) forecast(Should be blend of NWP models)"
    usage_str += "\n statcast_forecast\t Path to input statcast forecast"
    usage_str += "\n site_list\t Path to file containing mapping from integer ID to id in 'grid_to_site_map'"
    usage_str += "\n grid_to_site_map\t Path to json file containing grid point to site mapping"
    usage_str += "\n config_file\t Path to json config file specifying spatial and temporal weights for blending"
    usage_str += "\n gen_date\t YYYYmmdd.HHMM date of generation time (15min prior to first forecast cycle)" 
    usage_str += "\n output_file\t Path to output NC file"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
    parser.add_option("-z", "--climate_zone_file", dest="climate_zone_file", help="Write Climate zone of each grid point to the output file (from this input file)")
    parser.add_option("-H", "--hourly_output_file", dest="hourly_output_file", help="Write time-ending hourly averaged data to this output file")

    (options, args) = parser.parse_args()
    
    if len(args) < 7:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  

    nwp_file = args[0]
    statcast_file = args[1]
    site_list = args[2]
    grid_site_map = args[3]
    config_file = args[4]
    fcst_date = args[5]
    output_file = args[6]

    logg.write_starting()

    # Make sure files exist:
    for f in args[:5]:
        if not os.path.exists(f):
            logg.write_time("Error: %s doesn't exist\n" % f)
            sys.exit()

    # Verify the config file
    conf = read_verify_config(config_file, logg)
    if conf == -1:
        sys.exit(-1)

    # Get datetime object
    fcst_dt = pd.to_datetime(fcst_date, format="%Y%m%d.%H%M")

    ret = process(nwp_file, statcast_file, site_list, grid_site_map, conf, fcst_dt, output_file, 
                  options.climate_zone_file, options.hourly_output_file, logg)

    logg.write_ending()


if __name__ == "__main__":
    main()
