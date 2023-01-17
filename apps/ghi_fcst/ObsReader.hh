//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: ObsReader.hh,v $
//       Version: $Revision: 1.2 $  Dated: $Date: 2015/10/29 20:33:20 $
//
//==============================================================================

/**
 *
 * @file ObsReader.hh
 *
 * @class ObsReader ObsReader reads a netCDF file containing observation values. 
 *                  Methods retrieve load observations for a given site ID.
 *                  The format and contents of the file are assumed to be known  
 *  
 * @date 07/28/2021
 *
 */

#ifndef OBS_READER_HH
#define OBS_READER_HH

#include<vector>
#include<string>
#include<map>

using std::string;
using std::map;
using std::vector;

/**
 * @class ObsReader
 */
class ObsReader 
{
public:
 
  const static float OBS_MISSING;
  const static float PI;

  /** 
   * Constructor
   * @param[in] obsFilepath  Path of netCDF input file
   * @param[in] obsDataResolution  time between observations 
   */
  ObsReader(const string &obsFile, const int obsDataResolution);

  /** 
   * Destructor 
   */
  ~ObsReader();
  
  /**
   * Read netCDF file
   * @return 0 if netCDF file is successfully read
   */
  int parse(void);
  
  /**
   * Return error string if file read fails
   */
  const string &getError() const {return error;}
  
  /**
   * Creation time of file. Used to determine upper bound of observation data.
   */
  const double getCreationTime() const
  {
    return creationTime;
  }
     
  /**
   * Check file contents to see of observation time is within the bounds of 
   * data times in the file and if the site id is in the file
   */
  const bool haveData(const int siteId, const double obsTime) const; 
 
  /**
   * Determine time bounds for data in the observation file
   * @param[out] start  Start time or lower bound of observation data time
   * @param[out] end  End time or upper bound of observation data time
   */
  void getStartEndTimes( double &start, double &end) const; 

  /**
   * Determine lower time bound for data in the observation file
   * @param[out] start  Start time or lower bound of observation data time
   * @param[out] end  End time or upper bound of observation data time
   */
  const double getStartTime(void) const {return timesList[0];} 

  /**
   * Get solar azimuth data for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation time in seconds.
   * @return observed data value
   */
  const float getAzimuth(const int siteId, const double obsTime);

  /**
   * Get solar elevation data for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation time in seconds.
   * @return observed data value
   */
  const float getElevation(const int siteId, const double obsTime);

  /**
   * Get GHI for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation data time in seconds.
   * @return observed data value
   */
  const float getGHI(const int siteId, const double obsTime) ;

  /**
   * Get clearness index, Kt, for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation data time in seconds.
   * @return observed data value
   */
  const float getKt(const int siteId, const double obsTime) ;

  /**
   * Get precipitation rate for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation data time in seconds.
   * @return observed data value
   */
  const float getPrecip(const int siteId, const double obsTime) ;

  /**
   * Get pressure for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation data time in seconds.
   * @return observed data value
   */
  const float getPressure(const int siteId, const double obsTime) ;

  /**
   * Get relative humidity data for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation time in seconds.
   * @return observed data value
   */
  const float getRh(const int siteId, const double obsTime) ;

  /**
   * Get temperature data for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation time in seconds.
   * @return observed data value
   */
  const float getTemp(const int siteId, const double obsTime); 

  /**
   * Get top of the atmosphere irradiance (TOA) data for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation time in seconds.
   * @return observed data value
   */
  const float getToa(const int siteId, const double obsTime);

  /**
   * Get wind direction data for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation data time in seconds.
   * @return observed data value
   */
  const float getWindDir(const int siteId, const double obsTime) ;

  /**
   * Get wind speed data for site ID at observation time
   * @param[in] siteId  Integer site id for observation data
   * @param[in] obsTime  Observation data time in seconds.
   * @return observed data value
   */  
  const float getWindSpeed(const int siteId, const double obsTime) ;

private:
  
  /**
   * String containing error message for failed file read
   */
  string error;

  /**
   * String containing input file name
   */
  string inputFile;
  
  /**
   * Number of observation times in the file
   */
  int numTimes;

  /**
   * First (earliest) observation time in the file
   */
  double firstTime;

  /**
   * Last (latest) observation time in the file
   */
  double lastTime;
  
  /**
   * Upper bound on observation times in files
   */
  double creationTime;
  
  /**
   * Array of times corresponding to observation data arrays
   */
  vector<double> timesList;
  
  /**
   * Number of siteIds in netCDF file
   */
  int numSites;

  /**
   * Array of strings containing siteIds
   */
  vector< int> siteList;

  /**
   * Map of site identification codes to interger index which indicates the order 
   * of site data in observation data array
   */
  map< const int, int> siteIdIndexMap;
  
  /**
   * Dimension of observations array (numTimes * numSites)
   */
  int numObs;

  /**
   * The resolution of observations in seconds
   */
  int obsDataResolutionSecs;

  /**
   *  Solar azimuth angle data array for all sites and all observations
   */
  vector <float> azimuth;

  /**
   *  Solar elevation angle data array for all sites and all observations
   */
  vector <float> elevation;

  /**
   *  GHI data array for all sites and all observations
   */
  vector<float> ghi;

  /**
   *  Clearness index, kt, data array for all sites and all observations
   */
  vector<float> kt;  

  /**
   *  Precipitation rate data array for all sites and all observation times 
   */
  vector<float> precip;
  
  /**
   *  Pressure data for all sites and all observation times
   */
  vector <float> pres;

  /**
   *  Relative humidity data array for all sites and all observation times
   */
  vector<float> rh;
 
  /**
   * Temperature data array for all sites and all observations
   */
  vector<float> temp;

  /**
   *  Top of the atmosphere irradiance data array for all sites and all 
   *  observation times
   */
  vector<float> toa;

  /**
   *  Wind direction data array for all sites and all observation times
   */
  vector<float> windDir;

  /**
   * Wind speed data array for all sites and all observation times
   */
  vector<float> windSpeed; 
 
  /**
   * Get array or vector offset of observation for time and site id.
   */ 
  const int getArrayOffset( const int siteId, const double obsTime); 
};

#endif /* OBS_READER_HH */
