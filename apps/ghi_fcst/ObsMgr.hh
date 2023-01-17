//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: ObsMgr.hh,v $
//       Version: $Revision: 1.2 $  Dated: $Date: 2015/10/29 20:33:20 $
//
//==============================================================================

/**
 *
 * @file ObsMgr.hh  ObsMgr manages n netCDF files containing observed meteorological
 *                  data values. Methods enable equests for data retrieve 
 *                  observations from relevant file at a certain time for a 
 *                  particular site id.
 *
 * @class ObsMgr ObsMgr manages n netCDF files containing observed meteorological
 *               data values. Methods enable equests for data retrieve 
 *               observations from relevant file at a certain time for a 
 *               particular site id.
 *  
 * @date 07/28/2021
 *
 */

#ifndef OBS_MGR_HH
#define OBS_MGR_HH

#include<vector>
#include<string>
#include "ObsReader.hh"

/**
 * @class ObsMgr
 */
class ObsMgr 
{
public:

  /** 
   * Constructor
   */
  ObsMgr();

  /** 
   * Constructor
   */
  ObsMgr(ObsReader *obsReader);

  /** 
   * Destructor 
   */
  ~ObsMgr();
  
  /** 
   * Add ObsReader object to vector
   * @param[in] obsFile  ObsReader object
   */
  void add(ObsReader *obsFile);

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
   * Get precip for site ID at observation time
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
   * Collection of observation file readers-- one for each observation file.
   */ 
  vector< ObsReader* > _obsFiles;

  /**
   * Integer indicator of which reader/file contains data for a site at 
   * a particular time 
   */
  const int getObsFileIndex(const int siteId, const double obsTime) const;
};

#endif /* OBS_MGR_HH */
