//==============================================================================
//
//   (c) Copyright, 2008 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: FcstProcessor.hh,v $
//       Version: $Revision: 1.2 $  Dated: $Date: 2014/11/25 03:42:58 $
//
//==============================================================================

/**
 * @file FcstProcessor.hh
 * @brief The FcstProcessor class is the forecast driver: Input data files are read, 
 *        a cubist driver is instantiated with model input, cubist model is fed 
 *        appropriate sets of predictors, GHI forecasts are produced and written
 *        to a netCDF file. 
 * @class FcstProcessor
 */

#ifndef FCST_PROCESSOR_HH
#define FCST_PROCESSOR_HH

#include <string>
#include <vector>
#include <utility>
#include <cubist_interface/cubist_interface.hh>
#include "NwpReader.hh"
#include "Arguments.hh"
#include "ObsReader.hh"
#include "NwpMgr.hh"
#include "ObsMgr.hh"
#include "SiteMgr.hh"

using std::string;
using std::vector;

/**
 * @class FcstProcessor
 */
class FcstProcessor
{
public:

  /** 
   * Constructor 
   * Copy command line arguments, initialize variables
   * @param[in] args command line arguments
   */
  FcstProcessor(const Arguments &argsParam);

  /** 
   * Destructor
   */
  ~FcstProcessor();

  /**
   * Run method is a collection of high level methods which read parameters, 
   * read meteorological observations, read NWP forecast files, 
   * instantiate a cubist driver, call cubist to predict GHI, write netCDF
   * @return 1 for failure, 0 for success.
   */
  int run();

  string error;

  /**
   * Missing data value expected by cubist driver
   */
  const static float CUBIST_MISSING;

private:
 
  /**
   * Object containing command line arguments
   */
  Arguments args;

   /**
    * Manager of site IDs and site locations
    */
  SiteMgr *siteMgr; 

  /**
   * Temporal resolution of forecasts
   */
  int fcstLeadsDelta; 

  /**
   * Vector of pointers to cubist_interface objects
   */
   vector < cubist_interface* > leadTimeCubistModels;

  /**
   * The forecast times corresponding to predicted GHI values
   */
  vector <double> validTimes;

  /**
   * Call letters if each mesonet site
   */
  vector <string> siteNames;

  /**
   * Integer ID for each mesonet site. Note that these are
   * related to the nymesonet ids. They are just for internal
   * processing. 
   */
  vector <int> siteIds;

  /**
   * The forecasted GHI values for all sites and all lead times
   */
  vector <float> ghiAll;

  /**
   * The forecasted clearness index values for all sites and all lead times
   */
  vector <float> ktAll;

  /**
   * The average TOA values for all sites and all lead times 
   */
  vector <float> toaAll;

  /**
   * The average solar elevation values for all sites and all lead times
   */
  vector <float> solarElAll;

   /**
   * The forecasted GHI values for all sites and all lead times
   */
  vector <float> wrfGhiAll;

  /**
   * The forecasted clearness index values for all sites and all lead times
   */
  vector <float> wrfKtAll;

  /**
   * The average TOA values for all sites and all lead times 
   */
  vector <float> wrfToaAll;

  /**
   * Integer indicator of the level of debug messaging
   */
  int debugLevel;

  /**
   * For each forecast lead time, load a vector of predictor values, 
   * feed to predictive model, record prediction.
   * @param[in] nwpMgr  NetCDF forecast file manager 
   * @param[in] obsMgr  NetCDF observation file manager
   * @param[out] genTime  Forecast generation time
   * @return 1 for failure, 0 for success.
   */
  int predict(NwpMgr &nwpMgr, ObsMgr &obsMgr, double &genTime);

  /**
   * @param[in] cdlFile  A Common data form Description Language File supplied
   *                    by the user to specify the output netCDF file format
   * @param[in] outputDir  String indicating the output directory for the 
   *                       forecast data.
   * @param[in] genTime  Generation time of the forecast. Used in output 
   *                     filename
   */
  void writeNetcdf(const string cdlFile, const string outputDir, 
		   const double genTime) ;

  /**
   * Instantiate the cubist interface for each lead time using the model base 
   * string from the command line plus ".ltNNN" where NNN is a string 
   * representing 3-digit lead minutes.
   * @return 1 for failure, 0 for success
   */
  int loadCubistModels();

  /**
   * Retrieve observations and NWP values to be used as predictors. 
   * @param[in] fcstTime  Forecast valid time 
   * @param[in] fcstGenTime  Forecast generation time
   * @param[in] siteID
   * @param[out] predictorVal  Vector of predictors for statistical learning 
   *                           model.
   * @param[in] NwpMgr  Manager class for NWP data, retrieves nwp data at 
   *                       given valid time 
   * @params[in] ObsMgr  Manager class for observation data, retrieves 
   *                     observation time at given time   
   */
  void loadPredictors(const double fcstTime, const double fcstGenTime,
                      const int siteID, vector <float> & predictorVals, 
                      NwpMgr &nwpMgr, ObsMgr &obsMgr);
  /**
   * Interface to the statistical learning model takes a csv string as input. 
   * Create that string from the predictors.
   * @param[in] predictorVals  Vector of predictors for statistical learning 
   *                           model.
   * @param[out] cubistInputStr  A csv string to feed the statistical learning  
   *                             model 
   */
  void createCubistInputStr( const vector <float> predictorVal, 
                            string &cubistInputStr);
};

#endif /* PROCESS_HH */
