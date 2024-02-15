/* *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* */
/* ** Copyright UCAR (c) 1992 - 2012 */
/* ** University Corporation for Atmospheric Research(UCAR) */
/* ** National Center for Atmospheric Research(NCAR) */
/* ** Research Applications Laboratory(RAL) */
/* ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA */
/* ** 2012/9/18 16:58:42 */
/* *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* */
/*
 * Module: log.cc
 *
 * Author: Gerry Wiener
 *
 * Date:   5-6-99
 *
 * Description:
 *     Simple message logger.
 */

#include <string.h>
#include <cstdio>
#include <ctime>
#include <limits.h>
#include "../include/log/log.hh"
using namespace std;

Log::Log()
{
  debug_low = INT_MIN;
  debug_high = INT_MAX;
  fp = NULL;
  path[0] = '\0';
  path_len = 0;

  /* initialize last_tms.tm_mday to impossible value */
  last_tms.tm_mday = -1;
}

Log::Log(const char *base_name)
{
  int ret;

  debug_low = INT_MIN;
  debug_high = INT_MAX;
  err_string = 0;
  fp = NULL;

  ret = strlen(base_name);
  if (ret > LOG_MAX_PATH)
    err_string = "base_name path too long";

  strcpy(path, base_name);
  path_len = ret;

  /* initialize last_tms.tm_mday to impossible value */
  last_tms.tm_mday = -1;
}

Log::Log(const Log &log)	/* copy constructor */
{
  fp = NULL;
  debug_low = log.debug_low;
  debug_high = log.debug_high;
  err_string = log.err_string;
  last_tms = log.last_tms;
  strcpy(path, log.path);
  path_len = log.path_len;
}

/* assignment operator */
Log& Log::operator=(const Log &log) 
{
  if (this != &log)
    {
      if (fp != NULL)
        {
	  fclose(fp);
	  fp = NULL;
	}

      debug_low = log.debug_low;
      debug_high = log.debug_high;
      err_string = log.err_string;
      last_tms = log.last_tms;
      strcpy(path, log.path);
      path_len = log.path_len;
    }

  return *this;
}

Log::~Log()
{
  if (fp != NULL)
    fclose(fp);
}

int Log::basic_write(const char *fmt, int time_flag, int dl, va_list ap)
{
  char buf[LOG_MAX_LINE+LOG_TIME_LEN];
  time_t curr_time;
  int ret;
  struct tm *ptms;

  if (debug_low <= dl && dl <= debug_high)
    {
      /* get time */
      time(&curr_time);

      /* convert to UTC */
      ptms = gmtime(&curr_time);

      if (path[0] == '\0')
	fp = stdout;
      else
	{
	  /* close out old file and open new one if necessary */
	  if (fp == NULL || ptms->tm_mday != last_tms.tm_mday)
	    {
	      if (fp != NULL)
		fclose(fp);

	      /* set year/month/day string */
	      sprintf(&path[path_len], ".%d%.2d%.2d.asc", ptms->tm_year + 1900, ptms->tm_mon + 1, ptms->tm_mday);

	      fp = fopen(path, "a");
	      if (fp == NULL)
		return(-1);
	      last_tms = *ptms;
	    }
	}

      /* fp cannot be NULL at this point */
      if (time_flag)
	{
	  sprintf(buf, "%02d:%02d:%02d ", ptms->tm_hour, ptms->tm_min, ptms->tm_sec);
	  ret = vsnprintf(&buf[LOG_TIME_LEN], LOG_MAX_LINE, fmt, ap);
	  // Not safe ret = vsprintf(&buf[LOG_TIME_LEN], fmt, ap);
	}
      else
	{
	  ret = vsnprintf(buf, LOG_MAX_LINE, fmt, ap);
	  // Not safe ret = vsprintf(buf, fmt, ap);
	}

      fputs(buf, fp);
      fflush(fp);
      return(ret);
    }

  return(0);
}

int Log::write(const char *fmt, ...)
{
  va_list ap;
  int time_flag = 0;
  int dl = debug_low;
  int ret;

  va_start(ap, fmt);
  ret = basic_write(fmt, time_flag, dl, ap);
  va_end(ap);
  return(ret);
}

