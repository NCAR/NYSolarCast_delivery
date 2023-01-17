#!/usr/bin/env python

"A class for logging messages in daily log files."

#/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
# * Copyright (c) 1995-2002 UCAR
# * University Corporation for Atmospheric Research(UCAR)
# * National Center for Atmospheric Research(NCAR)
# * Research Applications Program(RAP)
# * P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
# * All rights reserved. Licenced use only.
# * $Date: 2015/03/18 15:56:30 $
# *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


import os
import sys
import time


Log = "Log error: "
Log_error = "log.py module error: "

# The time string that comes at the beginning of timed log messages
LOG_TIME_STRING = "%H:%M:%S "

LOG_TIME_STRING_LEN = 9

# This must be set to the starting position of the hh field in LOG_TIME_STRING
LOG_HOUR_POS = 0 

def log_mod_time(file_base, timeval):
    """Find the modification time given the log file base name and the time of interest. Note that
    log files are created on a daily basis. See class Log for details.

    Args:
      file_base -- file base string
      timeval -- floating point time value

    Returns:
      Modification time of file as floating point time value.
    """
    
    try:
        ttup = time.gmtime(timeval)
        end = time.strftime("%Y%m%d", ttup)
        log_file = "%s.%s" % (file_base, end)
        t = os.path.getmtime(log_file)
    except:
        t = -1
        
    return t
    
def log_file_path(log_path, date_string, suffix):
    """
    Create log file path. The arg date_string must be in the form: yyyymmdd 

    Args:
      log_path: directory string
      date_string: date string in the form yyyymmdd
      suffix: suffix string

    Returns:
        string : full log file path
    """
    return("%s.%s%s" % (log_path, date_string, suffix))

class Log:
    """Class for logging messages in daily log files."""

    def __init__(self, path, suffix=""):
        """ Initialize class by specifying the log file path """
        self.log_path = path
        self.day = -1
        self.fp = None
        self.suffix = suffix

        # Add "." separator if suffix does not contain one
        if suffix:
            if suffix[0] != ".":
                self.suffix = ".%s" % suffix

    def close(self):
        """Explicitly closes any open log file. Called within the class, and only necessary publicly when more than one process might be writing to the same log file."""
        if self.fp != None:
            self.fp.close()

    def get_log_path(self):
        """ Return the path for the log file """
        return (self.log_path)
        
    #deprecated -- person shouldn't change suffix of log after initialization so this will not be supported
    def set_suffix(self, suffix):
        """ Set the file name suffix for the log file """
        if suffix[0] == ".":
            self.suffix = suffix
        else:
            self.suffix = ".%s" % suffix
        
    def write(self, msg):
        """ Write a message to the log file """
        curr_time = time.time()
        gmtime = time.gmtime(curr_time)
        self.write_log_msg(msg, gmtime, 0)
        
    def write_time(self, msg):
        """ Write a message to the log file and prepend the time."""

        curr_time = time.time()
        gmtime = time.gmtime(curr_time)
        self.write_log_msg(msg, gmtime, 1)
        
    def write_log_msg(self, msg, gmtime, inc_time):
        """
        Function that forms the basis for write() and write_time()

        Args:
          msg: string
          gmtime: a time structure of the type returned by time.time()
          inc_time: 1 for prepending time and 0 otherwise

        Returns:
          None
        """

        lfp = ""
        need_cr = 0
        date_string = time.strftime("%Y%m%d", gmtime)
            
        if self.log_path != "":
            # if the day is new, open a new file
            if gmtime[2] != self.day:
                self.day = gmtime[2]

                # first, close any currently open log file
                if self.fp != None:
                    self.fp.close()

                try:
                    lfp = log_file_path(self.log_path, date_string, self.suffix)
                    try:
                        # check that the log file exists
                        file_stat = os.stat(lfp)
                    
                        if file_stat.st_size > 0:
                            # the file has at least 1 character
                            self.fp = open(lfp, "a+")
                            # make sure the log file has an ending "\n"
                            #self.fp.seek(-1, 2)
                            #buf = self.fp.read(1)
                            #self.fp.seek(0, 2)

                            #if (len(buf) > 0):
                            
                            for line in lfp:
                                if(line[-1] != '\n'):    

                                    need_cr = 1
                                else:
                                    need_cr=0 
                        else:            
                            # the log file had size 0 so simply open it
                            self.fp = open(lfp, "a")
                            

                    except:
                        # the log file did not exist so create it
                        self.fp = open(lfp, "a")
                        
                except:
                    raise RuntimeError(Log_error + "write: file error on %s, %s, %s" % (lfp, sys.exc_info()[0], sys.exc_info()[1]))
        else:
            self.fp = sys.stdout

        # write the message to the current log file
        try:
            out_msg = ""
            if inc_time:
                time_string = time.strftime(LOG_TIME_STRING, gmtime)
                out_msg = "%s%s" % (time_string, msg)
            else:
                out_msg = msg

            if need_cr:
                self.fp.write("\n%s" % out_msg)
            else:
                self.fp.write("%s" % out_msg)

            self.fp.flush()
          
        except:
            raise RuntimeError(Log_error + "write: file processing error")

                

if __name__ == '__main__':
    
    logg = Log("log_test")
    logg.set_suffix(".pyl")
    logg.write_time("Starting: log.py\n")
    logg.write("Info: rolling the logs again\n")
    logg.write_time("Info: rolling the logs\n")
    logg.write_time("Error: rolling the logs again\n")
    logg.write_time("Ending: log.py")

       
