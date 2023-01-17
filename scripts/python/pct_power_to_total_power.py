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
THIS SCRIPT WAS DESIGNED TO CONVERT THE PERCENT POWER FORECASTS TO A TOTAL POWER
"""

import time, os, sys
from datetime import datetime
from optparse import OptionParser
import log_msg
from netCDF4 import Dataset
import numpy as np

# Add this so that it doesn't warn on multiplying the masked values
np.seterr(over='ignore')

def process(in_file, capacity, out_file, logg):
    """ This is the main driver function.  
           Reads the input nc, copies all dimensions/variables to the output file
           while changing the 'power_percent_capacity' and 'creation_time'

    Args:
        in_file : string path to input file
        capacity : float total capacity
        out_file : string path to output file
        logg : log object

    Returns:
    """
    logg.write_time("Reading: %s\n" % in_file)
    in_nc = Dataset(in_file, "r")

    logg.write_time("Writing: %s\n" % out_file)
    out_nc = Dataset(out_file, "w")
    
    # Write the dimensions
    for dim in in_nc.dimensions:
        out_nc.createDimension(in_nc.dimensions[dim].name, in_nc.dimensions[dim].size)

    # Write the variables
    for var in in_nc.variables:
        if var == 'power_percent_capacity':
            out_var = out_nc.createVariable("total_power", in_nc.variables[var].dtype,
                                            in_nc.variables[var].dimensions)
            out_var[:] = (in_nc.variables[var][:] / 100) * capacity

            # Set the variable attributes
            out_var.long_name = "Total Power"
            out_var.units = "kW"

        else:
            out_var = out_nc.createVariable(var, in_nc.variables[var].dtype,
                                            in_nc.variables[var].dimensions)
            if var == 'creation_time':
                out_var[:] = int(time.time())
            else:
                out_var[:] = in_nc.variables[var][:]

            # Set the variable attributes
            for attr in in_nc.variables[var].ncattrs():
                out_var.setncattr(attr, in_nc.variables[var].getncattr(attr))

                
    in_nc.close()
    out_nc.close()
    return


def main():
    usage_str = "%prog in_file capacity out_file"
    usage_str += "\n in_file\t path to input NC file"
    usage_str += "\n capacity\t float total capacity"
    usage_str += "\n out_file\t path to outputput NC file"    
    parser = OptionParser(usage = usage_str)
    parser.add_option("-l", "--log", dest="log", help="write log messages to specified file")
        
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
    capacity = float(args[1])
    out_file = args[2]

    logg.write_starting()

    process(in_file, capacity, out_file, logg)

    logg.write_ending()


if __name__ == "__main__":
    main()
