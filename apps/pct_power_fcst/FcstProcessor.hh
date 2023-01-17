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
 * @brief The FcstProcessor class is the forecast driver: approporiate input data 
 *        files are read, a cubist driver is instantiated with model input, 
 *        cubist model is fed appropriate predictors, percent capacity forecasts
 *        are produced and written to a netCDF file. 
 *        Note: Cubist is the machine learning algorithm used to create forecat 
 *        models
 * @class FcstProcessor
 */

#ifndef FCST_PROCESSOR_HH
#define FCST_PROCESSOR_HH

#include <string>
#include <vector>
#include <utility>
#include <cubist_interface/cubist_interface.hh>
#include "BlendedModelReader.hh"
#include "Arguments.hh"
#include "BlendedModelMgr.hh"
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
   * Run method is a collection of high level methods which handle parameters, 
   * read blended forecast files,instantiate a cubist driver, call cubist to 
   * predict, and finally write predictions to netCDF
   * @return 1 for failure, 0 for success.
   */
  int run();

  string error;

  /**
   * Missing data value expected by cubist driver
   */
  const static float FCST_MISSING;

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
   cubist_interface* cubistModel;

  /**
   * The forecast times corresponding to predicted percent capacity values
   */
  vector <double> validTimes;

  /**
   * Integer site ID 
   */
  vector <int> siteIds;

  /**
   * The forecasted percent power capacity values for all sites and all lead times
   */
  vector <float> pctCap;

  /**
   * Integer indicator of the level of debug messaging
   */
  int debugLevel;

  /**
   * For each forecast lead time, load a vector of predictor values, 
   * feed to predictive model, record prediction.
   * @param[in] modelMgr  NetCDF blended forecast file manager 
   * @param[out] genTime  Forecast generation time
   * @return 1 for failure, 0 for success.
   */
  int predict(BlendedModelMgr &modelMgr, double &genTime);

  /**
   * @param[in] cdlFile  A Common data form Description Language File supplied
   *                     by the user to specify the output netCDF file format
   * @param[in] outputDir  String indicating the output directory for the 
   *                       forecast data.
   * @param[in] genTime  Generation time of the forecast. Used in output 
   *                     filename
   */
  void writeNetcdf(const string cdlFile, const string outputDir, 
		   const double genTime) ;

  /**
   * Instantiate the cubist interface 
   * @return 1 for failure, 0 for success
   */
  int loadCubistModel();

  /**
   * Retrieve model values used as predictors. 
   * @param[in] fcstTime  Forecast valid time 
   * @param[in] fcstGenTime  Forecast generation time
   * @param[in] siteID
   * @param[out] predictorVal  Vector of predictors for statistical learning 
   *                           model.
   * @param[in] BlendedModelMgr  Manager class for NWP data, retrieves nwp data at 
   *                       given valid time 
   */
  void loadPredictors(const double fcstTime, const double fcstGenTime,
                      const int siteID, vector <float> & predictorVals, 
                      BlendedModelMgr &modelMgr);
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

#endif /* FCST_PROCESSOR_HH */
