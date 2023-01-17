#!/usr/bin/env python

"""power_fcst.py

   Runs C++ application pct_power_fcst in real-time or archive modes  
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
      err_code: zero on success,  fatal on error.
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

def get_run_times( options, cf, logg, times_list):        
    """
    Get a list of application trigger times based on blended model times. In archive mode
    get a list of trigger times, in real-time mode return the latest expected data time
    
    Args:
      options: program options object
      cf : configuration file for system directory and filenames and  path
           locations 
      logg : logg file object
      times_list: list of unix times corresponding to observation files

    Returns:
      err_code : zero on success,  fatal on error.
    """
 
    if options.date:
        #
        # Processing single time
        #
        singleTime = tim.date2sec(options.date)
   
        times_list.append(singleTime) 

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
        #print(options.end_interval) 
        end_date_utc = tim.date2sec(options.end_interval)

        data_delta_seconds = int(options.data_delta)*60 

        for i in range(start_date_utc, end_date_utc + data_delta_seconds, data_delta_seconds):
            hr_str = time.strftime("%H", time.gmtime(i)) 
            #print(hr_str)
            #
            # Process only hours for NYS for which we generate blended model data 
            #
            if (hr_str in ["11","12","13","14","15","16","17","18","19"]):
                #print ("appending ", i )
                times_list.append(i) 
    else:
        #
        # This is realtime, get the current time
        # process time
        #
        #print ( time.time())
        d = datetime.utcnow()
        ptime = calendar.timegm(d.utctimetuple())
        # cant be used with local time on ncar supercomputer
        #ptime = time.time()

        gmtime = time.gmtime(ptime)
        
        #
        #  data come in this temporal resolution 
        # 
        data_delta_seconds = int(options.data_delta)*60
      
        #
        # Subtract off seconds since last data time 
        #
        dtime =  ptime - (ptime % data_delta_seconds)
        
        times_list.append(dtime) 
       

def make_log_option_str(options, cf, model_base_name, date_str, genHour,  logg):
    """
    Make optional log string for application command line
    
    Args:
      options : program options object
      cf : configuration file for system directory and file name path
           locations
      model_base_name: Used in output directory structure 
      genHour: genHour ( including mins HHMM) of forecast time 
      date_str: yyyymmdd string 
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
    log_base_name = model_base_name+"_month" + "." + genHour

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

def  get_model_file(unix_time, cf, options, logg):

    """
    Get blended model file relevant to unix_time
    Method will try to get the file of the same gen hour as unix_time.
      
    Args:
      unix_time: unix time of processing 
      cf: configuration file for system directory and filenames and  path
          locations 

    Returns:
      blended forecast file or empty string
    """

    #
    # Create a time string from the unix time. Make path parts
    # from the time.
    #
    processing_time_str = time.strftime("%Y%m%d%H%M", time.gmtime(unix_time))

    #
    # Create blended file path from config and time string components
    # 
    gen_hour_min_str = processing_time_str[8:12]

    yyyymmdd = processing_time_str[0:8]

    date_hour_min = yyyymmdd + "." + gen_hour_min_str

    if options.data_delta == 60:
   
       glob_base = "{0}.{1}*".format("NWP_statcast_blend_hourly", date_hour_min)

       path_components = [cf.Input_fcst_dir_60min,yyyymmdd, glob_base]
    else:
       glob_base = "{0}.{1}*".format("NWP_statcast_blend", date_hour_min)

       path_components = [cf.Input_fcst_dir_15min,yyyymmdd, glob_base]

    glob_path = os.path.join(*path_components)

    #
    # Get the available files
    #
    file_list = glob.glob(glob_path)

    #
    #
    # Just one per hour for this model so there should only be 1 
    #
    if (len(file_list) > 1 or len(file_list) == 0):

       # write error message

       logg.write_info("File not found: %s" % glob_path)

       return ""

    else:
       return file_list[0]

def  make_model_option_str(options, cf, unix_time, logg):

    """
    pct_power_fcst uses NWP data. Create appropriate options string 

    Args:
      options : program options object
      cf : configuration file for system directory and file name path
           locations
      unix_time:  We will make a list NWP files needed for processing 
                  using the unix_time as a trigger time
      logg: logg file object

    Returns:
      options string for ghi_fcst application
    """
   
    #
    # Get the previous two blended model files
    # 
    file1 = get_model_file(unix_time, cf, options, logg)

    file2 = get_model_file(unix_time - int(options.data_delta)*60, cf, options, logg)

    #
    # Make a comma separated list of files
    #
    model_option = "-m "
   
    if file1 != "" and file2 != "":
       model_option = model_option + file1 + "," + file2
   
    elif file1 != "" :
       model_option = model_option + file1
  
    elif file2 != "":
       model_option = model_option + file2
 
    else:
       logg.write_warning("Blended model data is missing for unix processing time %d." % unix_time)

    #
    # Return option string
    #
    return model_option

def make_site_list_path( cf, model_base_name, logg):
    """
    Make application netcdf cdl file path
    
    Args:
      cf : configuration file for system directory and file name path
          locations
      logg: logg file object
    
    Returns:
      site list file path string
    """
    
    #
    # Create filename
    #
    site_file_name =  model_base_name + "_site_list.csv"

    #
    # Create filepath
    #
    site_path = os.path.join(cf.site_list_dir, site_file_name)

    logg.write_info("site list path: %s" % site_path)

    return site_path

