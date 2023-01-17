//==============================================================================
//
//   (c) Copyright, 2011 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: FcstProcessor.cc,v $
//       Version: $Revision: 1.6 $  Dated: $Date: 2016/03/07 21:49:01 $
//
//==============================================================================

/**
 * @file FcstProcessor.cc
 * @brief Source for FcstProcessor class
 */

// Include files 

#include <iostream>
#include <fstream>
#include <time.h>
#include <math.h>
#include <string>
#include <vector>
#include <string>
#include <log/log.hh>
#include "Arguments.hh"
#include "FcstProcessor.hh"
#include "cdf_field_writer.hh"

using std::string;
using std::vector;

extern Log *Logg;
extern int DebugLevel;

const float FcstProcessor::FCST_MISSING = NC_FILL_FLOAT;

FcstProcessor::FcstProcessor(const Arguments &argsParam):
  args(argsParam)
{ 
  error = string("");
}

FcstProcessor::~FcstProcessor()
{  
  //
  // Cleanup
  //
  if( siteMgr)
  {
     delete siteMgr;
  }
  if (cubistModel)
  {
     delete cubistModel;
  }
}

int FcstProcessor::run()
{
  Logg->write_time("Info: Running process.\n");

  if (DebugLevel > 0)
  {
     args.print();
  }

  //
  // Instantiate manager of NWP forecast files and load files
  //  
  BlendedModelMgr modelMgr;

  if ((int) args.modelFiles.size() == 0)
  {
     Logg->write_time("ERROR: No NWP data available. fcst cannot run.\n");
     
     return 1;
  }

  for (int i = 0; i < (int) args.modelFiles.size(); i++)
  {
    if (DebugLevel > 1)
    {
      Logg->write_time("Info: Reading blended model file %s\n", 
		       args.modelFiles[i].c_str());
    }

    //
    // Create a reader object for each NWP file
    //
    BlendedModelReader *modelReader = new BlendedModelReader(args.modelFiles[i]);  

    //
    // parse file and store reader if successful, return error otherwise 
    //
    modelReader->parse();

    string modelError = modelReader->getError();

    if ( strcmp(modelError.c_str(),"") != 0 )
    {
      Logg->write_time("ERROR: Failure to reading blended model file %s: %s\n", 
		       args.modelFiles[i].c_str(), modelError.c_str());

      return 1;
    }
    else
    {
      modelMgr.add(modelReader);
    }
  }

  //
  // Create cubist interface objects for each lead time
  // Cubist is the machine learning algorithm
  //
  if( loadCubistModel())
  {
     Logg->write_time("Error: Cubist interface did not initialize properly ");
     return 1;
  }

  //
  // Instantiate siteMgr for integer siteIDs
  // 
  siteMgr = new SiteMgr(args.siteIdFile);
 
  if( siteMgr->parse())
  {
     Logg->write_time("Error: Failure to read siteID file:  ",
                       args.siteIdFile.c_str());
      return 1;
  }
 
  //
  // Get predictors and use cubist_interface object to calculate 
  // the predictand then store forecast values for each site at each lead time
  //
  double fcstGenTime;

  if ( predict(modelMgr, fcstGenTime))
  {
    Logg->write_time("Error: Prediction failure.");

    return 1;
  }

  //
  // Write netCDF output file
  //
  writeNetcdf(args.cdlFile, args.outputDir, fcstGenTime);

  return 0;
}

