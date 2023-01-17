//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: NwpReader.hh,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 * @file NwpReader.hh  NwpReader class reads a netCDF format of expected format. NWP 
 *                     files may have been preprocessed to meet this requirement.
 *                     Variable names and availability are assumed to be known.
 * @class NwpReader  NwpReader class reads a netCDF format of expected format. NWP
 *                     files may have been preprocessed to meet this requirement.
 *                     Variable names and availability are assumed to be known.
 * @date 07/29/2021
 */

#ifndef NWP_READER_HH
#define NWP_READER_HH

#include<vector>
#include<string>
#include<map>
#include<algorithm>

using std::vector;
using std::map;
using std::string;

/**
 * @class NwpReader
 */
class NwpReader 
{
public:

  /**
   * Missing nwp data value.
   */
  const static float NWP_MISSING;

  /**
   * Resolution in seconds of forecast data
   */
  const static int FCST_TIME_RESOLUTION;

  /** 
   * Constructor
   * @param[in] nwpFile  Path of netCDF input file
   */
  NwpReader(string &nwpFile);
  
  /** 
   * Destructor 
   */
  ~NwpReader() {};
 
  /**
   * Read netCDF file
   * @return 0 if netCDF file is successfully read
   */
  int parse(void);
  
  /**
   * Method to get error string if file read fails
   */
  const string &getError() const 
  { 
    return error; 
  }

  /**
   * Get the generation time of the forecast data.
   * There is an assumption that the data in the forecast file
   * start with first forecast rather than the generation time
   */
  const double getGenTime() const
  {
    return validTime[0] - timeResolution;
  }
  
  /**
   * Check file contents to see if data at fcstTime is within the bounds of 
   * data times in the file
   * @param[in] fcstTime  Forecast data time
   */
  const bool haveData(double fcstTime) const; 

  /**
   * Get solar azimuth angle for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return Data value at forecast time
   */
  const float getAzimuth(const int siteId, const double fcstTime);

  /**
   * Get cloud fraction for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getCloudFrac(const int siteId, const double fcstTime);

  /**
   * Get DHI for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getDHI(const int siteId, const double fcstTime);

  /**
   * Get DNI for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getDNI(const int siteId, const double fcstTime);

   /**
   * Get solar elevation angle for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return data value at forecast time
   */
  const float getElevation(const int siteId, const double fcstTime);
   
  /**
   * Get GHI for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getGHI(const int siteId, const double fcstTime);
  
  /**
   * Get Kt for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getKt(const int siteId, const double fcstTime);

  /**
   * Get MixingRatio data for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getMixingRatio(const int tempSiteId, const double fcstTime) ;

  /**
   * Get surface pressure for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getPsfc(const int siteId, const double fcstTime);
  
   /**
   * Get relative humidity for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getRh(const int siteId, const double fcstTime);

  /**
   * Get mass weighted liquid cloud effective radius for given site 
   * ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTauQcTot(const int siteId, const double fcstTime);

  /**
   * Get mass weighted ice effective radius for given site ID at 
   * given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTauQiTot(const int siteId, const double fcstTime);

  /**
   * Get mass weighted snow optical thickness for given site ID 
   * at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTauQs(const int siteId, const double fcstTime);

  /**
   * Get TAOD5502D, total aerosol optical depth at 550 nm, for given site ID 
   * at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTaod5502d(const int siteId, const double fcstTime);

  /**
   * Get TOA, top of the atmosphere irradiance,  for given site ID at given 
   * forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return data value at forecast time
   */
  const float getToa(const int siteId, const double fcstTime);

  /**
   * Get temperature data for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTemp(const int tempSiteId, const double fcstTime) ;

  /**
   * Get wind direction data for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWindDir(const int siteId, const double fcstTime);

  /**
   * Get wind speed data for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWindSpeed(const int siteId, const double fcstTime);

  /**
   * Get total water path, liquid + ice + snow, for given site ID at given 
   * forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWpTot(const int siteId, const double fcstTime);

  /**
   * Get water vapor path for given site ID at given forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWvp(const int siteId, const double fcstTime);

 /**
   * Get Kt formed by using the WRF TOA value for given site ID at given forecast time
   * (instead if TOA used for forming observed Kt)
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWrfKt2(const int siteId, const double fcstTime);

 /**
   * Get TOA value from WRF for given site ID at given forecast time
   * (the variable toa will contain the TOA value used for observations)
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWrfToa2(const int siteId, const double fcstTime);

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

  /**
   * Get site name at array index.
   * @param[in] siteIndex  Array index
   * @return Integer site ID
   */
  const string getSiteName( const int siteIndex) const
  {
    return siteNames[siteIndex];
  }
  
private:
  