def make_app_cdl_path( cf, model_base_name, logg):
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
    # For now -- one name, will see if one cdl file fits all
    cdl_name =  model_base_name + ".cdl"

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
    fcst_dir_components = [cf.Fcst_dir+"_month", "netcdf", date_str]
  
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

def pct_power_fcst(options, cf, logg, num_fcsts, model_base_name, unix_time):
    """
    Create the pct_power_fcst C++ command string and execute
    Args:
      options: options for this script specified on command line
      cf: configuration file for system directory and file name path
          locations
      logg: logg file object
      num_fcsts: number of forecasts that will be created
      model_base_name: base of the Cubist model (base of *.names, *.model)
      unix_time: trigger time for the forecast based on observation data

    Returns:
      returns 0 on success, 1 on failure 
    """

    model_option_str = make_model_option_str(options, cf, unix_time, logg)

    if model_option_str == "-m ":
       #
       # We need the NWP data, if there are no forecasts, exit
       #
       logg.write_error("No blended model data in past 2 hours")

       return 1

    # 
    # Construct Cubist model path 
    #
    #model_dir = os.path.join(cf.Cubist_model_dir+"_monthofyear")
    model_dir = os.path.join(cf.Cubist_model_dir)

    model = os.path.join(model_dir, model_base_name)

    logg.write_info("cubist model base name: %s" % model)

    #
    # Construct time trigger option string
    #
    time_trigger_str = " -t {0} ".format(unix_time) 

    #
    # Construct date string for directory paths
    #
    yyyymmdd =  time.strftime("%Y%m%d", time.gmtime(unix_time))

    genHour = time.strftime("%H%M", time.gmtime(unix_time)) 
    
    #
    # Construct option string for pct_power_fcst application logging
    #
    log_option_str = make_log_option_str(options, cf, model_base_name, yyyymmdd, genHour, logg)
  
    # 
    # Construct application debug option string if debug option included
    # 
    debug_option_str = make_debug_option_string(options, logg)

    #
    # Construct siteID path string 
    #
    site_id_path = make_site_list_path(cf, model_base_name,logg) 

    #
    # Construct input cdl filename argument
    # 
    cdl_path = make_app_cdl_path(cf, model_base_name,logg)

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
    cmd = "pct_power_fcst_month %s %s %s %s %s %s %s %s %s %s %s" % (log_option_str, 
                                                                     debug_option_str,  
                                                                     single_fcst_str,
                                                                     time_trigger_str,
                                                                     model_option_str,
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
    Run the pct_power_fcst application based on the trigger times
 
    Args:
       num_fcsts: Number of forecasts to create
       model_basename_str: Basebame of the cubist machine learning model
      
    Returns:
      none
    """


    parser = OptionParser(usage = "%prog [options] num_fcsts(to process) model_base_name ")

    parser.add_option("-b", "--begin_interval", dest="begin_interval",help="run pct_power_fcst for all "
                      "blended model files in interval [begin_interval, end_interval] where "
                      "begin_interval and end_interval have the format yyyymmdd")
    parser.add_option("-d", "--date", dest="date",help="run pct_power_fcst one time using the "
                      "specified date and time of dicast file in the form yyyymmddHHMM")
    parser.add_option("-e", "--end_interval", dest="end_interval",help="run pct_power_fcst for all "
                      "blended files in interval [begin_interval, end_interval] where "
                      "begin_interval and end interval have the format yyyymmdd")
    parser.add_option("-g", "--debug", dest="debug_level_str", help="debug level "
                      "for pct_power_fcst application")

    parser.add_option("-l", "--log", dest="log", help="base name of log file for "
                      "this script")
    parser.add_option("-s", "--single_fcst_lead",dest="single_fcst_lead", 
                      help="run ghi_fcst for single lead time in minutes")
    parser.add_option("-t", "--data_delta_time",dest="data_delta",  default=60,
                      help="time resolution of input_data in minutes(default 60)")
    parser.add_option("-r", "--fcst_lead_resolution (default 60)",dest="leads_delta",  
                      default=60, help="time resolution of forecasts in minutes") 
    
    (options,args) = parser.parse_args()
    
    if len(args) < 2:
        parser.print_help()
        sys.exit(2)

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
    logg.write_starting("Beginning power_fcst.py")

    #
    # Get list of run times
    #
    time_list = []

    get_run_times(options, cf, logg, time_list)

    #
    # Process each observation time
    #
    ret = 0
    for unix_time in time_list:
       
       t =  time.strftime("%Y%m%d %H:%M:%S", time.gmtime(unix_time))

       logg.write_info("Running pct_power_fcst: Processing time %s: %s" % (unix_time, t))          

       ret = pct_power_fcst(options, cf, logg, num_fcsts, model_basename_str, unix_time) 

       if (ret == 0): 
          logg.write_info(msg="Ending pct_power_fcst processing of date and time %s, ret = %d" % (t, ret))
       else:
          logg.write_time(msg="Error: pct_power_fcst processing of date and time %s failed, ret = %d\n" % (t, ret))

    logg.write_ending(ret, msg="Ending power_fcst.py")
    sys.exit(ret)

if __name__ == "__main__":
    main()
