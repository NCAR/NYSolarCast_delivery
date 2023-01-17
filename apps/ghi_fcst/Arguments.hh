//==============================================================================
//
//   (c) Copyright, 2008 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//
//       File: $RCSfile: Arguments.hh,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 *  @file Arguments.hh
 *  @class Arguments
 *  @brief Class for parsing command line arguments.
 *  @date 07/28/2021
 */

#ifndef ARGUMENTS_HH
#define ARGUMENTS_HH

#include <string>
#include <time.h>
#include <vector>

using std::string;
using std::vector;

/** 
 * Arguments class 
 */
class Arguments
{
public:
  /** 
   *Constructor 
   */
  Arguments(int argc, char **argv);

  /** 
   * Print values of command line arguments 
   */
  void print();

  /** 
   * Command line string
   */
  string commandString;

  /** 
   * Name of program
   */
  string programName;

  /** 
   * Vector containing files which contain forecasts of meteorological          
   * variables used in ghi prediction.
   */
  vector <string> nwpFiles;

  /** 
   * String holding comma delimited list of nwp files
   */
  string nwpFilesStr;
  
  /** 
   * Cubist model basename. It is expected that there will be
   * a 'ghiModel.names' and 'ghiModel.model' file
   */
  string cubistModel;

  /**
   * SiteConfigFile is a csv file with integer siteId, latitude, and longitude 
   * of sites for which predictions will be made 
   */
  string siteIdFile;

  /** 
   * Cdl file specifying format of netCDF output
   */
  string cdlFile;

  /**
   * Output directory
   */
  string outputDir;

  /** 
   * Log filepath
   */
  string logDir;

  /** 
   * NetCDF meteorological observations filepath
   */
  vector <string> obsFiles;

  /** 
   * String holding comma delimited list of observation files
   */
  string obsFilesStr;
 
  /**
   * Forecast start time. 
   */
   time_t fcstStartTime;

  /** 
   * Debug level indicator
   */
  int debugLevel;

  /** 
   * Error string
   */
  string error;
  
  /**
   * Flag to indicating producing forecasts for a subset of the lead times
   * (overriding the paramter file setting)
   */
   bool subsetFcst;

  /**
   * String containing comma delimited lead times for generating forecasts
   * on a subset of lead times ( overriding the parameter file)
   */
  string fcstLeadTimesMinsStr;

  /**
   * Subset of forecast lead times for which to make predictions
   */
  vector <int> fcstLeadsSubset; 

  /**
   * The time between forecast lead times 
   */
  int fcstLeadsDelta;

  /**
   * Total number of forecast lead times to process 
   */
  int fcstLeadsNum;

  /** 
   * @param[in] commaStr  Comma delimited list of file strings
   * @param[out] vec  Vector containing individual file string elements 
   */
  void usage(char *prog_name);

  /** 
   * Parse a comma delimited string for individual file path strings
   * @param[in] commaStr  Comma delimited list of file strings
   * @param[out] vec  Vector containing individual file string elements 
   */
  void parseCommaDelimStr(string &commaStr, vector <string> &vec);

  /** 
   * Parse a comma delimited string of integers
   * @param[in] commaStr  Comma delimited list of file strings
   * @param[out] vec  Vector containing integers 
   */
  void parseCommaDelimStr(string &commaStr, vector <int> &vec);
  
};

#endif /* ARGUMENTS_HH */
