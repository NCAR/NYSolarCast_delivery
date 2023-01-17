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
THIS WAS DESIGNED TO RERUN THE NYSolarCast system
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import pandas as pd
import NYSC_sys_path as sys_path

FARMS = sys_path.FARMS

def main():
    usage_str = "%prog start_date(YYYYmmdd.HHMM) end_date(YYYYmmdd.HHMM)"
    parser = OptionParser(usage = usage_str)
        
    (options, args) = parser.parse_args()
    
    if len(args) < 2:
        parser.print_help()
        sys.exit()

    start_dt = pd.to_datetime(args[0], format="%Y%m%d.%H%M")
    end_dt = pd.to_datetime(args[1], format="%Y%m%d.%H%M")

    logg_base = "%s/rerun_NYSC_system" % (sys_path.Log_dir)
    logg = log_msg.LogMessage(logg_base)
    logg.set_suffix(".pyl")

    logg.write_starting()

    # Create the range of dates over which to rerun the system
    date_range = pd.date_range(start_dt.replace(hour=0, minute=0), end_dt)

    for date in date_range:
        if date == date_range[-1]:
            # This is end date, set hour/minute to end date hour/minute
            end_date = date.replace(hour=end_dt.hour, minute=end_dt.minute)
            date = date.replace(hour=0, minute=0)
        elif date == date_range[0]:
            # This is the start date, set start hour/minute to the start hour/minute
            date = date.replace(hour=start_dt.hour, minute=start_dt.minute)
            end_date = (date + pd.Timedelta(days=1)).replace(hour=0, minute=0)
        else:
            # This isn't the starting date, so set the hour to be 00:00
            date = date.replace(hour=0, minute=0)
            end_date = date + pd.Timedelta(days=1)


        if end_date == date:
            continue
        start_str = date.strftime("%Y%m%d%H%M")
        end_str = end_date.strftime("%Y%m%d%H%M")
        

        """
        # 1. download the epri obs
        cmd = "~/epri_scripts/python/run_download_nymeso_data.py -s %s -e %s" % (start_str, end_str)
        ret = run_cmd(cmd, logg)
        
        # 2. Perform obs qc
        cmd = "~/epri_scripts/python/run_nymeso_basic_and_shading_qc.py -s %s -e %s" % (start_str,
                                                                                        end_str)
        ret = run_cmd(cmd, logg)

        # 3. Convert to 15min
        cmd = "~/epri_scripts/python/run_nymeso_15min_avg.py -s %s -e %s" % (start_str, end_str)
        ret = run_cmd(cmd, logg)

        # 4. Convert to nc
        cmd = "~/epri_scripts/python/run_nymeso_csv_to_nc.py -s %s -e %s" % (start_str, end_str)
        ret = run_cmd(cmd, logg)

        # 5. Upload to sftp
        cmd = "~/epri_scripts/python/upload_obs_to_sftp.py -s %s -e %s" % (start_str, end_str)
        ret = run_cmd(cmd, logg)

        # 6. Convert the HRRR forecast to netCDF
        cmd = "~/epri_scripts/python/run_hrrr_grib2nc.py -s %s -e %s" % (start_str,
                                                                         end_str)        
        ret = run_cmd(cmd, logg)
        
        """
        # 7. Convert wrf fcst to statcast netCDF
        cmd = "~/epri_scripts/python/run_WrfNetCDF2StatcastNetCDF.py -s %s -e %s" % (start_str,
                                                                                     end_str)
        ret = run_cmd(cmd, logg)


        # 8. Blend the WRF and HRRR forecasts
        cmd = "~/epri_scripts/python/run_NWP_grid_blender.py -s %s -e %s" % (start_str,
                                                                             end_str)        
        ret = run_cmd(cmd, logg)

        # 9. Run the statcast GHI forecast
        cmd = "~/epri_scripts/python/run_ghi_fcst.py -b %s -e %s" % (start_str[:8],
                                                                     end_str[:8])        
        ret = run_cmd(cmd, logg)

        # 10.  Blend Statcast with the NWP blended forecast
        cmd = "~/epri_scripts/python/run_NWP_Statcast_blender.py -s %s -e %s" % (start_str,
                                                                                 end_str)
        ret = run_cmd(cmd, logg)
        
        # 11. Upload the blended forecast to sftp
        cmd = "~/epri_scripts/python/upload_blended_fcst_to_sftp.py -s %s -e %s" % (start_str,
                                                                                    end_str)
        ret = run_cmd(cmd, logg)

        # 12.  Upload the day-ahead forecast to sftp
        cmd = "~/epri_scripts/python/upload_day_ahead_fcst_to_sftp.py -s %s0600 -e %s0600" % (start_str[:8],
                                                                                              end_str[:8])
        ret = run_cmd(cmd, logg)


        # 13. Run the distributed power forecasts
        cmd = "~/epri_scripts/python/run_power_fcst.py distributed 60 -b %s -e %s" % (start_str[:8],
                                                                                      end_str[:8])
        ret = run_cmd(cmd, logg)

        # 14.  Run the site power forecasts
        for site in FARMS:
            cmd = "~/epri_scripts/python/run_power_fcst.py %s 15 -b %s -e %s" % (site, 
                                                                                 start_str[:8],
                                                                                 end_str[:8])
            ret = run_cmd(cmd, logg)

           
        
        # 15.  Repeat 13,14 for the day-ahead pieces
        # Run the distributed power forecasts
        cmd = "~/epri_scripts/python/run_power_fcst_day_ahead.py distributed 60 -b %s -e %s" % (start_str[:8],
                                                                                                end_str[:8])
        ret = run_cmd(cmd, logg)
    
        # Run the site power forecasts
        for site in FARMS:
            cmd = "~/epri_scripts/python/run_power_fcst_day_ahead.py %s 15 -b %s -e %s" % (site, 
                                                                                           start_str[:8],
                                                                                           end_str[:8])
            ret = run_cmd(cmd, logg)
        

        # Convert the percent power forecasts to total power
        cmd = "~/epri_scripts/python/run_pct_power_to_total_power.py -s %s -e %s" % (start_str,
                                                                                     end_str)
        ret = run_cmd(cmd, logg)
            
    
        # 16. Run the pct power rollup
        cmd = "~/epri_scripts/python/run_pct_power_rollup.py -s %s -e %s" % (start_str,
                                                                             end_str)
        ret = run_cmd(cmd, logg)


        # 17.  Upload the near term power forecasts to sftp
        #cmd = "~/epri_scripts/python/upload_power_fcsts_to_sftp.py -s %s -e %s" % (start_str,
        #                                                                           end_str)
        cmd = "~/epri_scripts/python/upload_power_fcsts_to_sftp.overwrite.py -s %s -e %s" % (start_str,
                                                                                             end_str)
        ret = run_cmd(cmd, logg)


        # 18.  Upload the day ahead power forecasts to sftp
        cmd = "~/epri_scripts/python/upload_day_ahead_power_fcsts_to_sftp.py -s %s0600 -e %s0600" % (start_str[:8],
                                                                                                     end_str[:8])
        ret = run_cmd(cmd, logg)

        # 19.  Upload the near term total power forecasts to sftp
        #cmd = "~/epri_scripts/python/upload_total_power_fcsts_to_sftp.py -s %s -e %s" % (start_str,
        #                                                                                 end_str)
        cmd = "~/epri_scripts/python/upload_total_power_fcsts_to_sftp.overwrite.py -s %s -e %s" % (start_str,
                                                                                                   end_str)
        ret = run_cmd(cmd, logg)

        # 20.  Upload the day ahead total power forecasts to sftp
        cmd = "~/epri_scripts/python/upload_day_ahead_total_power_fcsts_to_sftp.py -s %s0600 -e %s0600" % (start_str[:8],
                                                                                                           end_str[:8])
        ret = run_cmd(cmd, logg)


        logg.write_time("\n")

    logg.write_ending()
    
def run_cmd(cmd, logg):
    """
    """
    logg.write_time("Executing: %s\n" % cmd)
    #ret = 0
    ret = os.system(cmd)
    logg.write_time("Ret: %d\n" % ret)
    return ret
    


if __name__ == "__main__":
    main()