int FcstProcessor::predict( BlendedModelMgr &modelMgr, double &fcstGenTime) 
{

   //
   // Get the generation time from the  optional input arg 'fcstStartTime'
   // or from most recent input NWP file (NWP model trigger)
   // 
   if (args.fcstStartTime >= 0)
   { 
      fcstGenTime = args.fcstStartTime;
   }
   else
   {
      time_t genTime = modelMgr.getMostRecentGenTime() ;

      fcstGenTime = genTime;

      Logg->write_time("fcstGen time %lf\n",fcstGenTime);
   }

   //
   // Loop through sites making all forecasts at each site
   //
   for( int s = 0; s < siteMgr->getNumSites(); s++)
   {
      //
      // Get and save the site IDs and site names for use or for the output file
      //
      int siteId = siteMgr->getSiteId(s);

      siteIds.push_back(siteId);

      //
      // Loop on forecasts being computed (either all possible forecasts or 
      // a subset (which is an optional command line arg). Set the fcstLeadBound 
      // and firstFcstTime appropriately. 
      //
      time_t firstFcstTime;

      int fcstLeadBound;
 
      if (args.subsetFcst)
      {
         firstFcstTime = fcstGenTime + args.fcstLeadsSubset[0]* 60;
     
         fcstLeadBound = (int)args.fcstLeadsSubset.size();

         if (DebugLevel > 1)
         { 
            Logg->write_time("Info: Calculating %d %d minute forecasts starting "
                             "at %ld.\n", fcstLeadBound, args.fcstLeadsDelta, 
                             firstFcstTime );
         }
      } 
      else
      {
         firstFcstTime = fcstGenTime + fcstLeadsDelta;

         fcstLeadBound = args.fcstLeadsNum;

         if (DebugLevel > 1)
         {
            Logg->write_time("Info: Calculating %d %d minute forecasts for site %d.\n", 
                             fcstLeadBound, args.fcstLeadsDelta,s );
         }
      }

      //
      // Loop on total number of lead times to be processed. For each lead time:
      // 1) fill vector with predictor values for feeding the cubist model
      // 2) convert the vector of  predictors to a string to satisfy the cubist 
      //    interface
      // 3) call cubist interface to get the Cubist model prediction
      // 4) record the prediction 
      //
      for (int i = 1; i <= fcstLeadBound; i++)
      {
         //
         // Set the forecast time
         //
         double fcstTime;

         if (args.subsetFcst)
         {
            fcstTime = fcstGenTime + args.fcstLeadsSubset[i-1] * 60;
         }
         else
         {
            fcstTime = fcstGenTime + (i* args.fcstLeadsDelta * 60);
         }
         //
         // If site is the first, record the lead times
         //
         if (s == 0)
         {
            validTimes.push_back(fcstTime);
         }

         //
         // Container for predictors for this site and lead time
         //
         vector <float> predictorVals;

         //
         // Get the predictors for this site, generation and lead time 
         //
         loadPredictors(fcstTime, fcstGenTime, siteId, predictorVals, modelMgr);

         //
         // Convert vector of input predictor values to a string to satsify 
         //   cubist interface API
         // 
         string cubistInputStr = string("");

         createCubistInputStr( predictorVals, cubistInputStr);

         //
         // Get the prediction using the cubist interface 
         //
         float prediction = cubistModel->predict(cubistInputStr);

         //
         // Record in container that has predictions for all siteIds and all 
         // times.
         // if GHI is MISSING or near 0 do not make a prediction
         // The blended model GHI may have different values for missing data:
         // 9999.0, netCDF Missing float value, ~0
         //
         if ( fabs( predictorVals[ (int) predictorVals.size() - 2] + 9999.0 ) < .0000001  ||
              fabs( predictorVals[ (int) predictorVals.size() - 2] - FCST_MISSING ) < .0000001 ||
              fabs( predictorVals[ (int) predictorVals.size() - 2] < .0000001 ))
         {
            pctCap.push_back(FCST_MISSING);

            prediction = FCST_MISSING;
         }
         else
         { 
            pctCap.push_back(prediction);
         }
         //
         // Output debug messages at various levels 
         // 
         if (DebugLevel > 1)
         {
            if(DebugLevel > 2)
            {
               Logg->write_time("Info: cubistInputStr: %s\n", 
                                 cubistInputStr.c_str());
            }

            struct tm * tmPtr;

            time_t t = fcstTime;

            tmPtr = gmtime ( &t );

            int year = tmPtr->tm_year + 1900;

            int month = tmPtr->tm_mon +1;

            int day = tmPtr->tm_mday;

            int hour = tmPtr->tm_hour ;

            int min = tmPtr->tm_min;

            char date[32];

            sprintf( date, "%.4d%.2d%.2dT%.2d:%.2d", year, month, day, hour, min);
            
            if ( fabs( predictorVals[ (int) predictorVals.size() - 2] - FCST_MISSING ) < .0000001)
            {
               Logg->write_time("Info: FcstNum  %d, PctCap: MISSING,  fcstTime: %lf or %s\n",
                              i,  fcstTime,date);  
            }
            else
            { 
               Logg->write_time("Info: FcstNum  %d, PctCap: %f,  fcstTime: %lf or %s\n", 
                                 i, prediction, fcstTime,date);
            }
         } // end if debug  
      } // end looping on forecast leads
   } // end loop on sites 

   return 0;
}

void FcstProcessor::writeNetcdf(const string cdlFile, const string outputDir,
                          const double genTime)
{
  // 
  // Create time string-- Time on file will be first fcst time
  //
  time_t gTime = genTime;

  tm *timePtr = gmtime(&gTime);

  char timeStr[16];

  strftime(timeStr,16, "%Y%m%d.%H%M00",timePtr);
    
  //  
  // Create the output filepath
  //  
  // Get the model base name
  //  
  std::size_t found = args.cubistModel.find_last_of("/");
      
  string modelBase = args.cubistModel.substr(found + 1);
      
  string outfile = outputDir + "/" +  "power_pct_cap." + modelBase + "." + timeStr + ".nc";
      
  Logg->write_time("Info: Writing output to %s\n", outfile.c_str());  
      
  //  
  // Create output netCDF file 
  //  
  cdf_field_writer cdf_file(cdlFile, outfile);  

  //  
  // Log the creation time of output file
  //  
  vector<double> ctime;
      
  ctime.push_back(time(0));
  
  string errorStr;
      
  cdf_file.put_field(string("creation_time") , ctime, errorStr);
      
  //
  // Put fcst data and forecast times in output file
  //
  cdf_file.put_field(string("valid_times"), validTimes, errorStr);

  //
  // Put integer site IDs in output file
  //
  cdf_file.put_field(string("siteId"), siteIds, errorStr);

  // 
  //   Put data in the file 
  // 
  cdf_file.put_field(string("power_percent_capacity"), pctCap, errorStr);
}

