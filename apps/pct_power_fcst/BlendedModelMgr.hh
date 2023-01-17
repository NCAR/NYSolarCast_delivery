//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: BlendedModelMgr.hh,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 * @file BlendedModelMgr.hh  Class manages requests for forecast data from n NWP files.
 *                     The data returned is always from the most recent forecast for which it
 *                     is available
 *
 * @class BlendedModelMgr  Class manages requests for forecast data from n NWP files.
 *                   The data returned is always from the most recent forecast for which it
 *                   is available
 * @date 05/20/21
 */

#ifndef BLENDED_MODEL_MGR_HH
#define BLENDED_MODEL_MGR_HH

#include <vector>
#include <string>
#include "BlendedModelReader.hh"

class BlendedModelMgr 
{
public:

  /** 
   * Constructor
   */
  BlendedModelMgr(void);
  
  /** 
   * Constructor
   */
  BlendedModelMgr(BlendedModelReader *blendedModelFile);
  
  /** 
   * Destructor 
   */
  ~BlendedModelMgr();
  
  /** 
   * Add BlendedModelReader object to vector
   * @param[in] blendedModelFile  BlendedModelReader object
   */
  void add(BlendedModelReader *blendedModelFile);
 
  /**
   * Get generation time of indicated file
   * @return unix forecast generation time
   */
  const double getGenTime(int fileIndex) const { return _modelFiles[fileIndex]->getGenTime(); }

  /**
   * Get forecast lead time resolution 
   * @return forecast lead time resolution in seconds
   */
  const int    getFcstResolution() const { return _modelFiles[0]->getFcstResolution();}

  /**
   * Get the generation of the most recent forecast file 
   * @return unix forecast generation time
   */
  const double getMostRecentGenTime() const { return _modelFiles[0]->getGenTime(); }

  /**
   * Get missing data value
   * @return missing data value
   */
  const float getMissing() const { return BlendedModelReader::MISSING;} 

  /**
   * Get climate zone for a given site ID
   * @param[in] siteId  Integer site id for forecast data
   * @return id  
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
   

private:

  /**
   * container for model file reader objects- one per forecast file
   */ 
  vector< BlendedModelReader* > _modelFiles;
 
  /**
   * Get the index of the file containing the most latest data 
   * generated at forecast time
   * @return integer file index 
   */
  const int getBlendedModelFileIndex(const double fcstTime) const;
};

#endif /* BLENDED_MODEL_MGR_HH */
