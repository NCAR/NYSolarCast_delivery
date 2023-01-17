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
THIS SCRIPT IS USED TO DOWNLOAD NYMESONET DATA
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
import requests

URL = "https://api.nysmesonet.org/data/dynserv/export/epri-nypa/"


def process(out_file, logg, options):
    """ Downloads real-time data from the NYMesonet feed

    Args:
        out_dir : String path to output directory
        logg : log object
        options : options object

    Returns:
    """
    if options.start_date and options.end_date:
        download_url = URL + "/%s/%s" % (options.start_date, options.end_date)
    else:
        download_url = URL + "/latest"

    logg.write_time("Downloading: %s\n" % download_url)
    response = requests.get(download_url)

    logg.write_time("Writing: %s\n" % out_file)
    with open(out_file, "wb") as f:
      f.write(response.content)

    return
  
def main():
    usage_str = "%prog output_file"
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")    
    parser.add_option("-s", "--start_date", dest="start_date", help="Download data starting at this date ('YYYYMMDDTHHMM')")
    parser.add_option("-e", "--end_date", dest="end_date", help="Download data ending at this date ('YYYYMMDDTHHMM')")    
    (options, args) = parser.parse_args()
    
    if len(args) < 1:
        parser.print_help()
        sys.exit(2)

    if options.log:
        logg = log_msg.LogMessage(options.log)
        logg.set_suffix(".asc")
    else:
        logg = log_msg.LogMessage("")
                                  
    out_file = args[0]

    if options.start_date and not options.end_date:
        logg.write_time("Error: If passing start date must also pass end date\n")
        sys.exit()
    if options.end_date and not options.start_date:
        logg.write_time("Error: If passing start date must also pass end date\n")
        sys.exit()

    logg.write_starting()

    process(out_file, logg, options)

    logg.write_ending()


if __name__ == "__main__":
    main()
