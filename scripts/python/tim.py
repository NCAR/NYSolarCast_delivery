#!/usr/bin/env python

import calendar
import math
import re
import string
import time

SECS_PER_DAY = 86400

TM_YEAR = 0
TM_MON = 1
TM_DAY = 2
TM_HOUR = 3
TM_MIN = 4
TM_SEC = 5
TM_WDAY = 6
TM_JDAY = 7
TM_DST = 8

# Forecast period indicators
fpind_list = ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M"]


def mkgmtime(tup):
    """ Returns a gmtime from a time tuple

    Args:
        tup : time.struct_time

    Returns:
        integer : seconds since epoch
    """
    return calendar.timegm(tup)

#----------------------------------------------------------
def date2sec(date):

    """This function converts the input date to seconds

    Args:
        date:     date in either format YYYYMMDD, YYYYMMDDHH
                  or YYYYMMDDHHMM to be converted to seconds
        
    Returns:
        date_sec: the date converted to seconds

    """
    if len(date) == 8:
        date_str = date + '000000'
    elif len(date) == 10:
        date_str = date + '0000'
    elif len(date) == 12:
        date_str = date + '00'

    date_tup = time.strptime("%s" % (date_str),"%Y%m%d%H%M%S")
    date_sec = mkgmtime(date_tup)

    return(date_sec)

#----------------------------------------------------------
def sec2date(date):

    """
    This function converts the input date in seconds to YYYYMMDDHH

    Args:
        date:   date in sec
        
    Returns:
        date_str:       date  YYYYMMDDHH 
        hr_str:         hour  HH 
    """
    
    date_str_long = time.gmtime(date)
    date_str = "%s%s%s" %(date_str_long[0],string.zfill(date_str_long[1],2),string.zfill(date_str_long[2],2)) 
    hr_str = "%s" %(string.zfill(date_str_long[3],2))
    
    return(date_str, hr_str)

#----------------------------------------------------------   
def yday2mmdd(ydate):

    """
    This function takes the date with the Julian day of the
    year, of the form YYYYDDDHHMM and converts it to MMDD
    and returns YYYYMMDDHHMM
    Works with YYYYDDDHHMMSS, YYYYDDDHHMM, YYYYDDDHH, YYYYDDD,
    YYDDD
    ""

    Args:
        ydate:   date with the Julian day, YYYYDDDHHMM

    Returns:
        date:   date of the form YYYYMMDD and HH
    """

    # put ydate in a time tuple starting at the beginning of
    # the year, without the Julian day of the year, DDD.  Then
    # convert this to seconds

    length = len(ydate)

    if length == 13:
        year = ydate[:4]
        yday = ydate[4:7]
        hour = ydate[7:9]
        min  = ydate[9:11]
        sec  = ydate[11:13]
    elif length == 11:
        year = ydate[:4]
        yday = ydate[4:7]
        hour = ydate[7:9]
        min  = ydate[9:11]
        sec  = '00'
    elif length == 9:
        year = ydate[:4]
        yday = ydate[4:7]
        hour = ydate[7:9]
        min  = '00'
        sec  = '00'
    elif length == 7:
        year = ydate[:4]
        yday = ydate[4:7]
        hour = '00'
        min  = '00'
        sec  = '00'
    elif length == 5:
        year = '20' + ydate[:2]
        yday = ydate[2:5]
        hour = '00'
        min  = '00'
        sec  = '00'

    # convert to seconds year starting at 0 month and 0 day, and use hour, min and sec
    month = '01'
    day = '01'

    date_str = year + month + day + hour + min + sec
    date_tup = time.strptime("%s" % (date_str),"%Y%m%d%H%M%S")
    date_sec = int(time.mktime(date_tup) )

    # convert the julian day of the year to seconds
    # subtract one days worth of seconds because starting from 1 rather than 0
    julian_sec = int(yday) * 86400 - 86400 

    # get the total seconds
    total_sec = date_sec + julian_sec 

    # convert the total seconds back to a date string
    date_str_long = time.gmtime(total_sec)
    date_str = "%s%s%s" %(date_str_long[0],string.zfill(date_str_long[1],2),string.zfill(date_str_long[2],2)) 
    hr_str = "%s" %(string.zfill(date_str_long[3],2))

    return(date_str, hr_str)   

#----------------------------------------------------------   
if __name__ == '__main__':
    #l = time_list(0, SECS_PER_DAY, 10)
    # print l
    print (date_list_time_delta("20070101", "20070102", 900))
    print (date_list("20061101", "20070101"))
    print ("hms(3*3600 + 7 * 60 + 9): ", hms(3*3600 + 7 * 60 + 9))
    print ("tp2sec: ", tp2sec("HHHMMSS", "1000102"))

    seconds = time.time()
    print ("seconds: ", seconds)
    gm_tuple = time.gmtime(seconds)
    print ("gmt:        ", gm_tuple)
    local_tuple = time.localtime(time.time())
    print ("local time: ", local_tuple)
    print ("seconds from mkgmtime: ", mkgmtime(gm_tuple))
    print ("seconds from mktime:   ", time.mktime(local_tuple))
    
    print (datetotuple("19981015013245"))
    a=list(datetotuple("19981015013245"))
    a[TM_HOUR] = a[TM_HOUR] - 3
    print (a)
    print (tuple(a))
    print (datetotuple("1998101501"))
    print (datetotuple("19981015"))
    print (datetogmt("1998101501"))
    print (datetogmt("19981015"))
    trunc_day = truncate_day(seconds)
    print ("gmt trunc day: ", time.gmtime(trunc_day))

    print ("Forecast period A = hour", fpind2h("A"))
    print ("Forecast hour 0 = period", h2fpind(0))
    print ("Forecast period M = hour", fpind2h("M"))
    print ("Forecast hour 12 = period", h2fpind(12))

    print (date_list_hhmm("200612202330", "200701012330"))
