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
This scipt runs the power forecast. It is a wrapper for power_fcst.py.
"""

import time, os, sys
import NYSC_sys_path as sys_path
from datetime import datetime
from optparse import OptionParser
import log_msg
import errno
import pandas as pd

def mkdir_path(path):
    """
    Excecute system command to make directory path

    Args:
        path : directory path to be created
    """

    try:
        os.makedirs(path)
    except os.error as e:
        if e.errno != errno.EEXIST:
            print("Failure to make directory %s" % path)
            raise


def main():
   """
   This is a wrapper script for power_fcst.py. It does the following:
   1) sets up and performs logging 
   2) defines run time mode depending on user specified options: time interval, 
     single date, or real-time (no time options), 
   3) passes in arguments to power_fcst.py for this project:
     number of forecasts to be computed(6 or 24), the forecast time resolution(15 min
     or 60min), the time resolution of the input data (15min or 60min), 
     C++ application debug level ( 4 = high)
   4) executes power_fcst.py, logs result
   
   Args:
       model_base: basename of the statistical model used for forecasting
       model_lead_resolution: Either 15 or 60
   Returns:
       none
   """
   
   usage_str = "%prog modelBase modelResolution(60,15)"
   parser = OptionParser(usage = usage_str)
   
   #
   # Run options for ghi_fcst.py :  -b and -e for begin and end interval
   # and -d for a particular time
   #
   parser.add_option("-b", "--begin_date", dest="begin_date", help="yyyymmdd")
   parser.add_option("-e", "--end_date", dest="end_date", help="yyyymmdd")
   parser.add_option("-d", "--date", dest="date", help="yyyymmddhhmm")
   
   (options, args) = parser.parse_args()
   
   if len(args) < 2:
      parser.print_help()
      sys.exit(2)
      
   model_base = args[0]

   model_lead_resolution = args[1]

   # 
   # Create log structure. Dated directory is created corresponding to 
   # time processed.
   # 
   date_str_option = ""

   log_date_str = ""
   
   #
   # If processing interval, or processing single time set those run options 
   # and the sppropriate log directory date
   #
   if options.begin_date and options.end_date:
       date_str_option = "-b %s -e %s" % (options.begin_date, options.end_date)
       log_date_str = options.begin_date
   elif options.date:
       date_str_option = "-d %s" % options.date
       log_date_str =  options.date [0:8] 
   else:
       # Added to handle latency in the real time system
       ptime = time.time()
       # Subtract a day (Timing not important and the WRF files are often late)
       current_time = ptime - (ptime % 900) - 86400
       date_str_option = "-d %s" % pd.to_datetime(current_time, unit='s').strftime("%Y%m%d%H%M")
       
      
   #
   # If processing real-time set the date to current date
   #
   if ( log_date_str == ""):
      log_date_str = time.strftime("%Y%m%d", time.gmtime(time.time()))
     
   logDir = sys_path.Log_dir + "/" + log_date_str 

   mkdir_path(logDir)

   #
   # Configure the logg object
   # 
   logg_base = "%s/power_fcst_%s" % (logDir, model_base) 
   
   logg = log_msg.LogMessage(logg_base)

   logg.set_suffix(".pyl")

   #
   # Execution of power_fcst.py
   #
   logg.write_starting()
   
   proc_script = os.path.join(sys_path.scripts_dir, "power_fcst.py")

   if model_lead_resolution == "15":
      cmd = "%s %s -l %s -t 15 -r 15 24 %s " % (proc_script,  date_str_option, logg_base, model_base)
   else:    
      cmd = "%s %s -l %s 6 %s " % (proc_script,  date_str_option, logg_base, model_base)
   #print(cmd)

   logg.write_time("Executing: %s\n" % cmd)

   ret = os.system(cmd)
   if ret == 0:
      logg.write_time("Success: %d\n" % ret)
   else:
      logg.write_time("Error: Ret: %d\n" % ret)
    

   logg.write_ending(ret)

if __name__ == "__main__":
    main()