int Log::write_time(const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int dl = debug_low;
  int ret;

  va_start(ap, fmt);
  ret = basic_write(fmt, time_flag, dl, ap);
  va_end(ap);
  return(ret);
}

int Log::write_time_error(const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int dl = debug_low;
  int ret;

  int   errLen = strlen(fmt) + 8;
  char *errStr = new char[errLen];
  snprintf( errStr, errLen, "Error: %s", fmt );
  errStr[errLen-1] = '\0';

  va_start(ap, fmt);
  ret = basic_write(errStr, time_flag, dl, ap);
  va_end(ap);

  delete[] errStr;
  
  return(ret);
}

int Log::write_time_warning(const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int dl = debug_low;
  int ret;

  int   warnLen = strlen(fmt) + 10;
  char *warnStr = new char[warnLen];
  snprintf( warnStr, warnLen, "Warning: %s", fmt );
  warnStr[warnLen-1] = '\0';

  va_start(ap, fmt);
  ret = basic_write(warnStr, time_flag, dl, ap);
  va_end(ap);

  delete[] warnStr;
  
  return(ret);
}

int Log::write_time_info(const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int dl = debug_low;
  int ret;

  int   infoLen = strlen(fmt) + 7;
  char *infoStr = new char[infoLen];
  snprintf( infoStr, infoLen, "Info: %s", fmt );
  infoStr[infoLen-1] = '\0';

  va_start(ap, fmt);
  ret = basic_write(infoStr, time_flag, dl, ap);
  va_end(ap);

  delete[] infoStr;
  
  return(ret);
}

int Log::write_time_starting() 
{
   return(write_time("Starting.\n"));
}

int Log::write_time_starting(const char *progName) 
{
   return(write_time("Starting %s.\n", progName));
}

int Log::write_time_ending( int exitStatus ) 
{
   return(write_time("Ending:  exit status = %d\n", exitStatus));
}


int Log::write_time_ending(const char *progName, int exitStatus) 
{
  return(write_time("Ending %s: exit status = %d\n", progName, exitStatus));
}


int Log::write(int dl, const char *fmt, ...)
{
  va_list ap;
  int time_flag = 0;
  int ret;

  va_start(ap, fmt);
  ret = basic_write(fmt, time_flag, dl, ap);
  va_end(ap);
  return(ret);
}

int Log::write_time(int dl, const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int ret;

  va_start(ap, fmt);
  ret = basic_write(fmt, time_flag, dl, ap);
  va_end(ap);
  return(ret);
}


int Log::write_time_error(int dl, const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int ret;

  int   errLen = strlen(fmt) + 8;
  char *errStr = new char[errLen];
  snprintf( errStr, errLen, "Error: %s", fmt );
  errStr[errLen-1] = '\0';

  va_start(ap, fmt);
  ret = basic_write(errStr, time_flag, dl, ap);
  va_end(ap);

  delete[] errStr;
  
  return(ret);
}

int Log::write_time_warning(int dl, const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int ret;

  int   warnLen = strlen(fmt) + 10;
  char *warnStr = new char[warnLen];
  snprintf( warnStr, warnLen, "Warning: %s", fmt );
  warnStr[warnLen-1] = '\0';

  va_start(ap, fmt);
  ret = basic_write(warnStr, time_flag, dl, ap);
  va_end(ap);

  delete[] warnStr;
  
  return(ret);
}

int Log::write_time_info(int dl, const char *fmt, ...)
{
  va_list ap;
  int time_flag = 1;
  int ret;

  int   infoLen = strlen(fmt) + 7;
  char *infoStr = new char[infoLen];
  snprintf( infoStr, infoLen, "Info: %s", fmt );
  infoStr[infoLen-1] = '\0';

  va_start(ap, fmt);
  ret = basic_write(infoStr, time_flag, dl, ap);
  va_end(ap);

  delete[] infoStr;
  
  return(ret);
}

int Log::write_time_starting( int dl ) 
{
   return( write_time( dl, "Starting.\n" ) );
}

int Log::write_time_starting( int dl, const char *progName ) 
{
   return( write_time( dl, "Starting %s.\n", progName ) );
}

int Log::write_time_ending( int dl, int exitStatus ) 
{
   return( write_time( dl, "Ending:  exit status = %d\n", exitStatus ) );
}
