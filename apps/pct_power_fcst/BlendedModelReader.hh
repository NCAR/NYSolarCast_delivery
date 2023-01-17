//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: BlendedModelReader.hh,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 * @file BlendedModelReader.hh
 *
 * @class BlendedModelReader
 *
 *  <file/class description>
 *  
 * @note <description> [optional]
 *
 * @date 03/20/2021
 *
 */

#ifndef BLENDED_MODEL_READER_HH
#define BLENDED_MODEL_READER_HH

#include<vector>
#include<string>
#include<map>
#include<algorithm>

using std::vector;
using std::map;
using std::string;

/**
 * @class BlendedModelReader
 */
class BlendedModelReader 
{
public:

  /**
   * The Missing Data Value is not explicitly stated in file. Default NetCDF 
   * float value assumed.
   */
  const static float MISSING;

  /**
   * Forecast lead time resolution
   */
  int fcst_time_resolution;

  /** 
   * Constructor
   * @param[in] dicastFile  Path of netCDF input file
   */
  BlendedModelReader(string &dicastFile);
  
  /** 
   * Destructor 
   */
  ~BlendedModelReader() {};
 
  /**
   * Read netCDF file
   * @return 0 if netCDF file is successfully read
   */
  int parse(void);
  
  /**
   * Error string if file read fails
   */
  const string &getError() const 
  { 
    return error; 
  }

  /**
   * Get the generation time of the forecast data.
   */
  const double getGenTime() const
  {
    return validTime[0] - fcst_time_resolution;
  }

  /**
   * Get the forecast resolution of the data.
   */
  const int getFcstResolution() const
  {
    return fcst_time_resolution;
  }
  
  /**
   * Check file contents to see if data at fcstTime is within the bounds of 
   * data times in the file
   * @param[in] fcstTime  Forecast data time
   * @return true or false 
   */
  const bool haveData(double fcstTime) const; 

  /**
   * Get climate zone for given site ID 
   * @param[in] siteId  Integer site id for forecast data
   * @return integer zone id 
   */
  const int getClimateZone(const int siteId);

  /**
   * Get GHI for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getGHI(const int siteId, const double fcstTime);

  /**
   * Get relative humidity data for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getRh(const int siteId, const double fcstTime);

  /**
   * Get temperature data for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTemp(const int tempSiteId, const double fcstTime) ;

  /**
   * Get integer index of site ID in array. Note that data is ordered
   * by site IDs
   * @param[in] site  Integer site id
   * @return Array index for site id
   */
  const int getSiteIndex(const int siteId);

  /**
   * Get site ID at array index.
   * @param[in] siteIndex  Array index
   * @return Integer site ID
   */
  const int getSite(const int siteIndex) const 
  {
    return siteList[siteIndex];
  }

private:
  
  /**
   *  Vector of site Ids. Forecast data is organized in same site ID order 
   */
  vector <int> siteList;

  /**
   * Map of site index integer Ids which indicates the order data 
   * for data from that site in observation data array
   */
  map < int, int  > siteIdIndexMap;

  /**
   * Map of site index integer Ids to climate zones 
   */
  map < int, int  > siteClimateZoneMap;

  /**
   *  Vector of site name strings  
   */
  vector <string> siteNames;

  /**
   * Map of site identification strings to interger index which indicates the order data
   * for data from that site in observation data array
   */
  map < int, string > siteNamesMap;
  
  /**
   * Array of times corresponding to forecast data arrays
   */
  vector <double> validTime;

  /**
   * climate zone for all sites 
   */
  vector <int> climateZone;

  /**
   *  GHI array for all sites and all forecasts 
   */
  vector <float> ghi;
 
  /**
   *  Relative humidity for all sites and all forecasts
   */
  vector <float> rh;

  /**
   *  Temperature data array for all sites and all forecasts
   */
  vector <float> temp;

  /**
   * Total number of sites for which forecasts are made
   */
  int numSites;

  /**
   * Last forecast time in the file.
   */
  double lastFcstTime;

  /**
   * String containing error message for failed file read
   */
  string error;

  /**
   * String containing input file path 
   */
  string inputFile;

  /**
   *  Creation time of input file 
   */ 
  double creationTime;

  /**
   * Get the offset of a data variable with this site ID at forecast time
   */ 
  const int getArrayOffset( const int siteId, double fcstTime) ;
};

#endif /* BLENDED_MODEL_READER_HH */