int FcstProcessor::loadCubistModel( )
{
   string modelStr = args.cubistModel;

   //
   // Instantiate the interface to the Cubist model
   //
   cubist_interface *cubistInterfacePtr = new  cubist_interface(modelStr);

   if (cubistInterfacePtr == NULL)
   {
     Logg->write_time("Error: Failure to initialize cubist model with cubist"
                      " basename: %s\n", modelStr.c_str());

     return 1;
   }
   else
   {
     cubistModel = cubistInterfacePtr;
      
     if (DebugLevel > 1)
     {
       Logg->write_time("Info: Initialized cubist model with cubist basename: "
                        "%s\n", modelStr.c_str());
     }
   }
   return 0;
}
   

void FcstProcessor::loadPredictors(const double fcstTime, const double fcstGenTime,
                                   const int siteId, vector <float> & predictorVals, 
                                   BlendedModelMgr &modelMgr)
{
   //
   // variables at forecast time: month of year, temperature, relative humidity, climate zone
   // and ghi 
   //
   
   int monthOfYear;
   struct tm * tmPtr;
   time_t t = fcstTime;
   tmPtr = gmtime ( &t );
   monthOfYear = tmPtr->tm_mon +1;
   predictorVals.push_back(monthOfYear);

   float tFcst = modelMgr.getTemp(siteId, fcstTime);
   predictorVals.push_back(tFcst);

   float rhFcst = modelMgr.getRh(siteId, fcstTime);
   predictorVals.push_back(rhFcst);

   // climate zone 
   int climateZone = modelMgr.getClimateZone(siteId);
   predictorVals.push_back(climateZone);

   //
   // Get GHI 
   //
   float ghiFcst = modelMgr.getGHI(siteId, fcstTime);
   predictorVals.push_back(ghiFcst);

   //
   // The predictand place holder
   //
   float predPlaceHold = FCST_MISSING;
   predictorVals.push_back(predPlaceHold);

   if (DebugLevel > 1)
   { 
      Logg->write_time("Info: SiteId: %d, genTime: %.0lf, leadNum: %d\n",
                       siteId, fcstGenTime, (int)(fcstTime - fcstGenTime)/modelMgr.getFcstResolution());
      Logg->write_time("fcst resolution: %d\n", modelMgr.getFcstResolution());
       
      Logg->write(" Model Values: \n");
      Logg->write(" Month  %d\n", monthOfYear);

      if(tFcst  == FCST_MISSING)
         Logg->write(" T2  MISSING\n" );
      else
         Logg->write(" T2  %f\n", tFcst);
   
     if( rhFcst  == FCST_MISSING)
         Logg->write(" RH  MISSING\n" );
      else
         Logg->write(" RH  %f\n", rhFcst);
  
      if( climateZone == FCST_MISSING)
         Logg->write(" climateZone  MISSING\n" );
      else
         Logg->write(" climateZone  %d\n", climateZone);
  
      if( ghiFcst == FCST_MISSING)
         Logg->write(" GHI  MISSING\n" );
      else
         Logg->write(" ghi  %f\n", ghiFcst);
  
   }
}

void FcstProcessor::createCubistInputStr(const vector <float> predictorVals, 
                                   string &cubistInputStr)
{
   //
   // Start the input string with ignored Date, integer month of the year,
   // two more indicators of ignored data for the site estimated capacity
   //
   if (DebugLevel > 3)
   {
      Logg->write(" \ncubist input to align with names file\n ?\n %d\n ?\n ?\n", (int) predictorVals[0] );
   }

   char numStr[32];
   sprintf(numStr,"%d",(int)predictorVals[0]);
   cubistInputStr = "?," + string(numStr) + ",?,?,";  

   //
   // Now add the model values: T (float), RH(float), climate zone(int), GHI(float)
   //
   for (int i = 1; i < (int) predictorVals.size(); i++)
   {
      //
      // Check for missing data values -9999.0, -999.0, -9, NetCDF default missing
      // value 
      // 
      if ((fabs(predictorVals[i] - FCST_MISSING) > .00000001  ) &&
          (fabs(predictorVals[i] + 9999.0) > .00000001)&&
          (fabs(predictorVals[i] + 999.0)  > .00000001) && 
          (fabs(predictorVals[i] + 9.0)    > .00000001))  
      { 

         if ( i != 3)
         {
            sprintf(numStr,"%.6f,", predictorVals[i]);
         }
         else
         {
            sprintf(numStr,"%d,", (int) predictorVals[i]);
         } 
         cubistInputStr = cubistInputStr + string(numStr);
         if (DebugLevel > 3)
          {
             Logg->write(" %s\n",numStr);
          }
      }
      else
      {
        cubistInputStr = cubistInputStr + string("?,");
        if (DebugLevel > 3)
        {
           Logg->write(" ?\n");
        }
      }
   }
   //
   // Erase the last comma
   //
   cubistInputStr.erase( cubistInputStr.end() -1);
}
