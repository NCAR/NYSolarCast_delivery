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
"""

import time, os, sys, calendar
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
import numpy as np
from netCDF4 import Dataset, stringtoarr, chartostring, stringtochar
import json
import pytz

MAX_STR_LEN = 25
#
# Default netCDF fill values.
#
#NC_FILL_CHAR   = '\x00'
NC_FILL_CHAR  = "<missing>"
NC_FILL_SHORT  = -32767
NC_FILL_INT    = -999
#NC_FILL_FLOAT  = -9999.0
NC_FILL_FLOAT  = 9.9692099683868690e+36
NC_FILL_DOUBLE = 9.9692099683868690e+36

#OBS_TIME_NAME = "observationTime"
STN_NAME = "station_name"


def process(df, in_file_name, config_file, out_file, options, logg):
    """ Main driver function

    Args:
        df : pandas dataframe
        in_file_name: string path to input file name
        conf_file : string path to configuration file
        out_file : string path to output file
        options : options argument
        logg : log object

    Returns:
    """
    # Read the json config file lines
    logg.write_time("Reading: %s\n" % config_file)
    config_data = read_json_data(config_file)
    # Check the config file
    check_config(config_data, logg)


    # Check that all of the 'input_names' in the config file
    #   correspond to a column in the dataframe
    for v in config_data["Variables"]:
        if v['input_name'] not in df.columns:
            logg.write_time("Error: '%s' not a column in the input file."
                            "  Exiting\n" % v['input_name'])
            sys.exit(-1)

    # Figure out what the observation time variable is
    out_names = []
    for v in config_data['Variables']:
        out_names.append(v['output_name'])
    if 'observationTime' in out_names:
        OBS_TIME_NAME = 'observationTime'
    elif 'observation_time' in out_names:
        OBS_TIME_NAME = 'observation_time'
    else:
        logg.write_time("Error: Can't figure out what the observation time var is, add option to pass this in\n")
        sys.exit(-1)
    
    # Remove rows that are all missing
    df.dropna(how='all', inplace=True)
    
    #
    # Create the netcdf file
    #
    if options.cdl:
        nc_obj = create_nc_cdl(out_file, options.cdl, logg)
    else:
        nc_obj = create_nc_config(in_file_name, out_file, df, config_data, OBS_TIME_NAME, options, logg)

    # Loop for the command line option to set all missing input data
    if options.input_fill:
        df.replace(options.input_fill, np.nan, inplace=True)
        
    #
    # Set any "_FillValue" data to np.nan
    #
    for v in config_data['Variables']:
        # Get nc var object
        nc_var = nc_obj.variables[v['output_name']]

        # Get the NC fill value, and replace all vals that match this with np.nan
        if "_FillValue" in nc_var.ncattrs():
            nc_fill = nc_var._FillValue
            # Replace NC fill value with NaN
            df[v['input_name']].replace(nc_fill, np.nan, inplace=True)
        
        # Get the config var fill value (should usually be same as nc, unless using a cdl)
        var_type = get_pd_var_type_str(df, v['input_name'], logg)
        conf_fill = get_fill_val(v, var_type, logg)
        if conf_fill != False:
            # Replace config_fill value with NaN
            df[v['input_name']].replace(conf_fill, np.nan, inplace=True)             
            
    #
    # Do any units conversions
    # 
    for v in config_data['Variables']:
        if 'conversion' in v:
            # Perform units conversion
            perform_unit_conversion(df, v['input_name'], v['conversion'], logg)
        if 'input_time_format' in v:
            # Perform time conversion
            perform_time_conversion(df, v['input_name'], v['input_time_format'], logg)
    
 
    #
    # Fill in the netCDF data
    #
    if options.siteDim:        
        write_site_nc(nc_obj, df, config_data, OBS_TIME_NAME, options, logg)
    else:
        write_nc(nc_obj, df, config_data, OBS_TIME_NAME, options, logg)

    #
    # Print out unused cols (if necessary)
    #
    if options.unused_cols:
        in_vars, out_vars = get_config_vars(config_data)
        for col in df.columns.tolist():
            if col not in in_vars:
                logg.write_time("Unused column: %s\n" % col)


def create_nc_cdl(out_file, cdl_file, logg):
    """ Creates the netCDF file using the provided cdl
    
    Args:
        out_file : string path to output file to create 
        cdl_file : string path to cdl file to use
        logg : log object

    Returns:
        out_nc : NetCDF4 Dataset
    """
    # Create the netcdf file
    cmd = "ncgen %s -o %s" % (cdl_file, out_file)
    logg.write_time("Executing: %s\n" % cmd)
    ret = os.system(cmd)
    if ret != 0:
        logg.write("Failure: %d -- Exiting\n" % ret)
        sys.exit(-1)
        
    # Open the netCDF file for writing
    out_nc = Dataset(out_file, 'r+')

    return out_nc


def create_nc_config(in_file_name, out_file, df, config_data, OBS_TIME_NAME, options, logg):
    """ Creates the netCDF file from the info provided in the config file

    Args:
        in_file_name : string corresponding to input file name
        out_file : string path to output file to create
        df : pandas dataframe
        config_data : configuration object
        OBS_TIME_NAME : string corresponding to name of output obs time variable
        options : options object
        logg : log object
    
    Returns:
        out_nc : NetCDF4 Dataset
    """
    # Get the input and output variable names from the config file
    in_vars, out_vars = get_config_vars(config_data)
    
    # Get whatever extra dimensions need to be created for the variables
    var_dim_map = get_variable_dimensions(config_data, logg)

    # Create the netCDF file
    logg.write_time("Writing output file: %s\n" % out_file)
    out_nc = Dataset(out_file, 'w', data_model='NETCDF4')

    # Create the dimensions
    out_nc.createDimension("rec_num")
    #out_nc.createDimension("str_len", MAX_STR_LEN)
    if options.station_name:
        out_nc.createDimension("scalar_dim", 1)
    for d in var_dim_map:
        out_nc.createDimension(d+"_dim", len(var_dim_map[d]))
    if options.lat or options.lon or options.elevation:
        dim_len = 0
        if options.lat:
            dim_len = len(options.lat.split(","))
        elif options.lon:
            dim_len = len(options.lon.split(","))
        else:
            dim_len = len(options.elevation.split(","))
        # If there is a station_name_dim and it is same size as lat/lon/elev.  Use that
        lat_lon_elev_dim = False
        for d in out_nc.dimensions:
            if out_nc.dimensions[d].size == dim_len:
                lat_lon_elev_dim = d
        if lat_lon_elev_dim == False:
            lat_lon_elev_dim = 'meta_dim'
            out_nc.createDimension("meta_dim", dim_len)
        
    # Create the dimension variables
    for d in var_dim_map:
        if isinstance(var_dim_map[d][0], int):
            this_dim_var = out_nc.createVariable(d, 'i', (d+"_dim",))
        elif isinstance(var_dim_map[d][0], float):
            this_dim_var = out_nc.createVariable(d, 'f', (d+"_dim",))
        elif isinstance(var_dim_map[d][0], str):
            this_dim_var = out_nc.createVariable(d, 'S4', (d+"_dim"))            
        else:
            logg.write_time("Error: Not sure how to create this dimension type: %s\n" % d)
            sys.exit(-1)        
        
    # For later use
    stn_var_f = False
        
    # If a site_dimension was passed, add that
    if options.siteDim:
        stn_var_f = True
        file_sites = df[options.siteDim].unique().tolist()
        out_nc.createDimension("station_name_dim", len(file_sites))
        stn_dim_var = out_nc.createVariable(STN_NAME, 'S4',
                                            ('station_name_dim'), zlib=True)                
        
        # If there is a site meta file, add lat/lon/elevation
        #       (with dimensions equal to the stationName)
        if options.site_list or options.illef:
            create_default_var(out_nc, 'latitude', 'f', 'latitude', 'degree_north', ('station_name_dim',))
            create_default_var(out_nc, 'longitude', 'f', 'longitude', 'degree_east', ('station_name_dim',))
            create_default_var(out_nc, 'elevation', 'f', 'elevation', 'm', ('station_name_dim',))            
    else:
        # Add these variables based off of input options
        #    (no site dim, so they are flat (dimension = rec_num)
        # If a site_list is passed, set all vars
        if options.site_list or options.illef:
            # Create StationName
            if STN_NAME not in out_nc.variables:
                create_default_var(out_nc, STN_NAME, 'S4', 'Station Name', '', ('rec_num'))                
            stn_var_f = True
            create_default_var(out_nc, 'latitude', 'f', 'latitude', 'degree_north', ('rec_num',))
            create_default_var(out_nc, 'longitude', 'f', 'longitude', 'degree_east', ('rec_num',))            
            create_default_var(out_nc, 'elevation', 'f', 'elevation', 'm', ('rec_num',))
        else:
            # Create the meta vars with a single element
            # Add elevation based on input options
            if options.station_name:
                create_default_var(out_nc, STN_NAME, 'S4', 'Station Name', '', ('scalar_dim'))
                stn_var_f = True               
            if options.lat:
                create_default_var(out_nc, 'latitude', 'f', 'latitude', 'degree_north', (lat_lon_elev_dim,))
            if options.lon:
                create_default_var(out_nc, 'longitude', 'f', 'longitude', 'degree_east', (lat_lon_elev_dim,))
            if options.elevation:
                create_default_var(out_nc, 'elevation', 'f', 'elevation', 'm', (lat_lon_elev_dim,))            
            

    # Create the OBS_TIME_NAME variable
    obs_conf_var = get_var_from_config(config_data, OBS_TIME_NAME)
    this_fill = get_fill_val(obs_conf_var, 'i', logg)
    if this_fill != False:
        fill = this_fill
    else:
        fill = NC_FILL_INT
        
    obs_time_var = out_nc.createVariable(OBS_TIME_NAME, 'd', ('rec_num',),
                                         fill_value=fill, zlib=True)
    obs_time_var.long_name = "UTC time of observation"
    obs_time_var.units = "seconds since 1970-1-1 00:00:00.0"
    # Set any other attributes
    set_var_attributes(obs_conf_var, obs_time_var, logg)

    # If a localTime or utcTime offset is passed, create the localTime var
    if options.local_offset or options.utc_offset:
        create_default_var(out_nc, 'observation_time_local', 'i', 'Local time of observation',
                           "seconds since 1970-1-1 00:00:00.0", ('rec_num',))
        lcl_time_var = out_nc.variables['observation_time_local']
        if options.local_offset:
            if options.local_offset.replace('-','').isdigit():
                if int(options.local_offset) > 0:
                    lcl_zone = "UTC+%d" % (int(options.local_offset))
                else:
                    lcl_zone = "UTC%d" % (int(options.local_offset))
            else:
                lcl_zone = options.local_offset
                        
        else:
            if options.utc_offset.replace('-','').isdigit():
                if int(options.utc_offset) > 0:
                    lcl_zone = "UTC-%d" % (int(options.utc_offset))
                else:
                    lcl_zone = "UTC+%d" % (abs(int(options.utc_offset)))
            else:
                lcl_zone = options.utc_offset
        lcl_time_var.setncattr("time_zone", lcl_zone)
        
    #
    # Create the rest of the variables
    #
    multi_dim_var_list = []
    for var_info in config_data['Variables']:
        # Get the output var name    
        this_out_var = var_info['output_name']
        this_in_var = var_info['input_name']

        if this_out_var == OBS_TIME_NAME:
            continue
        
        # If the station is used as a dimension, don't create that var.
        #     it is created separately
        if options.siteDim and this_in_var==options.siteDim:
            continue
        
        # If stationName is a column, but it was already created from the command options,
        #               don't create again
        if stn_var_f == True and this_out_var == STN_NAME:
            #logg.write_time("stationName specified as row in input file, "
            #                "but also argument to set station name was passed.  "
            #                "Going with value from provided argument\n")
            continue
        
        # Get the type of this data
        var_type = get_pd_var_type_str(df, this_in_var, logg)
        if var_type == 'f':
            fill = NC_FILL_FLOAT
        elif var_type == 'i':
            fill = NC_FILL_INT
        elif var_type == 'S4':
            fill = NC_FILL_CHAR
        else:
            logg.write_time("Warning: Not sure what type of var '%s' is. "
                            "Skipping\n" % this_in_var)
            continue

        # Look for a _FillValue from the config file
        conf_fill = get_fill_val(var_info, var_type, logg)
        if conf_fill != False:
            fill = conf_fill

            
        var_found = False                
        var_dims = ('rec_num',)
        if options.siteDim:
            var_dims += ('station_name_dim',)                            
        if "dimension_name" in var_info:
            # Get the dimension attributes
            dim_names = var_info['dimension_name']
            if type(dim_names) != list:
                dim_names = [dim_names]
                
            # Check if this variable has already been created
            if this_out_var in multi_dim_var_list:
                var_found = True
                continue
            for dim_name in dim_names:
                var_dims += (dim_name+'_dim',)                

        if var_found == False:
            this_var = out_nc.createVariable(this_out_var, var_type,
                                             var_dims, fill_value=fill, zlib=True)                     
            # Add any attributes
            set_var_attributes(var_info, this_var, logg)

            # Add it to the list so we don't create it again
            multi_dim_var_list.append(this_out_var)

    # Add a 'input_file' global attribute
    out_nc.setncattr("input_file", in_file_name.split("/")[-1])
    out_nc.setncattr("creation_time", time.strftime("%Y/%m/%d %H:%M:%S", time.gmtime(time.time()))) 
    # Add the global attributes
    for attr in config_data['Global Attributes']:
        for key, value in attr.items():
            if type(value) == list:
                if type(value[0]) == str:
                    out_nc.setncattr_string(key, value)
                else:
                    out_nc.setncattr(key, value)                                                    
            else:
                out_nc.setncattr(key, value)
            
    return out_nc

def write_site_nc(out_nc, df, config_data, OBS_TIME_NAME, options, logg):
    """ Fills in the netcdf file that was already created

    Args:
        out_nc : NetCDF4 Dataset
        df : Pandas DataFrame
        config_data : configuration object (dictionary) 
        OBS_TIME_NAME : string corresponding to name of output obs time variable
        options : options object
        logg : log object

    Returns:
    """
    # Get the input and output variable names from the config file
    in_vars, out_vars = get_config_vars(config_data)
    
    # Make sure we have the time field
    if OBS_TIME_NAME not in out_vars:
        logg.write_time("Error: '%s' not in variable list.  Specify this\n" % OBS_TIME_NAME)
        sys.exit(-1)

    # Get the variable dimension map (for indexing and other uses)
    var_dim_map = get_variable_dimensions(config_data, logg)
    
    # Set the vars containing values for the dimensions    
    set_nc_dimensional_vars(out_nc, var_dim_map, logg)
    
    # Sort the dataframe by time
    obs_df_var = in_vars[out_vars.index(OBS_TIME_NAME)]    
    df.sort_values(obs_df_var, inplace=True)

    # Only get unique times (don't repeat them for each site)
    native_obs_times = df[obs_df_var].unique().tolist()
    # Set the OBS_TIME_NAME variable (and 'localTime' if that's passed)
    set_nc_obs_time(out_nc, native_obs_times, OBS_TIME_NAME, options, logg)
    
    # Set the stationName variable
    sites = sorted(df[options.siteDim].unique().tolist())
    set_nc_var_data(out_nc, STN_NAME, sites, logg)

    #
    # If we have a site_list, set the lat/lon/elevation info
    #
    if options.site_list or options.illef:
        site_meta_df = get_site_meta_map(options, logg)
        lat_arr = []
        lon_arr = []
        elev_arr = []
        for s in sites:
            if s in site_meta_df.index:
                site_row = site_meta_df.ix[s]
                lat_arr.append(site_row['lat'])
                lon_arr.append(site_row['lon'])
                elev_arr.append(site_row['elev'])
            else:
                lat_arr.append(NC_FILL_FLOAT)
                lon_arr.append(NC_FILL_FLOAT)
                elev_arr.append(NC_FILL_FLOAT)
        set_nc_var_data(out_nc, 'latitude', lat_arr, logg)
        set_nc_var_data(out_nc, 'longitude', lon_arr, logg)
        set_nc_var_data(out_nc, 'elevation', elev_arr, logg)
    # Set lat/lon vars based on provided lat/lon pair        
    if options.lat:
        lats = ",".split(options.lat)
        lats = [float(f) for f in lats]
        set_nc_var_data(out_nc, 'latitude', lats, logg)
    if options.lon:
        lons = ",".split(options.lon)
        lons = [float(f) for f in lons]
        set_nc_var_data(out_nc, 'longitude', lons, logg)
    if options.elevation:
        elevs = ",".split(options.elevation)
        elevs = [float(f) for f in elevs]
        set_nc_var_data(out_nc, 'elevation', elevs, logg)
        
    #
    # Set the output variables
    #
    # Make sure there is an entry for each site at each time
    df = populate_missing(df, obs_df_var, options.siteDim, logg)

    # Sort the dataframe by time and site
    df.sort_values([obs_df_var, options.siteDim], inplace=True)
    
    multi_dim_var_map = {}
    for var_info in config_data['Variables']:
        out_var_name = var_info['output_name']
        in_var_name = var_info['input_name']
        
        # Skip OBS_TIME_NAME (already created above)
        if out_var_name == OBS_TIME_NAME:
            continue

        # Skip stationName (used as a dimension)
        if out_var_name == STN_NAME:
            continue

        # Get the NC variable object
        this_var = out_nc.variables[out_var_name]
    
        # Get the fill value
        fill = np.nan
        if "_FillValue" in this_var.ncattrs():                
            fill = this_var._FillValue
    
        # Set multi-dimensional variable values
        if 'dimension_name' in var_info:
            # Get the variable dimensions into a list
            dim_names = var_info['dimension_name']
            dim_values = var_info['dimension_value']
            if type(dim_names) != list:
                dim_names = [dim_names]
            if type(dim_values) != list:
                dim_values = [dim_values]                
            
            # Get the values from the dataframe
            these_vals = df[in_var_name].fillna(fill).values
                                         
            # Check if this variable has already been added to the map, add it
            if out_var_name not in multi_dim_var_map:
                # Create the multidim np array
                multi_dim_var_map[out_var_name] = np.zeros(shape=this_var.shape)
                # Set all array to the fill value before adding actual data
                multi_dim_var_map[out_var_name][multi_dim_var_map[out_var_name]==0] = fill  

            if len(dim_names) == 1:
                dim_name = dim_names[0]
                dim_value = dim_values[0]
                
                # Get the dimension index of this column
                out_ind = get_dimension_index(dim_name, dim_value, var_dim_map)
                            
                # Set the data in the numpy array (later set the NC var)
                #    (faster to do it this way)
                multi_dim_var_map[out_var_name][:,:,out_ind] = these_vals.reshape(len(native_obs_times),
                                                                                  len(sites))
            else:
                # Handle 2 extra dimensions
                if len(dim_names) > 2:
                    logg.write_time("Error: Can't handle more than 2 extra dimensions at this point\n")
                    sys.exit(-1)
                d1_name = dim_names[0]
                d1_value = dim_values[0]
                d2_name = dim_names[1]
                d2_value = dim_values[1]
                # Get the 1st dimension index of this column 
                d1_ind = get_dimension_index(d1_name, d1_value, var_dim_map)
                # Get the 2nd dimension index of this column 
                d2_ind = get_dimension_index(d2_name, d2_value, var_dim_map)

                # Set the data in the numpy array (later set the NC var)
                #    (faster to do it this way)
                multi_dim_var_map[out_var_name][:,:,d1_ind,d2_ind] = these_vals.reshape(len(native_obs_times),
                                                                                        len(sites))
                
                
        else:
            # Set the single dimensional vars
            logg.write_time("Writing: %s\n" % out_var_name)

            these_vals = df[in_var_name].fillna(fill).values
            
            # Reshape the values
            this_var[:] = these_vals.reshape(this_var.shape)
        
    # Set the multi-dim variables
    for var_name in multi_dim_var_map:
        set_nc_var_data(out_nc, var_name, multi_dim_var_map[var_name], logg)
                
    out_nc.close()
    return

def write_nc(out_nc, df, config_data, OBS_TIME_NAME, options, logg):
    """ Fills in the netcdf file that was already created

    Args:
        out_nc : NetCDF4 Dataset
        df : Pandas DataFrame
        config_data : configuration object (dictionary)
        OBS_TIME_NAME : string corresponding to name of output obs time variable
        options : options object
        logg : log object

    Returns:
    """
    # Get the input and output variable names from the config file
    in_vars, out_vars = get_config_vars(config_data)
        
    # Get the variable dimension map (for indexing and other uses)
    var_dim_map = get_variable_dimensions(config_data, logg)
    
    # Set the vars containing values for the dimensions    
    set_nc_dimensional_vars(out_nc, var_dim_map, logg)

    # Get the time values from the dataframe
    obs_df_var = in_vars[out_vars.index(OBS_TIME_NAME)]
    native_obs_times = df[obs_df_var].values
    # Set the OBS_TIME_NAME variable
    #    and localTime var if that option was passed
    set_nc_obs_time(out_nc, native_obs_times, OBS_TIME_NAME, options, logg)

    # Add a station name if it was passed
    if options.station_name:
        station_name_var = out_nc.variables[STN_NAME]
        these_sites = [options.station_name] * len(station_name_var)
        set_nc_var_data(out_nc, STN_NAME, these_sites, logg)

    # Add site meta info if a site list was passed
    if options.site_list or options.illef:
        if STN_NAME not in out_vars:
            logg.write_time("Error: site list passed, but '%s' column not specified"
                            " in output file, so code doesn't know what field to map "
                            "to the meta information, skipping\n" % STN_NAME)
        else:
            site_meta_df = get_site_meta_map(options, logg)
            site_df_field = in_vars[out_vars.index(STN_NAME)]
            lat_arr = []
            lon_arr = []
            elev_arr = []
            for ind, row in df.iterrows():
                ic_id = row[site_df_field]
                if ic_id in site_meta_df.index:
                    site_row = site_meta_df.ix[ic_id]
                    lat_arr.append(site_row['lat'])
                    lon_arr.append(site_row['lon'])
                    elev_arr.append(site_row['elev'])
                else:
                    lat_arr.append(NC_FILL_FLOAT)
                    lon_arr.append(NC_FILL_FLOAT)
                    elev_arr.append(NC_FILL_FLOAT)
            set_nc_var_data(out_nc, 'latitude', lat_arr, logg)
            set_nc_var_data(out_nc, 'longitude', lon_arr, logg)
            set_nc_var_data(out_nc, 'elevation', elev_arr, logg)
            
    if options.lat:
        lats = options.lat.split(",")
        lats = [float(f) for f in lats]
        set_nc_var_data(out_nc, 'latitude', lats, logg)
    if options.lon:
        lons = options.lon.split(",")
        lons = [float(f) for f in lons]
        set_nc_var_data(out_nc, 'longitude', lons, logg)
    if options.elevation:
        elevs = options.elevation.split(",")
        elevs = [float(f) for f in elevs]
        set_nc_var_data(out_nc, 'elevation', elevs, logg)
        
    # Set the output variables
    multi_dim_var_map = {}
    for var_info in config_data['Variables']:
        out_var_name = var_info['output_name']        
        in_var_name = var_info['input_name']
        
        # Skip OBS_TIME_NAME (already created above)
        if out_var_name == OBS_TIME_NAME:
            continue

        # if an output variable wasn't created, skip
        if out_var_name not in out_nc.variables:
            logg.write_time("Warning: %s doesn't exist in output file. Ignoring\n" % out_var_name)
            continue

        # Get the NC variable object
        this_var = out_nc.variables[out_var_name]
    
        # Get the fill value
        fill = np.nan
        if "_FillValue" in this_var.ncattrs():                
            fill = this_var._FillValue
        
        
        # Set multi-dimensional variable values
        if 'dimension_name' in var_info:
            # Get the variable dimensions into a list            
            dim_names = var_info['dimension_name']
            dim_values = var_info['dimension_value']
            if type(dim_names) != list:
                dim_names = [dim_names]
            if type(dim_values) != list:
                dim_values = [dim_values]
                
            # Check if this variable has already been added to the map
            if out_var_name not in multi_dim_var_map:
                # Create the multidim np array
                multi_dim_var_map[out_var_name] = np.zeros(shape=this_var.shape)
                # Set the values to the fill value
                multi_dim_var_map[out_var_name].fill(fill)

            if len(dim_names) == 1:
                dim_name = dim_names[0]
                dim_value = dim_values[0]
                # Get the dimension index of this column
                out_ind = get_dimension_index(dim_name, dim_value, var_dim_map)
                
                # Set the data in the numpy array
                #   (later set the NC var -- faster to do it this way)
                multi_dim_var_map[out_var_name][:,out_ind] = df[in_var_name].fillna(fill).values
            else:
                # Handle 2 extra dimensions
                if len(dim_names) > 2:
                    logg.write_time("Error: Can't handle more than 2 extra dimensions at this point\n")
                    sys.exit(-1)

                d1_name = dim_names[0]
                d1_value = dim_values[0]
                d2_name = dim_names[1]
                d2_value = dim_values[1]
                # Get the 1st dimension index of this column 
                d1_ind = get_dimension_index(d1_name, d1_value, var_dim_map)
                # Get the 2nd dimension index of this column 
                d2_ind = get_dimension_index(d2_name, d2_value, var_dim_map)
                multi_dim_var_map[out_var_name][:,d1_ind,d2_ind] = df[in_var_name].fillna(fill).values 
            

        # Set the single dimensional vars
        else:
            logg.write_time("Writing: %s\n" % out_var_name)
            if this_var.datatype == 'S4' or this_var.datatype == 'S1':
                # if the input data is an integer, change it to a string
                if df[in_var_name].dtype != np.object:
                    df[in_var_name] = df[in_var_name].astype(str)
                # set string variable                
                data_strings = df[in_var_name].fillna(fill).values
                str_len = this_var.shape[-1]
                data_chars = [stringtoarr(d_st[:str_len], str_len) for d_st in data_strings]
                this_var[:] = data_chars
            else:
                this_var[:] = df[in_var_name].fillna(fill).values                

        
    # Set the multi-dim variables
    for var_name in multi_dim_var_map:
        set_nc_var_data(out_nc, var_name, multi_dim_var_map[var_name], logg)
                
    out_nc.close()
    return

def perform_unit_conversion(df, var_name, conversion, logg):
    """ Performs some predefined units conversions

    Args:
        df : Pandas DataFrame
        var_name : string corresponding to variable name
        conversion : string corresponding to conversion to perform
        logg : log object

    Returns:
    """
    # First, if the column is not a float, attempt to convert
    if df[var_name].dtype == np.object:
        try:
            df[var_name] = pd.to_numeric(df[var_name])
        except:
            logg.write_time("Error: Attempting to do unit conversion on field(%s) that isn't a float.\n"  % var_name)
            sys.exit(-1)
        
    if conversion == "C_to_K":
        df[var_name] = df[var_name] + 273.15
    elif conversion == "K_to_C":
        df[var_name] = df[var_name] - 273.15
    elif conversion == "mm_to_cm":
        df[var_name] = df[var_name] / 10
    elif conversion == "mm_to_m":
        df[var_name] = df[var_name] / 1000
    else:
        logg.write_time("Unrecognized units conversion, ignoring\n")
    return

def perform_time_conversion(df, var_name, conversion, logg):
    """ Converts a time string to epoch seconds 
    
    Args:
        df : pandas DataFrame
        var_name : string corresponding to variable name
        conversion : format of the input time string
    """
    df[var_name] = pd.to_datetime(df[var_name], format=conversion).astype(np.int64) // 10**9
    return

def get_pd_var_type_str(df, this_in_var, logg):
    """ Gets the variable type (as a string) of the input var

    Args:
        df : Pandas DataFrame
        this_in_var : string of variable name to check the type
        logg : log object
    
    Returns:
        var_type : string corresponding to variable type
    """
    if df[this_in_var].dtype == float:
        var_type = 'f'
    elif df[this_in_var].dtype == int:
        var_type = 'i'
    elif df[this_in_var].dtype == 'int64':
        var_type = 'i'        
    elif df[this_in_var].dtype == object:
        var_type = 'S4'
    else:
        logg.write_time("Warning: Not sure what type of var '%s' is ('%s')." % (this_in_var,
                                                                                df[this_in_var].dtype))
        var_type = ''
    return var_type
        
    
def set_var_attributes(var_info, nc_var, logg):
    """ Sets variable attributes based on a map
    
    Args:
        var_info : dictionary containing attribute information
        nc_var : NetCDF4.Variable
        logg : log object

    Returns:
    """
    if "Attributes" in var_info:
        for a in var_info['Attributes']:
            for key, value in a.items():
                if key == "_FillValue":
                    continue
                if type(value) == list:
                    if type(value[0]) == str:
                        nc_var.setncattr_string(key, value)
                    else:
                        nc_var.setncattr(key, value)                                                    
                else:
                    nc_var.setncattr(key, value)
    return


def get_fill_val(var_info, var_type, logg):
    """ Gets the fill value of a variable

    Args:
        var_info : dictionary containing attribute information
        var_type : string type of variable 
        logg : log object
    
    Returns:
        fill : FillValue of the variable else False if _FillValue doesn't exist
    """
    if 'Attributes' in var_info:
        if '_FillValue' in var_info['Attributes'][0]:
            if var_type == 'f':
                fill = float(var_info['Attributes'][0]['_FillValue'])
            elif var_type == 'i':
                fill = int(var_info['Attributes'][0]['_FillValue'])
            else:
                fill = var_info['Attributes'][0]['_FillValue']
            return fill
        else:
            return False
    return False
  
def get_dimension_index(dim_name, dim_value, var_dim_map):
    """ Returns the index in the var_dim_map of the dim_value

    Args:
        dim_name : string name of the dimension
        dim_value : value of the dimension
        var_dim_map : dictionary mapping dimensions to indexes 

    Returns:
        out_ind : index of the dimension in the variable   
    """    
    dim_vals = var_dim_map[dim_name]
    out_ind = dim_vals.index(dim_value)
    return out_ind

def create_default_var(nc_obj, var_name, var_type, var_long_name, var_units, dims):
    """ Creates a default NetCDF variable
    
    Args:
        nc_obj : NetCDF4 Dataset object
        var_name : string corresponding to the variable name
        var_type : string corresponding to the variable type
        var_long_name : string corresponding to the long name of the variable
        var_units : string corresponding to the units of the variable
        dims : tuple containing the dimensions of the variable
    
    Returns:
    """
    if var_type == 'f':
        fill_value = NC_FILL_FLOAT
    elif var_type == 'i':
        fill_value = NC_FILL_INT
    else:
        fill_value = NC_FILL_CHAR

    this_var = nc_obj.createVariable(var_name, var_type, dims, fill_value = fill_value, zlib=True)
    if var_long_name != '':
        this_var.long_name = var_long_name
    if var_units != '':
        this_var.units = var_units
    return
             
    
def set_nc_obs_time(nc_obj, native_obs_times, OBS_TIME_NAME, options, logg):
    """ Sets the OBS_TIME_NAME variable.  Sets the 'localTime' variable if necessary

    Args:
        nc_obj : NetCDF4 Dataset object
        native_obs_times : list of input observation times (time strings)
        OBS_TIME_NAME : string name to use for the time variable
        options : options object
        logg : log object

    Returns:
    """
    # Reformat the date if we need to
    if options.date_format:
        native_obs_times = [tme2epoch(str(o), options.date_format) for o in native_obs_times]
    # Convert local time to utc time
    if options.utc_offset:
        if options.utc_offset.replace('-','').isdigit():
            offset = int(options.utc_offset)*3600
            utc_obs_times = [n+offset for n in native_obs_times]
        else:
            native_datetimes = pd.to_datetime(native_obs_times, unit="s").tz_localize(options.utc_offset)
            utc_obs_times = [t.tz_convert("UTC").timestamp() for t in native_datetimes]
    else:
        utc_obs_times = native_obs_times

    # Set the observation time (Hardcoded for now -- Should this not be?)        
    set_nc_var_data(nc_obj, OBS_TIME_NAME, utc_obs_times, logg)

    # Set the localTime variable if that option was passed
    # If a local time offset is passed, add that variable
    if options.local_offset:
        if options.local_offset.replace('-','').isdigit():
            # Add/subtract constant shift
            offset = int(options.local_offset) * 3600
            lcl_times = [t+offset for t in utc_obs_times]
        else:
            # Need to adjust for a time zone
            utc_datetimes = pd.to_datetime(utc_obs_times, units="s", utc=True)
            lcl_dt = utc_datetimes.tz_convert(options.local_offset)
            lcl_times = [t.timestamp() for t in lcl_dt]
        set_nc_var_data(nc_obj, 'observation_time_local', lcl_times, logg)
    # If a utc time offset is passed, add the local times from the file
    if options.utc_offset:
        set_nc_var_data(nc_obj, 'observation_time_local', native_obs_times, logg)
    
    return

    
def set_nc_dimensional_vars(nc_obj, var_dim_map, logg):
    """ Set the dimension variables    

    Args:
        nc_obj : NetCDF4 Dataset object
        var_dim_map : dictionary mapping variables to dimensions
        logg : log object

    Returns:
    """

    for d in var_dim_map:
        dim_arr = var_dim_map[d]
        # Fill the netcdf data
        set_nc_var_data(nc_obj, d, dim_arr, logg)
        
    return

    
def set_nc_var_data(nc_obj, nc_var_name, data_arr, logg):
    """ Sets the variable data for a given nc var name

    Args:
        nc_obj : NetCDF4 Dataset object
        nc_var_name : Name of variable
        data_arr : data to use to fill 'nc_var_name'
        logg : log object

    Returns:
    """
    logg.write_time("Writing: %s\n" % nc_var_name)
    this_var = nc_obj.variables[nc_var_name]
    var_type = this_var.dtype
    if var_type == str:
        for k in range(0, len(data_arr)):
            this_var[k] = data_arr[k]
    else:
        this_var[:] = data_arr
    return

    
def get_site_meta_map(options, logg):
    """ Get lat/lon/elevation info for each site

    Args:
        options : options object
        logg : log object

    Returns:
        s_df : Pandas Dataframe containing site information
    """
    site_meta = {}    
    if options.site_list:
        inF = open(options.site_list, "r")
        for line in inF:
            fields = line.rstrip("\r\n").split(";")
            ic_id = fields[2]
            lat = float(fields[3])
            lon = float(fields[4])
            elev = float(fields[5])
            site_meta[ic_id] = {'lat':lat, 'lon':lon,'elev':elev}
        inF.close()
    elif options.illef:
        inF = open(options.site_list, "r")
        for line in inF:
            fields = line.rstrip("\r\n").split(",")
            ic_id = fields[0]
            lat = float(fields[1])
            lon = float(fields[2])
            elev = float(fields[3])
            site_meta[ic_id] = {'lat':lat, 'lon':lon,'elev':elev}
        inF.close()

    # Convert the map to a pandas dataframe (index = id)
    s_df = pd.DataFrame.from_dict(site_meta, orient='index')
    return s_df
    

def get_variable_dimensions(config_data, logg):
    """ For vars that have multiple dimensions -- this maps the dimension name to the dimension values

    Args:
        config_data : Dictionary containing configuration information
        logg : log object

    Returns:
        dim_map : Dictionary mapping dimension names to dimension values
    """
    dim_map = {}
    for v in config_data['Variables']:
        if "dimension_name" in v.keys():
            if "dimension_value" not in v.keys():
                logg.write_time("Error: for var: %s no dimension_value listed\n" % v["input_name"])
                sys.exit(-1)

            if type(v['dimension_value']) == list:
                if len(v['dimension_value']) != len(v['dimension_name']):
                    logg.write_time("Error: for var: %s len(dimension_value) != len(dimension_name)\n"
                                     % v["input_name"])
                    sys.exit(-1)
                for d in range(0, len(v['dimension_value'])):
                    dim_value = v["dimension_value"][d]
                    dim_name = v["dimension_name"][d]
                    if dim_name not in dim_map:
                        dim_map[dim_name] = [dim_value]
                    else:
                        if dim_value not in dim_map[dim_name]:
                            dim_map[dim_name].append(dim_value)
            else:
                dim_value = v["dimension_value"]
                dim_name = v["dimension_name"]
                if dim_name not in dim_map:
                    dim_map[dim_name] = [dim_value]
                else:
                    if dim_value not in dim_map[dim_name]:
                        dim_map[dim_name].append(dim_value)
                
    return dim_map
            

def get_global_attributes(config_data, logg):
    """ Gets the global attributes and values from the config file

    Args:
        config_data : Dictionary containing configuration information
        logg : log object

    Returns:
        glob_attrs : list of tuples (attribute_name, attribute_value)
    """
    glob_attrs = []
    
    glob_start = False
    for conf_line in config_lines[1:]:
        if conf_line.startswith("Global Attributes:"):
            glob_start = True
            continue
        # Skip lines until we see 'Global Attributes'
        if glob_start == False:
            continue
        fields = conf_line.split(',')
        if len(fields) != 2:
            logg.write_time("Error with global attribute: %s.  "
                            "Should only have 2 fields\n" % conf_line)
            continue
        glob_attrs.append(fields)

    return glob_attrs                    

def populate_missing(df, time_var, site_var, logg):
    """ If there is a missing site-time pair, this will add rows for each

    Args:
        df : Pandas DataFrame
        time_var : string corresponding to time column from df
        site_var : string corresponding to site column from df
        logg : log object

    Returns:
        df : Pandas DataFrame
    """
    add_sites = []
    add_times = []
    
    all_times = df[time_var].unique().tolist()
    all_sites = df[site_var].unique().tolist()

    for s in all_sites:
        site_df = df[df[site_var]==s]
        site_times = site_df[time_var].unique().tolist()
        if site_times != all_times:
            # Add empty rows for the missing times
            for t in all_times:
                if t not in site_df[time_var].values:
                    add_sites.append(s)
                    add_times.append(t)
    add_df = pd.DataFrame(columns=df.columns)
    add_df[site_var] = add_sites
    add_df[time_var] = add_times

    return (df.append(add_df))
            

def check_config(config_data, logg):
    """ Checks that the configuration data contains all necessary information (exits if failure)

    Args:
        config_data : Dictionary containing configuration information
        logg : log object

    Returns:
    """
    # Check the variables
    if 'Variables' not in config_data:
        logg.write_time("Error: No 'Variables' listed in your config file.  Exiting\n")
        sys.exit(-1)

    conf_vars = config_data['Variables']
    obsTime = False
    multi_dim_map = {}
    input_names = []
    for v in range(0, len(conf_vars)):
        var = conf_vars[v]
        if var['output_name'] == 'observationTime' or var['output_name'] == 'observation_time':
            obsTime = True
        if "input_name" not in var:
            logg.write_time("Error: No 'input_name' listed in var number %d."
                            "  Exiting\n" % v)
            sys.exit(-1)
        if var['input_name'] in input_names:
            logg.write_time("Warning: '%s' listed twice as an input_name" % var['input_name'])
        input_names.append(var['input_name'])
        if "output_name" not in var:
            logg.write_time("Error: No 'output_name' listed in var number %d."
                            "  Exiting\n" % v)
            sys.exit(-1)
        # Make sure that matching output_names share the same dimension_names
        if "dimension_name" in var:
            if var['output_name'] not in multi_dim_map:
                multi_dim_map[var['output_name']] = var['dimension_name']
            else:
                if var['dimension_name'] != multi_dim_map[var['output_name']]:
                    logg.write_time("Error: 'dimension_name' value mismatch(%s vs %s) for output_name: %s.\n" % (
                        var['dimension_name'], multi_dim_map[var['output_name']], var['output_name']))
                    sys.exit(-1)
    if obsTime == False:
        logg.write_time("Error: No 'observation_time' or 'observationTime' field specified.  Pass in as an option.  Exiting\n")
        sys.exit(-1)

    logg.write_time("Config file passed preliminary checks\n")
    return

def read_json_data(filename):
    """ Reads data from a json file

    Args:
        filename : string path to input file
    
    Returns:
        data : dictionary containing data from input json file (filename)
    """
    data = []
    json_file = open(filename)
    json_str = json_file.read()
    data = json.loads(json_str)
    return data
 
def file_exists(file, logg):
    """ Checks if a file exists

    Args:
        file : string path to file
        logg : log object

    Returns:
       file_status : 0 = exists, -1 = doesn't exist

    """
    if not os.path.exists(file):
        logg.write_time("Error: %s doesn't exist.\n" % file)
        return -1
    return 0


def tme2epoch(time_str, frmt_str):
    """ Converts a time string to epoch seconds

    Args:
        time_str : string which matches the frmt_str
        frmt_str : string which matches the time (ie. %Y-%m-%d %H:%M%S)

    Returns:
        time : an integer corresponding to the epoch time
    """
    ts = time.strptime(time_str, frmt_str)
    return calendar.timegm(ts)


def check_args_and_options(config_file, out_file, options, logg):
    """ Make's sure the input arguments and options are valid (exits if failure)

    Args:
        config_file : string path to config file
        out_file : string path to output file
        options : options object
        logg : log object
    
    Returns:
        exit_status : 0 = success
    """

    # Check config file
    ret = file_exists(config_file, logg)
    if ret == -1:
        sys.exit(-1)

    # Check options
    if options.site_list:
        if not os.path.exists(options.site_list):
            logg.write_time("Error: %s doesn't exist.  Exiting\n" % options.site_list)
            sys.exit(-1)
        if options.illef:
            logg.write_time("Error: Can't pass site list and id_lat_lon_elev_list,"
                            "one or the other\n")
            sys.exit(-1)
    if options.lat or options.lon or options.elevation:
        if options.site_list or options.illef:
            logg.write_time("Error: Can't pass site list and lat_lon info."
                            "  One or the other\n")
            sys.exit(-1)
        if options.siteDim:
            logg.write_time("Error: If you are providing a lat/lon, you "
                            "shouldn't be passing a siteDimension.  "
                            "-o applies to every site in the file\n")
            sys.exit(-1)
    if options.elevation:
        if options.site_list or options.illef:
            logg.write_time("Error: Can't pass site list and elevation info."
                            "  One or the other\n")
            sys.exit(-1)
        if options.siteDim:
            logg.write_time("Error: If you are providing an elevation, you "
                            "shouldn't be passing a siteDimension.  "
                            "-o applies to every site in the file\n")
            sys.exit(-1)
    lats = False
    lons = False
    elevs = False
    if options.lat:
        lats = options.lat.split(",")
    if options.lon:
        lons = options.lon.split(",")
    if options.elevation:
        elevs = options.elevation.split(",")
    if lats != False and lons != False:
        if len(lats) != len(lons):
            logg.write_time("Error: Must pass an equal amount of latitudes and longitudes\n")
            sys.exit(-1)
    if lats != False and elevs != False:
        if len(lats) != len(elevs):
            logg.write_time("Error: Must pass an equal amount of latitudes and elevations\n")
            sys.exit(-1)
    if lons != False and elevs != False:
        if len(lons) != len(elevs):
            logg.write_time("Error: Must pass an equal amount of longitudes and elevations\n")
            sys.exit(-1)
    if options.station_name:
        if options.site_list or options.illef:
            logg.write_time("Error: Can't pass station_name and a site list."
                            "  One or the other\n")
            sys.exit(-1)
        if options.siteDim:
            logg.write_time("Error: If you're passing a hardcoded station_name, "
                            "you shouldn't be also passing a siteDimension.  "
                            "-S assumes no site is listed in the file.  Exiting\n")
            sys.exit(-1)
            
    if options.utc_offset and options.local_offset:
        logg.write_time("Error: Can't pass utc_offset AND local_offset,"
                        " one or the other\n")
        sys.exit(-1)
    if options.siteDim and options.station_name:
        logg.write_time("Error: Can't pass site_dim AND station_name.  "
                        "If file has a site field you shouldn't have to pass a station_name\n")
        sys.exit(-1)

    if options.utc_offset:
        if not options.utc_offset.replace('-','').isdigit():
            if options.utc_offset not in pytz.all_timezones:
                logg.write_time("Error: %s not a valid timezone.  Check "
                                "https://gist.github.com/heyalexej/8bf688fd67d7199be4a1682b3eec7568 for reference" % options.utc_offset)
                sys.exit(-1)

    if options.local_offset:
        if not options.local_offset.replace('-','').isdigit():
            if options.local_offset not in pytz.all_timezones:
                logg.write_time("Error: %s not a valid timezone.  Check "
                                "https://gist.github.com/heyalexej/8bf688fd67d7199be4a1682b3eec7568 for reference" % options.local_offset)
                sys.exit(-1)
                            

    return 0

def get_var_from_config(config_data, output_var_name):
    """ Gets the variable data from the config file

    Args:
        config_data : Dictionary containing configuration information
        output_var_name : variable name to get data for

    Returns:
       v : dictionary containing variable information else False if 'output_var_name' doesn't exist
    """
    for v in config_data['Variables']:
        if v['output_name'] == output_var_name:
            return v
    return False

def get_config_vars(config_data):
    """ Gets the input/output variable names from the config file

    Args:
        config_data : Dictionary containing configuration information

    Returns:
       in_vars : list of all input variable names from config_data
       out_vars : list of all output variable names from config_data
    """
    in_vars = []
    out_vars = []
    for var in config_data['Variables']:
        in_vars.append(var['input_name'])
        out_vars.append(var['output_name'])
    return (in_vars, out_vars)
                    

def write_example_config(conf_file, df):
    """ Creates a sample config file

    Args:
        conf_file : string path to output file to create
        df : Pandas DataFrame

    Returns:
    """
    config_data = {}
    config_data['Variables'] = []
    config_data['Global Attributes'] = [{"ATTRIBUTE_NAME" : "ATTRIBUTE_VALUE",
                                         "ATTRIBUTE_NAME2" : "ATTRIBUTE_VALUE2"}]
    for col in df.columns:
        var_d = {}
        var_d["input_name"] = col
        var_d["output_name"] = col
        var_d["Attributes"] = [{"long_name" : "", "units" : ""}]
        config_data['Variables'].append(var_d)
    
    with open(conf_file, 'w') as fp:
        json.dump(config_data, fp, indent=4)
        
    return

 
def main():
    usage_str = "%prog in_file config_file output_file"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
    parser.add_option("-C", "--config_example", dest="config_example", help="Write a sample "
                      "config to this file.  Will use the input file as a template")    
    parser.add_option("-c", "--cdl", dest="cdl", help="Use this cdl file (don't create one "
                      "from atributes in config file)")
    parser.add_option("-f", "--siteDimension", dest="siteDim", help="Use this field(from the"
                      " input file) as the site dimension")
    parser.add_option("-F", "--input_fill_value", dest="input_fill", help="Set this as the fill value on input data.  Used if passing cdl and the fillValue in output NC is different from what is in the input file")
    parser.add_option("-t", "--utc_offset", dest="utc_offset", help="time offset(hours or timezone) to "
                      "utc (assumes file time is Local)")
    parser.add_option("-L", "--local_offset", dest="local_offset", help="time offset (hours or timezone)"
                      "to local time (adds a local_time variable, if no cdl is passed)")    
    parser.add_option("-d", "--date_format", dest="date_format", help="format of the date "
                      "field (if not epoch seconds)")    
    parser.add_option("-s", "--site_list", dest="site_list", help="Use this site list file "
                      "for lat/lon/elev info")
    parser.add_option("-i", "--id_lat_lon_elev_list", dest="illef", help="file with header "
                      "'site,lat,lon,elev', containing info for each site in the input file")
    parser.add_option("-S", "--station_name", dest="station_name", help="Write this as the "
                      "station name (adds the variable if cdl not passed)"
                      " (assumed to be a string)")
    parser.add_option("-a", "--lat", dest="lat", help="comma separated list of latitudes to write")
    parser.add_option("-o", "--lon", dest="lon", help="comma separated list of longitudes to write")
    parser.add_option("-e", "--elevation", dest="elevation", help="comma separated list of elevations to write")
    parser.add_option("-N", "--unused_cols", dest="unused_cols", action="store_true",
                      help="Print out a listing of the input columns that weren't used")
    parser.add_option("-m", "--delimiter", dest="delimiter", help="use this as the input file delimiter (default is comma)")
        
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
    config_file = args[1]
    out_file = args[2]

    # Make sure input file exists before attemping to read it
    if not os.path.exists(in_file):
        logg.write_time("Error: %s doesn't exist\n" % in_file)
        sys.exit(-1)

    logg.write_time("Reading: %s\n" % in_file)
    # Read the input file
    if options.delimiter:
        if options.delimiter == "whitespace":
            df = pd.read_csv(in_file, encoding = "ISO-8859-1", delim_whitespace=True, index_col=False)
        else:
            df = pd.read_csv(in_file, encoding = "ISO-8859-1", delimiter=options.delimiter, index_col=False)            
    else:
        df = pd.read_csv(in_file, encoding = "ISO-8859-1", index_col=False)

    if options.config_example:
        logg.write_time("Writing: %s\n" % options.config_example)
        write_example_config(options.config_example, df)
        logg.write_ending()
        sys.exit()         

                    
    logg.write_starting()

    # Check the arguments
    check_args_and_options(config_file, out_file, options, logg)
    
    # Run the code
    process(df, in_file, config_file, out_file, options, logg)

    logg.write_ending()

if __name__ == "__main__":
    main()
