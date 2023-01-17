#!/usr/bin/env python

"""run_ghi_fcst.py

   Runs C++ application ghi_fcst in real-time or archive modes  
"""

import log, log_msg
import os
import time
from optparse import OptionParser
import sys
import NYSC_sys_path as sys_path
import subprocess
import os
import errno
import tim
import glob
from datetime import datetime
import calendar

def mkdir_path(path, logg):
    """
    Excecute system command to make directory path
    
    Args:
      path : directory path to be created
      logg : logg file object

    Returns:
      none
    """

    try:
        os.makedirs(path)
    except os.error as e:
        if e.errno != errno.EEXIST:
            logg.write_error("Failure to make directory %s" % path)
            raise

def run_cmd(cmd, logg):
    """
    Use subprocess object to exec a system command.

    Args:    
      cmd : command string to be executed
     logg : logg file object

    Returns:
      err_code : zero on success,  fatal on error.
    """

    logg.write_info(cmd)
    p = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE,
                        stdout=subprocess.PIPE)
    (stdout_str, stderr_str) = p.communicate()
    ret = p.returncode
    
    if ret != 0: 
        logg.write_error("failed executing: %s" % cmd)
        logg.write_ending(ret)
        return 1 
    
    else: 
       return 0

def get_obs_run_times( options, cf, logg, obs_times_list):        
    """
    Get a list of application trigger times based on observation times. In archive mode
    get a list of trigger times, in real-time mode return the latest expected observation time
    
    Args: 
      options: program options object
      cf : configuration file for system directory and filenames and  path
           locations 
      logg : logg file object
      obs_times_list: list of unix times corresponding to observation files

    Returns:
      err_code : zero on success,  fatal on error.
    """
 
    if options.date:
        #
        # Processing single time
        #
        singleTime = tim.date2sec(options.date)
   
        obs_times_list.append(singleTime) 

    elif options.begin_interval:
        #  
        # Processing interval of dates, ghi_fcst will run for every file available
        # between the start and end dates
        #
        if not options.end_interval:
            logg.write_error("Need end date of interval to process an interval. "
                             "Check command line")
            return
        
        start_date_utc = tim.date2sec(options.begin_interval)
        
        end_date_utc = tim.date2sec(options.end_interval)

        obs_delta_seconds = int(options.obs_delta)*60 

        for i in range(start_date_utc, end_date_utc + obs_delta_seconds, obs_delta_seconds):
            hr_str = time.strftime("%H", time.gmtime(i))
            #print(hr_str)
            #
            # Process only hours for NYS for which we generate WRF-Solar
            #
            if (hr_str in ["11","12","13","14","15","16","17","18","19"]):
                #print ("appending ", i )
                obs_times_list.append(i) 
    else:
        #
        # This is realtime, get the current time
        # process time
        #
        ptime = time.time()
        
        #
        #  obs come in this temporal resolution 
        # 
        obs_delta_seconds = int(options.obs_delta)*60
      
        #
        # Subtract off seconds since last obs gen time 
        #
        otime =  ptime - (ptime % obs_delta_seconds)
        
        obs_times_list.append(otime) 
       

def  get_model_file(unix_obs_time, cf, options, logg):

    """
    Get model file relevant to unix_obs_time
    Method will try to get the file of the same gen hour as the observation.
      
    Args:
      unix_obs_time: unix time of observation data 
      cf : configuration file for system directory and filenames and  path
         locations 

    Returns:
      forecast file or empty string
  
    """
    obs_time = unix_obs_time
    
    #
    # Create a time string from the unix time. Make path parts
    # from the time.
    #
    obs_time_str = time.strftime("%Y%m%d%H%M", time.gmtime(obs_time))

    #
    # Create file path from config and time string components
    # 
    gen_hour_str = obs_time_str[8:10] 

    yyyymmdd = obs_time_str[0:8]

    date_hour = yyyymmdd + "." + gen_hour_str

    glob_base = "{0}.{1}*".format(cf.Nwp_filename_base, date_hour)

    path_components = [cf.Nwp_fcst_dir, yyyymmdd, glob_base]
    
    glob_path = os.path.join(*path_components)
    
    #
    # Get the available files
    #
    file_list = glob.glob(glob_path)

    #
    # Just one per hour for this model so there should only be 1 
    #
    if (len(file_list) > 1 or len(file_list) == 0):
       
       # write error message
   
       logg.write_info("File not found: %s" % glob_path)

       return ""

    else: 
       return file_list[0] 

