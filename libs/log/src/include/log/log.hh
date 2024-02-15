/* *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* */
/* ** Copyright UCAR (c) 1992 - 2012 */
/* ** University Corporation for Atmospheric Research(UCAR) */
/* ** National Center for Atmospheric Research(NCAR) */
/* ** Research Applications Laboratory(RAL) */
/* ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA */
/* ** 2012/9/18 16:58:20 */
/* *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* */
/*
 *   Module: log.hh
 *
 *   Author: Gerry Wiener
 *
 *   Date:   5/6/99
 *
 *   Description: Defines log class.
 */
#ifndef LOG_HH
#define LOG_HH

#include <cstdarg>
#include <cstdio>
#include <ctime>
using namespace std;

const int LOG_MAX_LINE = 2048;
const int LOG_MAX_PATH = 2048;
const int LOG_DATE_LEN = 9;
const int LOG_TIME_LEN = 9;

class Log
{
public:
  Log();
  Log(const char *base_name);
  Log(const Log &);	/** copy constructor */
  Log & operator=(const Log &); /** assignment operator */
  ~Log();
  const char * error() const { return err_string; }

  /** set high debug level to dh */
  void set_debug(int dh) { debug_high = dh; }

  /** set low debug level to dl */
  void set_debug_low(int dl) { debug_low = dl; }

  /** write generic log statement in printf format */
  int write(const char *fmt, ...);

  /** write generic log statement in printf format but prepend time */
  int write_time(const char *fmt, ...);

  /** write generic log statement in printf format but prepend time and Error: */
  int write_time_error(const char *fmt, ...);

  /** write generic log statement in printf format but prepend time and Warning: */
  int write_time_warning(const char *fmt, ...);

  /** write generic log statement in printf format but prepend time and Info: */
  int write_time_info( const char *fmt, ...);

  /** write time and Starting: message */
  int write_time_starting();

  /** write time, Starting: and program name message */
  int write_time_starting(const char *prog_name);

  /** write time, Ending: and  exit status value */
  int write_time_ending( int exit_status );

  /** write_time_ending */
  int write_time_ending(const char *progName, int exitStatus);

  /** write debug log message using debug level dl */
  int write(int dl, const char *fmt, ...);

  /** write debug log message using debug level dl but prepend time */
  int write_time(int dl, const char *fmt, ...);

  /** write debug log message using debug level dl but prepend Error: */
  int write_time_error(int dl, const char *fmt, ...);

  /** write debug log message using debug level dl but prepend Warning: */
  int write_time_warning(int dl, const char *fmt, ...);

  /** write debug log message using debug level dl but prepend Info: */
  int write_time_info( int dl, const char *fmt, ...);

  /** write debug Starting message */
  int write_time_starting( int dl );

  /** write debug Starting message with program name */
  int write_time_starting( int dl, const char *prog_name );

  /** write debug Ending  message with exit status */
  int write_time_ending( int dl, int exit_status );

  /** return log file path */
  const char *get_path() {return path;};

private:
  int basic_write(const char *fmt, int time_flag, int dl, va_list ap);
  int debug_low;
  int debug_high;
  const char *err_string;
  FILE *fp;
  struct tm last_tms;
  char path[LOG_MAX_PATH+LOG_DATE_LEN];
  int path_len;
};



#endif /* LOG_HH */


