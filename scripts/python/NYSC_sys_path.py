#!/usr/bin/env python

"""
THIS SCRIPT CONTAINS ENVIRONMENTAL VARIABLES REFERENED BY OTHER PYTHON SCRIPTS
"""

import os

Data_base_dir = "/glade/work/brummet/epri/data"
Log_dir = os.path.join(Data_base_dir, "log")
Lock_dir = os.path.join(Data_base_dir, "lock")
Tmp_dir = os.path.join(Data_base_dir, "tmp")

Static_data_dir = os.path.join(Data_base_dir, "static")
config_dir = os.path.join(Static_data_dir, "config")
site_list_dir = os.path.join(Static_data_dir, "site_list")
cdl_dir = os.path.join(Static_data_dir, "cdl")
Cubist_model_dir = os.path.join(Static_data_dir, "model")

WRF_fcst_dir = "/glade/p/ral/wsap/jaredlee/EPRI/Phase3/wrf/"
WRF_historic_fcst_dir = "/glade/campaign/ral/wsap/epri/Phase3/wrf/"


scripts_dir = "/glade/u/home/brummet/epri_scripts/python/"
#
# Stuff for statcast (ghi_fcst)
#
Nwp_filename_base = "wrf_nc"
Obs_filename_base = "nymeso_15min_obs"
SiteId_file_name_base = "stations.list"
Ghi_fcst_base = "ghi_fcst"             # forecast data

Statcast_fcst_dir = os.path.join(Data_base_dir, Ghi_fcst_base) 
Nwp_fcst_dir = os.path.join(Data_base_dir, "wrf_fcst_4_statcast")
Obs_dir = os.path.join(Data_base_dir, "phase3/NYMesonet_real_time/15min_nc")

#
# Percent power stuff
#
Input_fcst_dir_60min = os.path.join(Data_base_dir, "nwp_statcast_blend_hourly")
Input_fcst_dir_15min = os.path.join(Data_base_dir, "nwp_statcast_blend")
Input_day_ahead_fcst_dir_60min = os.path.join(Data_base_dir, "nwp_blend_hourly")
Input_day_ahead_fcst_dir_15min = os.path.join(Data_base_dir, "nwp_blend")
Fcst_dir = os.path.join(Data_base_dir, "pct_power_fcst")
Total_fcst_dir = os.path.join(Data_base_dir, "total_power_fcst")

# Filenames
Input_filename_base_60min = "NWP_statcast_blend_hourly"
Input_filename_base_15min = "NWP_statcast_blend"

# FARM NAMES
FARMS = ['Farm_A1', 'Farm_A2', 'Farm_A3', 'Farm_A4',
         'Farm_B1', 'Farm_B2', 'Farm_B3', 'Farm_B4', 'Farm_B5', 'Farm_B6']