def make_log_option_str(options, cf, model_base_name, date_str, genHour,  logg):
    """
    Make optional log string for application command line
    
    Args:
      options : program options object
      cf : configuration file for system directory and file name path
           locations
      model_base_name: Used in output directory structure 
      date_str: yyyymmdd string 
      genHour: genHour string(including mins HHMM) of forecast time
      logg: logg file object

    Returns:
      log option string
    """

    log_dir = os.path.join( cf.Log_dir, date_str)

    #
    # Make the path
    #
    mkdir_path(log_dir, logg)

    #
    # Add file basename
    #
    log_base_name = model_base_name + ".gt" + genHour

    log_path = os.path.join(log_dir, log_base_name)

    log_option_string = " -l %s " %  log_path

    logg.write_info("log_option_string: %s" % log_option_string)

    return log_option_string
        
def make_debug_option_string(options, logg):
    """
    Make optional debug string for application command line
    
    Args:
      options : program options object
      logg: logg file object

    Returns:
      debug option string
    """

    if options.debug_level_str:
        debug_option_string = "-d " +  options.debug_level_str + " "
    else:
        debug_option_string = ""

    logg.write_info("debug_option_string: %s" % debug_option_string)
 
    return debug_option_string

def  get_obs_file(unix_obs_time, cf, logg):
    """
    Get observation file corresponding to unix_obs_time

    Args:
      unix_obs_time: Time corresponding to filename of observation file
      cf : configuration file for system directory and file name path
          locations
      logg: logg file object

    Returns:
      Observation file path or empty string if not found
    """
    #
    # Create date string
    #
    obs_date = time.strftime("%Y%m%d", time.gmtime(unix_obs_time))
              
    #
    # Create the obs file name
    #
    obs_date_hour_min =  time.strftime("%Y%m%d.%H%M", time.gmtime(unix_obs_time))

    obs_file = cf.Obs_filename_base + "." + obs_date_hour_min  + ".nc"
    
    #
    # Create obs file path
    #
    obs_file_path_components = [cf.Obs_dir, obs_date, obs_file]

    obs_file = os.path.join(*obs_file_path_components)

    #
    # Return file path if it exists or an error
    #
    if os.path.exists(obs_file):
        logg.write_info("obs_file: %s" % obs_file)
        return obs_file
    else:
        logg.write_info("WARNING obs_file: %s not found" % obs_file)
        return ""

def  make_nwp_option_str(options, cf, unix_obs_time, logg):

    """
    ghi_fcst uses NWP data. Create appropriate options string 
    
    Args:
      options : program options object
      cf : configuration file for system directory and file name path
           locations
      unix_obs_trigger_time:  We will make a list NWP files needed for processing 
                              using the unix_obs_time as a  trigger time
      logg: logg file object

    Returns:
      options string for ghi_fcst application
    """
   
    #
    # Get the previous two hours of nwp files
    # 
    file1 = get_model_file(unix_obs_time, cf, options, logg)

    file2 = get_model_file(unix_obs_time - 3600, cf, options, logg)

    #
    # Make a comma separated list of files
    #
    nwp_option = "-m "
    
    if file1 != "" and file2 != "":
       nwp_option = nwp_option + file1 + "," + file2
   
    elif file1 != "" :
       nwp_option = nwp_option + file1
  
    elif file2 != "":
       nwp_option = nwp_option + file2
 
    else:
       logg.write_warning("NWP data is missing for unix processing time %d." % unix_obs_time)

    #
    # Return option string
    #
    return nwp_option


