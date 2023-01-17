//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: NwpMgr.hh,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 * @file NwpMgr.hh  NwpMgr manages requests for forecast data from n NWP 
                    files. The data returned is always from the most recent 
 *                  forecast for which it is available.
 *
 * @class NwpMgr  NwpMgr manages requests for forecast data from n NWP files.
 *                The data returned is always from the most recent forecast for
 *                which it is available.
 * @date 03/20/21
 */

#ifndef NWP_MGR_HH
#define NWP_MGR_HH

#include <vector>
#include <string>
#include "NwpReader.hh"



class NwpMgr 
{
public:

  /** 
   * Constructor
   */
  NwpMgr(void);
  
  
  /** 
   * Constructor
   */
  NwpMgr(NwpReader *nwpFile);
  
  /** 
   * Destructor 
   */
  ~NwpMgr();
  
  /** 
   * Add NwpReader object to vector
   * @param[in] nwpFile  NwpReader object
   */
  void add(NwpReader *nwpFile);

  const double getGenTime(int fileIndex) const 
               {return _nwpFiles[fileIndex]->getGenTime(); }

  const double getMostRecentGenTime() const {return _nwpFiles[0]->getGenTime(); }

  const float getMissing() const { return NwpReader::NWP_MISSING;} 
   
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
   * Get mass weighted liquid cloud optical thickness for given site ID at given
   * forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTauQcTot(const int siteId, const double fcstTime);

  /**
   * Get mass weighted ice optical thinckness for given site ID at given 
   * forecast time
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getTauQiTot(const int siteId, const double fcstTime);

  /**
   * Get mass weighted snow optical thickness for given site ID at given 
   * forecast time
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
   * Get wrf Kt  for given site ID at given forecast time
   * The wrf TOA is different than the one used for observed Kt calculations
   * This Kt value uses wrf GHI/wrf TOA
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWrfKt2(const int siteId, const double fcstTime);

  /**
   * Get wrf TOA  for given site ID at given forecast time
   * This TOA is different than the one used for observed Kt calculations
   * @param[in] siteId  Integer site id for forecast data
   * @param[in] fcstTime  forecast data time in seconds.
   * @return forecasted data value
   */
  const float getWrfToa2(const int siteId, const double fcstTime);

private:
 
  vector< NwpReader* > _nwpFiles;
 
  const int getNwpFileIndex(const double fcstTime) const;
};

#endif /* NWP_MGR_HH */