  /**
   *  Vector of site Ids. Forecast data is organized in same site ID order 
   */
  vector <int> siteList;

  /**
   * Map of site index integer Ids which indicates the  
   * site data order in data array
   */
  map < int, int  > siteIdIndexMap;

  /**
   *  Vector of site name strings  
   */
  vector <string> siteNames;

  /**
   * Map of site identification strings to integer index which 
   * indicates the site data order in data array
   */
  map < int, string > siteNamesMap;
  
  /**
   * Array of times corresponding to forecast data arrays
   */
  vector <double> validTime;

  /**
   * Solar azimuth array for all sites and all forecasts
   */
  vector <float> azimuth;

  /**
   *  Max cloud fraction for all sites and all forecasts 
   */
  vector <float> cloudFrac;

  /**
   *  Diffuse horizontal irradiance data array for all sites and all forecasts
   */
  vector <float> dhi;

  /**
   *  Direct normal irradiance data array for all sites and all forecasts
   */
  vector <float> dni;

  /**
   * Solar elevation array for all sites and all forecasts
   */
  vector <float> elevation;

  /**
   *  global horizontal irradiance data array for all sites and all forecasts
   */
  vector <float> ghi;

  /**
   *  Clearness index data array for all sites and all forecasts
   */
  vector <float> kt;

  /**
   * Mixing ratio data array for all sites and all forecasts
   */
  vector <float> mixingRatio;

  /**
   *  Surface pressure data array for all sites and all forecasts
   */
  vector <float> pSfc;

  /**
   *  Relative humidity data array for all sites and all forecasts
   */
  vector <float> rh;

  /**
   * TAU_QC_TOT: Mass weighted liquid optical thickness data array
   * for all sites and all forecasts.
   */
  vector <float> tauQcTot;

  /**
   * TAU_QI_TOT: Mass weighted ice effective radius data array
   * for all sites and all forecasts
   */
  vector <float> tauQiTot; 

  /**
   * TAU_QS: Mass weighted snow optical thickness data array
   * for all sites and all forecasts
   */
  vector <float> tauQs;

  /**
   * TAOD5502D: Total aerosol optical depth at 550nm data array 
   * for all sites and all forecasts
   */
  vector <float> taod5502d;

  /**
   * Temperature (2m)data array for all sites and all forecasts
   */
  vector <float> temp;

  /**
   *  Top of the atmosphere irradiance data array for all sites 
   *  and all forecasts
   */
  vector <float> toa;

  /**
   *  Wind direction data array for all sites and all forecasts
   */
  vector <float> windDir;
  
  /**
   *  Wind speed data array for all sites and all forecasts
   */
  vector <float> windSpeed;

  /**
   * WP_TOT: Total water path, the sum of liquid, ice, and snow water paths 
   * This array is for all sites and all forecasts 
   */
  vector <float> wpTot;

  /**
   * WVP: Water vapor path data array for all sites and all forecasts
   */
  vector <float> wvp;

  /**
   * Wrf GHI/WrfTOA data array for all sites and all forecasts
   */
  vector <float> wrfKt2;

  /**
   * Wrf TOA data array for all sites and all forecasts
   */
  vector <float> wrfToa2;

  /**
   * Total number of sites for which forecasts are made
   */
  int numSites;

  /**
   * Last forecast time in the file.
   */
  double lastFcstTime;

  /**
   * Time resolution of lead times
   */
  int timeResolution;

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
   * Return the offset of a data variable with this site ID at forecast time
   */ 
  const int getArrayOffset( const int siteId, double fcstTime) ;
};

#endif /* NWP_READER_HH */