def make_obs_option_str(options, cf, unix_obs_trigger_time, logg):

    """
    ghi_fcst uses observations. Create appropriate
    options string depending 
    
    Args:
      options : program options object
      cf : configuration file for system directory and file name path
           locations
      unix_obs_trigger_time:  We will make a list observation files needed for processing 
                              using this trigger time
      logg: logg file object

    Returns:
      options string for ghi_fcst application
    """

    obs_base_dir = cf.Obs_dir
    
    #
    # assume 60minute obs lookback in real-time and create list of times
    #
    obs_times_list = []
    for i in range(int(unix_obs_trigger_time) - 3600, 
                   int(unix_obs_trigger_time) + int(options.obs_delta)*60, 
                   int(options.obs_delta)*60):  
        obs_times_list.append(i) 
   
    #
    # Make a comma separated list of files corresponding to times in list
    #
    obs_option = "-o "
    for oTime in obs_times_list:
        
        obs_file = get_obs_file(oTime, cf, logg)
        if obs_file != "":
            if oTime != obs_times_list[-1]:
                obs_option = obs_option + obs_file + ","
            else:
                obs_option = obs_option + obs_file
        else:
            logg.write_warning("Obs data will be missing for %d." % oTime)

    #
    # Remove trailing "," from comma separated list 
    #
    if obs_option[-1] == ",":
       obs_option = obs_option[0:-1] 

    #
    # Return files
    #
    return obs_option

def make_app_cdl_path( cf, logg):
    """
    Make application netcdf cdl file path
    
    Args:
      cf : configuration file for system directory and file name path
           locations
      logg: logg file object
    
    Returns:
      cdl file path string
    """

    #
    # Create filename
    #
    cdl_name =  "ghi_fcst.cdl"

    #
    # Create filepath
    #
    cdl_path = os.path.join(cf.cdl_dir, cdl_name)

    logg.write_info("cdl_path: %s" % cdl_path)

    return cdl_path


def make_outdir(cf, date_str, logg):
    """
    Create and make output directory path:
    fcstDir/netcdf/yyyymmdd
    eg. <Fcst_dir>/netcdf/yyyymmdd
    
    Args:
      cf: configuration file for system directory and file name path
          locations
      date_str: date string for ouput in dated subdirectory
      logg: logg file object

    Returns:
      output directory path
    """
   
    #
    # Make a list of the components of the output directory  
    #  
    fcst_dir_components = [cf.Statcast_fcst_dir, "netcdf", date_str]
  
    #
    # Create the path string
    # 
    fcst_dir =  os.path.join(*fcst_dir_components)
   
    #
    # Make the path
    #
    mkdir_path(fcst_dir, logg)
    
    logg.write_info("fcst_dir: %s" % fcst_dir)
    
    return fcst_dir

def ghi_forecast_obs_trigger(options, cf, logg, num_fcsts, model_base_name, unix_obs_time):
    """
    Create the C++ command string and execute
    Args:
      options: options for this script specified on command line
      cf: configuration file for system directory and file name path
          locations
      logg: logg file object
      num_fcsts: number of forecasts that will be created
      model_base_name: base of the Cubist model (base of *.names, *.model)
      unix_obs_time: trigger time for the forecast based on observation data

    Returns:
      returns 0 on success, 1 on failure 
    """


    nwp_option_str = make_nwp_option_str(options, cf, unix_obs_time, logg)
 
    if nwp_option_str == "-m ":

       #
       # We need the NWP data, if there are no forecasts, exit
       #
       logg.write_error("No NWP data in past 2 hours")

       return 1

    #    
    # Create observation string. Get at least an hour and half of previous observaiont
    # 
    obs_option_str = make_obs_option_str(options, cf, unix_obs_time, logg) 

    if obs_option_str == "-o ":
        
       #
       # This is an observation based forecast, if there are no observations, exit
       #
       logg.write_error("No observation data in past 60 minutes")

       return 1 
    
    # 
    # Construct Cubist model path 
    #
    model_dir = os.path.join(cf.Cubist_model_dir)

    model = os.path.join(model_dir, model_base_name)

    logg.write_info("cubist model base name: %s" % model)

    #
    # Construct obs trigger option string
    #
    time_trigger_str = " -t {0} ".format(unix_obs_time) 

    #
    # Construct date string for directory paths
    #
    yyyymmdd =  time.strftime("%Y%m%d", time.gmtime(unix_obs_time))

    genHour = time.strftime("%H%M", time.gmtime(unix_obs_time)) 
    
    #
    # Construct option string for ghi_fcst application logging
    #
    log_option_str = make_log_option_str(options, cf, model_base_name, yyyymmdd, genHour, logg)
  
    # 
    # Construct application debug option string if debug option included
    # 
    debug_option_str = make_debug_option_string(options, logg)

    #
    # Construct siteID path string
    #
    site_id_path = os.path.join(cf.config_dir, cf.SiteId_file_name_base)

    #
    # Construct input cdl filename argument
    # 
    cdl_path = make_app_cdl_path(cf, logg)

    #
    # Construct and make the output directory
    #
    fcst_dir = make_outdir(cf, yyyymmdd, logg)
    
    # 
    # Make options string for single forecast at specific lead
    #
    single_fcst_str = ""
    if options.single_fcst_lead is not None:
       single_fcst_str = "-s " + options.single_fcst_lead + " "

    #
    # Create application command string 
    # 
    cmd = "~/bin/ghi_fcst %s %s %s %s %s %s %s %s %s %s %s %s" % (log_option_str, 
                                                                  debug_option_str,  
                                                                  single_fcst_str,
                                                                  time_trigger_str,
                                                                  nwp_option_str,
                                                                  obs_option_str,
                                                                  options.leads_delta,
                                                                  num_fcsts,
                                                                  site_id_path,
                                                                  model, 
                                                                  cdl_path,
                                                                  fcst_dir)
    #
    # Run the command
    #
    logg.write_info("Running: %s" % cmd)
    
    ret = run_cmd(cmd, logg)

    return ret 
   
def main():
    """
    Define and record user options and arguments.
    Setup application logging
    Get C++ forecast applicaton trigger times from observations data.
    Run the ghi_fcst application based on the trigger times
 
    Args:
       num_fcsts: Number of forecasts to create
       model_basename_str: Basebame of the cubist machine learning model
      
    Returns:
      none
    """


    parser = OptionParser(usage = "%prog [options] num_fcsts(to process) model_base_name ")

    parser.add_option("-b", "--begin_interval", dest="begin_interval",help="run ghi_fcst for all "
                      "observation files in interval [begin_interval, end_interval] where "
                      "begin_interval and end_interval have the format yyyymmdd")
    parser.add_option("-d", "--date", dest="date",help="run ghi_fcst one time using the "
                      "specified date and time of observation file in the form yyyymmddHHMM")
    parser.add_option("-e", "--end_interval", dest="end_interval",help="run ghi_fcst for all "
                      "observation files in interval [begin_interval, end_interval] where "
                      "begin_interval and end interval have the format yyyymmdd")
    parser.add_option("-g", "--debug", dest="debug_level_str", help="debug level "
                      "for ghi_fcst application")
    parser.add_option("-l", "--log", dest="log", help="base name of log file for "
                      "this script")
    parser.add_option("-s", "--single_fcst_lead",dest="single_fcst_lead", 
                      help="run ghi_fcst for single lead time in minutes")
    parser.add_option("-t", "--obs_time_delta",dest="obs_delta",  default=15,
                      help="time resolution of observations in minutes")
    parser.add_option("-r", "--fcst_lead_resolution",dest="leads_delta",  default=15,
                      help="time resolution of forecasts in minutes") 
    
    (options,args) = parser.parse_args()
    
    if len(args) < 2:
        parser.print_help()
        sys.exit()

    #
    # number of forecasts to process
    #
    num_fcsts = args[0]

    # 
    # cubist model base string  
    #
    model_basename_str = args[1]
   
    # 
    # Set system path configuration
    # 
    cf = sys_path

    #
    # Instantiate a log message object
    #
    if options.log:
        logg = log_msg.LogMessage(options.log, "asc")
    else:
        logg = log_msg.LogMessage("")

    #
    # Begin processing
    # 
    logg.write_starting("Beginning ghi_fcst.py")

    #
    # Get list of observations times
    #
    obs_time_list = []

    get_obs_run_times(options, cf, logg, obs_time_list)
       
    #
    # Process each observation time
    #
    ret = 0
    for unix_time in obs_time_list:
       
       t =  time.strftime("%Y%m%d %H:%M:%S", time.gmtime(unix_time))

       logg.write_info("Running ghi_fcst: Processing observation time %s: %s" % (unix_time, t))          

       ret = ghi_forecast_obs_trigger(options, cf, logg, num_fcsts, model_basename_str, unix_time) 

       if (ret == 0): 
          logg.write_info(msg="Ending ghi_fcst processing of observation date and time %s, ret = %d" % (t, ret))
       else:
          logg.write_time(msg="Error: ghi_fcst processing of observation date and time %s failed, ret = %d\n" % (t, ret))
    logg.write_ending(ret, msg="Ending ghi_fcst.py")
    sys.exit(ret)

if __name__ == "__main__":
    main()
