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

const float FcstProcessor::CUBIST_MISSING = NC_FILL_FLOAT;

FcstProcessor::FcstProcessor(const Arguments &argsParam):
  args(argsParam)
{ 
  error = string("");
}

FcstProcessor::~FcstProcessor()
{  
  if( siteMgr)
  {
     delete siteMgr;
  }
  for (int i =0; i< (int) leadTimeCubistModels.size();i++)
  {
     if (leadTimeCubistModels[i])
     {
        delete leadTimeCubistModels[i];
     }
  } 
  leadTimeCubistModels.clear();
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
  NwpMgr nwpMgr;

  if ((int) args.nwpFiles.size() == 0)
  {
     Logg->write_time("ERROR: No NWP data available. ghi_fcst cannot run.\n");
     
     return 1;
  }

  for (int i = 0; i < (int) args.nwpFiles.size(); i++)
  {
    if (DebugLevel > 1)
    {
      Logg->write_time("Info: Reading wrf-solar file %s\n", 
		       args.nwpFiles[i].c_str());

    }

    NwpReader *nwpReader = new NwpReader(args.nwpFiles[i]);  

    nwpReader->parse();

    string nwpError = nwpReader->getError();

    if ( strcmp(nwpError.c_str(),"") != 0 )
    {
      Logg->write_time("ERROR: Failure to reading wrf-solar file %s: %s\n", 
		       args.nwpFiles[i].c_str(), nwpError.c_str());

      return 1;
    }
    else
    {
      nwpMgr.add(nwpReader);
    }
  }

  //
  // Instantiate manager of observations files and load files 
  //
  ObsMgr obsMgr;

  if ((int) args.obsFiles.size() == 0)
  {
     Logg->write_time("ERROR: No Observation data available. ghi_fcst cannot run.\n");

     return 1;
  }

  for (int i = 0; i < (int) args.obsFiles.size(); i++)
  {
    if (DebugLevel > 1)
    {
      Logg->write_time("Info: Reading observations file %s\n", 
		       args.obsFiles[i].c_str());
    }
     
    ObsReader *obsReader = new ObsReader(args.obsFiles[i], 900);    
     
    //
    // Parse the file
    //
    if (obsReader->parse())
    {
      string obsError = obsReader->getError();

      Logg->write_time("Error: Failure to read netCDF file %s: %s", 
		       args.obsFiles[i].c_str(), obsError.c_str());
      
      return 1;
    }
    else
    {
       obsMgr.add(obsReader);
    }
  }

  //
  // Create cubist interface objects for each lead time
  // Cubist is the statistical learning algorithm used
  // to create machine learning models
  //
  if( loadCubistModels())
  {
     Logg->write_time("Error: Cubist interface did not initialize properly "
                      "for one or more models" );

     return 1;
  }

  //
  // Instantiate siteMgr for integer and string siteIDs
  // The manager contains the sites to be processed 
  // 
  siteMgr = new SiteMgr(args.siteIdFile);
 
  if( siteMgr->parse())
  {
     Logg->write_time("Error: Failure to read siteID file:  ",
                       args.siteIdFile.c_str());
      return 1;
   }
 
  //
  // Use forecast of meteorological variables, current observations, previous 
  // observations of predictand, and a cubist_interface object to calculate and 
  // then store forecast values for each site at each lead time
  //
  double fcstGenTime;

  if ( predict(nwpMgr, obsMgr, fcstGenTime))
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

int FcstProcessor::predict( NwpMgr &nwpMgr, ObsMgr &obsMgr, double &fcstGenTime) 
{
   for( int s = 0; s < siteMgr->getNumSites(); s++)
   {
      //
      // Get and save the site IDs and site names for use or for the output file
      //
      int siteId = siteMgr->getSiteId(s);

      siteIds.push_back(siteId);

      siteNames.push_back( siteMgr->getSiteName(s));
   

      //
      // Get the generation time from the  optional input arg 'fcstStartTime' 
      // (if using observation data as a trigger) or from most recent input 
      // NWP file (NWP model trigger)
      //  
      if (args.fcstStartTime >= 0)
      {
         fcstGenTime = args.fcstStartTime;
      }
      else
      {
         time_t genTime = nwpMgr.getMostRecentGenTime();

         fcstGenTime = genTime;
      }

      //
      // Loop on forecasts being computed (either all possible forecasts or 
      // a subset (which is an optional command line arg). Set the fcstLeadBound 
      // and firstFcstTime variables appropriately. 
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
         // Get the predictors from observations, NWP model data, or variables 
         // derived from these values
         //
         loadPredictors(fcstTime, fcstGenTime, siteId, predictorVals, nwpMgr,
                        obsMgr);

         //
         // Convert vector of input predictor values to a string to satsify 
         //   cubist interface API
         // 
         string cubistInputStr = string("");

         createCubistInputStr( predictorVals, cubistInputStr);

         //
         // Initialize the predictions to missing 
         // 
         float prediction = CUBIST_MISSING;

         float ghiPrediction = CUBIST_MISSING;
 
         //
         // Check for forecasted NWP elevation-- if this is missing, the NWP data
         // for this site and lead must all be missing so we dont make a prediction
         //
         if ( nwpMgr.getToa(siteId,fcstTime) != NwpReader::NWP_MISSING)
         { 
            //
            // Get the prediction using the cubist interface 
            //
            prediction =   leadTimeCubistModels[i-1]->predict(cubistInputStr);

            //
            // The predictand is Clearness Index, Kt, then bound the 
            // result by 0 below and 1 above
            //
            if (prediction > 1)
            {
               prediction = 1;
            }
            else if (prediction < 0)
            {
               prediction = 0;
            }

            //
            // Compute GHI at lead time by multiplying Kt by TOA at lead time
            //
            ghiPrediction = prediction * nwpMgr.getToa(siteId,fcstTime);
         }

         ktAll.push_back(prediction); 
        
         ghiAll.push_back(ghiPrediction);

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
            if ( prediction == CUBIST_MISSING)
            { 
             Logg->write_time("Info: FcstNum  %d, Kt: MISSING, fcstTime: %lf or %s\n",
                              i,  fcstTime,date);  
            }
            else
            {
               Logg->write_time("Info: FcstNum  %d, Kt: %f, fcstTime: %lf or %s\n", 
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
      
  string outfile = outputDir + "/" +  "ghi_fcst." + modelBase + "." + timeStr + ".nc";
      
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
  cdf_file.put_field("GHI", ghiAll, errorStr);

  cdf_file.put_field("Kt", ktAll, errorStr);

  cdf_file.put_field("wrfGHI", wrfGhiAll, errorStr); 

  cdf_file.put_field("wrfKt", wrfKtAll, errorStr);

  cdf_file.put_field("solarEl", solarElAll, errorStr);

  cdf_file.put_field("TOA", toaAll, errorStr);
 
  cdf_file.put_field("wrfTOA", wrfToaAll, errorStr);
}

int FcstProcessor::loadCubistModels( )
{

   //
   // Instantiate the cubist interface for each lead time 
   //
   for( int i = 1; i <= args.fcstLeadsNum; i++)
   {
      //
      // Create the lead time model string
      //
      char leadBuf[4];

      sprintf(leadBuf,"%.3d", (i * args.fcstLeadsDelta));

      string leadTimeModelStr = args.cubistModel + string(".lt") + string(leadBuf);

      //
      // Instantiate the interface to the Cubist model
      //
      cubist_interface *cubistInterfacePtr = new  cubist_interface(leadTimeModelStr);

      if (cubistInterfacePtr == NULL)
      {
        Logg->write_time("Error: Failure to initialize cubist model with cubist"
                         " basename: %s\n", leadTimeModelStr.c_str());

        return 1;
      }
      else
      {
        leadTimeCubistModels.push_back(cubistInterfacePtr);
        
        if (DebugLevel > 1)
        {
          Logg->write_time("Info: Initialized cubist model with cubist basename: "
                           "%s\n", leadTimeModelStr.c_str());
        }
      }
   }
   return 0;
}
   

void FcstProcessor::loadPredictors(const double fcstTime, const double fcstGenTime,
                             const int siteId, vector <float> & predictorVals, 
                             NwpMgr &nwpMgr, ObsMgr &obsMgr)
{

   //
   // Get Observerations at forecast generation time
   //
   float t = obsMgr.getTemp(siteId, fcstGenTime);
   predictorVals.push_back(t);

   // RH obs not used by models
   float rh = obsMgr.getRh(siteId, fcstGenTime);
   predictorVals.push_back(rh);
        
   // GHI not used in models
   float obsGhi = obsMgr.getGHI(siteId, fcstGenTime);
   predictorVals.push_back(CUBIST_MISSING);

   // Pressure not used in models
   float p = obsMgr.getPressure(siteId, fcstGenTime);
   predictorVals.push_back(p);

   // Wind speed not used by models
   float ws = obsMgr.getWindSpeed(siteId, fcstGenTime);
   // not used
   predictorVals.push_back(CUBIST_MISSING);

   // Wind direction not used by models
   float wd = obsMgr.getWindDir(siteId, fcstGenTime);
   // not used
   predictorVals.push_back(CUBIST_MISSING);

   float el = obsMgr.getElevation(siteId, fcstGenTime);
   predictorVals.push_back(el);

   float az = obsMgr.getAzimuth(siteId, fcstGenTime);
   predictorVals.push_back(az);

   // Used for analysis but not prediction
   float obsToa =  obsMgr.getToa(siteId, fcstGenTime);
   //not used
   predictorVals.push_back(CUBIST_MISSING);

   float obsKt = obsMgr.getKt(siteId, fcstGenTime);
   predictorVals.push_back(obsKt);
   
   //
   // Get previous 45 minutes of values of Clearness Index, Kt
   //
   float prev15Kt = obsMgr.getKt(siteId, fcstGenTime - 900);
   predictorVals.push_back(prev15Kt);
  
   float prev30Kt = obsMgr.getKt(siteId, fcstGenTime - 1800);
   predictorVals.push_back(prev30Kt);

   float prev45Kt = obsMgr.getKt(siteId, fcstGenTime - 2700);
   predictorVals.push_back(prev45Kt);

   //
   // The predictand place holder
   //
   float predPlaceHold = CUBIST_MISSING;
   predictorVals.push_back(predPlaceHold);

   //
   // Get Solar variables at forecast time
   //
  
   //
   // TOA not used for prediction but recorded for analysis
   // 
   float toaFcst = nwpMgr.getToa(siteId, fcstTime);
   predictorVals.push_back(CUBIST_MISSING);
   toaAll.push_back(toaFcst);

   float azFcst = nwpMgr.getAzimuth(siteId, fcstTime);
   predictorVals.push_back(azFcst);

   float elFcst = nwpMgr.getElevation(siteId, fcstTime);
   predictorVals.push_back(elFcst);
   solarElAll.push_back(elFcst);
   
   //
   // Get GHI at forecast time (used in training not predicting) 
   //
   float obsGHIAtFcst = CUBIST_MISSING;
   predictorVals.push_back(obsGHIAtFcst);

   //
   // Get NWP vars generation time
   //
   float mr = nwpMgr.getMixingRatio(siteId, fcstGenTime);
   predictorVals.push_back(mr);

   // model GHI at gen time not used in model
   float wrfGhiGen = nwpMgr.getGHI(siteId, fcstGenTime);
   // not used
   predictorVals.push_back(CUBIST_MISSING); 

   float dniGen = nwpMgr.getDNI(siteId, fcstGenTime);
   predictorVals.push_back(dniGen);

   float dhiGen = nwpMgr.getDHI(siteId, fcstGenTime);
   predictorVals.push_back(dhiGen);

   float toadGen = nwpMgr.getTaod5502d(siteId, fcstGenTime);
   predictorVals.push_back(toadGen);

   float cloudFracGen = nwpMgr.getCloudFrac(siteId, fcstGenTime);
   // not used
   predictorVals.push_back(CUBIST_MISSING);

   float wvpGen = nwpMgr.getWvp(siteId, fcstGenTime);
   predictorVals.push_back(wvpGen);

   float wpTot = nwpMgr.getWpTot(siteId, fcstGenTime);
   predictorVals.push_back(wpTot);

   float tauQcTotGen = nwpMgr.getTauQcTot(siteId, fcstGenTime);
   predictorVals.push_back(tauQcTotGen);

   float tauQsGen = nwpMgr.getTauQs(siteId, fcstGenTime);
   predictorVals.push_back(tauQsGen);

   float tauQiTot = nwpMgr.getTauQiTot(siteId, fcstGenTime);
   predictorVals.push_back(tauQiTot);

   //
   // NWP variables at forecast time
   //
   float tFcst = nwpMgr.getTemp(siteId, fcstTime);
   predictorVals.push_back(tFcst);

   float mrFcst = nwpMgr.getMixingRatio(siteId, fcstTime);
   predictorVals.push_back(mrFcst);

   float pFcst = nwpMgr.getPsfc(siteId, fcstTime);
   predictorVals.push_back(pFcst);

   //
   // Wind speed not used in any model
   float wsFcst = nwpMgr.getWindSpeed(siteId, fcstTime);
   predictorVals.push_back(CUBIST_MISSING);

   //
   // Wind direction not used in any model
   //
   float wdFcst = nwpMgr.getWindDir(siteId, fcstTime);
   predictorVals.push_back(CUBIST_MISSING);

   // GHI not used in models 
   float wrfGhiFcst = nwpMgr.getGHI(siteId, fcstTime);
   predictorVals.push_back(CUBIST_MISSING);
   // Keep it for post analysis
   wrfGhiAll.push_back(wrfGhiFcst); 

   float dniFcst = nwpMgr.getDNI(siteId, fcstTime);
   predictorVals.push_back(dniFcst);

   float dhiFcst = nwpMgr.getDHI(siteId, fcstTime);
   predictorVals.push_back(dhiFcst);

   float toadFcst = nwpMgr.getTaod5502d(siteId, fcstTime);
   predictorVals.push_back(toadFcst);

   float cldFracFcst = nwpMgr.getCloudFrac(siteId, fcstTime);
   predictorVals.push_back(cldFracFcst);

   float wvpFcst = nwpMgr.getWvp(siteId, fcstTime);
   predictorVals.push_back(wvpFcst);

   float wpTotFcst = nwpMgr.getWpTot(siteId, fcstTime);
   predictorVals.push_back(wpTotFcst);

   float tauQcTotFcst = nwpMgr.getTauQcTot(siteId, fcstTime);
   predictorVals.push_back(tauQcTotFcst);

   float tauQsFcst = nwpMgr.getTauQs(siteId, fcstTime);
   predictorVals.push_back(tauQsFcst);

   float tauQiTotFcst = nwpMgr.getTauQiTot(siteId, fcstTime);
   predictorVals.push_back(tauQiTotFcst);

   float wrfKtFcst = nwpMgr.getKt(siteId, fcstTime);
   if ( fabs( wrfKtFcst + 999) > .0000001)
      predictorVals.push_back(wrfKtFcst);
   else
      predictorVals.push_back(CUBIST_MISSING);
   // record for post analysis
   wrfKtAll.push_back(wrfKtFcst);

   // for post analysis
   float wrfToa2 = nwpMgr.getWrfToa2(siteId, fcstTime);
   wrfToaAll.push_back(wrfToa2);

   if (DebugLevel > 1)
   { 
      Logg->write_time("Info: SiteId: %d, genTime: %.0lf, leadTime: "
                       "%.0lf, leadNum: %d\n", siteId, fcstGenTime, fcstTime,
                        (int)(fcstTime -fcstGenTime)/900);
    
       
      Logg->write(" Observation and NWP Values: \n");

      if( t == CUBIST_MISSING)
         Logg->write(" obsT        MISSING\n");
      else
         Logg->write(" obsT        %f\n",t );
    
      if( rh == CUBIST_MISSING)
         Logg->write(" obsRh       MISSING\n");
      else
         Logg->write(" obsRh       %f\n", rh );

      if( obsGhi == CUBIST_MISSING)
         Logg->write(" obsGhi      MISSING\n" );
      else
         Logg->write(" obsGhi      %f\n", obsGhi); 

      if ( p == CUBIST_MISSING)
         Logg->write(" obsP        MISSING\n" );
      else
         Logg->write(" obsP        %f\n", p );

      if( ws  == CUBIST_MISSING)
         Logg->write(" obsWs       MISSING\n" );
      else
         Logg->write(" obsWs       %f\n", ws);

      if( wd  == CUBIST_MISSING)
         Logg->write(" obsWd       MISSING\n" );
      else
         Logg->write(" obsWd       %f\n", wd);

      if(el  == CUBIST_MISSING)
         Logg->write(" obsEl       MISSING\n" );
      else
         Logg->write(" obsEl       %f\n", el);

      if(  az == CUBIST_MISSING)
         Logg->write(" obsAz       MISSING\n" );
      else
         Logg->write(" obsAz       %f\n",az );

      if( obsToa == CUBIST_MISSING)
         Logg->write(" obsToa      MISSING\n" );
      else
         Logg->write(" obsToa      %f\n", obsToa);

      if( obsKt  == CUBIST_MISSING)
         Logg->write(" obsKt       MISSING\n" );
      else
         Logg->write(" obsKt       %f\n", obsKt);

      if(  prev15Kt == CUBIST_MISSING)
         Logg->write(" prev15Kt    MISSING\n" );
      else
         Logg->write(" prev15Kt    %f\n", prev15Kt);

      if( prev30Kt == CUBIST_MISSING)
         Logg->write(" prev30Kt    MISSING\n" );
      else
         Logg->write(" prev30Kt    %f\n", prev30Kt);

      if(  prev45Kt == CUBIST_MISSING)
         Logg->write(" prev45Kt    MISSING\n" );
      else
         Logg->write(" prev45Kt    %f\n", prev45Kt);

      if( predPlaceHold == CUBIST_MISSING)
         Logg->write(" predPlace   MISSING\n" );
      else
         Logg->write(" predPlace   %f\n", predPlaceHold);

      if(  toaFcst == CUBIST_MISSING)
         Logg->write(" toaF        MISSING\n" );
      else
         Logg->write(" toaF        %f\n", toaFcst);

      if(azFcst  == CUBIST_MISSING)
         Logg->write(" azF         MISSING\n" );
      else
         Logg->write(" azF         %f\n", azFcst);
      
      if( elFcst == CUBIST_MISSING)
         Logg->write(" elF         MISSING\n" );
      else
         Logg->write(" elF         %f\n", elFcst);
     
      if( obsGHIAtFcst == CUBIST_MISSING)
         Logg->write(" obsGhiF     MISSING\n" );
      else
         Logg->write(" obsGhiF     %f\n", obsGHIAtFcst);
      
      if( mr == CUBIST_MISSING)
         Logg->write(" qWrfG       MISSING\n" );
      else
         Logg->write(" qWrfG       %f\n", mr);
      
      if( wrfGhiGen == CUBIST_MISSING)
         Logg->write(" ghiWrfG     MISSING\n" );
      else
         Logg->write(" ghiWrfG     %f\n", wrfGhiGen);
      
      if( dniGen == CUBIST_MISSING)
         Logg->write(" dniWrfG     MISSING\n" );
      else
         Logg->write(" dniWrfG     %f\n", dniGen);
     
      if( dhiGen  == CUBIST_MISSING)
         Logg->write(" dhiWrfG     MISSING\n" );
      else
         Logg->write(" dhiWrfG     %f\n", dhiGen);
    
      if(  toadGen == CUBIST_MISSING)
         Logg->write(" taodWrfG    MISSING\n" );
      else
         Logg->write(" taodWrfG    %f\n", toadGen);
   
       if( cloudFracGen == CUBIST_MISSING)
         Logg->write(" cldWrfG     MISSING\n" );
      else
         Logg->write(" cldWrfG     %f\n", cloudFracGen);
   
      if( wvpGen == CUBIST_MISSING)
         Logg->write(" wvpWrfG     MISSING\n" );
      else
         Logg->write(" wvpWrfG     %f\n", wvpGen);
  
      if( wpTot == CUBIST_MISSING)
         Logg->write(" wpTotWrfG   MISSING\n" );
      else
         Logg->write(" wpTotWrfG   %f\n", wpTot);
    
      if(tauQcTotGen  == CUBIST_MISSING)
         Logg->write(" tauQcTWrfG  MISSING\n" );
      else
         Logg->write(" tauQcTWrfG  %f\n", tauQcTotGen);
   
     if( tauQsGen  == CUBIST_MISSING)
         Logg->write(" tauQsWrfG   MISSING\n" );
      else
         Logg->write(" tauQsWrfG   %f\n", tauQsGen);
  
      if( tauQiTot == CUBIST_MISSING)
         Logg->write(" tauQiTWrfG  MISSING\n" );
      else
         Logg->write(" tauQiTWrfG  %f\n", tauQiTot);
  
      if( tFcst == CUBIST_MISSING)
         Logg->write(" TWrfF       MISSING\n" );
      else
         Logg->write(" TWrfF       %f\n", tFcst);
  
      if(  mrFcst == CUBIST_MISSING)
         Logg->write(" qWrfF       MISSING\n" );
      else
         Logg->write(" qWrfF       %f\n", mrFcst);
      
      if( pFcst == CUBIST_MISSING)
         Logg->write(" pWrfF       MISSING\n" );
      else
         Logg->write(" pWrfF       %f\n", pFcst);
      
      if( wsFcst == CUBIST_MISSING)
         Logg->write(" wsWrfF      MISSING\n" );
      else
         Logg->write(" wsWrfF      %f\n", wsFcst);
     
      if(  wdFcst == CUBIST_MISSING)
         Logg->write(" wdWrfF      MISSING\n" );
      else
         Logg->write(" wdWrfF      %f\n", wdFcst);
     
      if(  wrfGhiFcst == CUBIST_MISSING)
         Logg->write(" ghiWrfF     MISSING\n" );
      else
         Logg->write(" ghiWrfF     %f\n", wrfGhiFcst);
     
      if( dniFcst == CUBIST_MISSING)
         Logg->write(" dniWrfF     MISSING\n" );
      else
         Logg->write(" dniWrfF     %f\n", dniFcst);
    
      if( dhiFcst  == CUBIST_MISSING)
         Logg->write(" dhiWrfF     MISSING\n" );
      else
         Logg->write(" dhiWrfF     %f\n", dhiFcst);
    
      if(   toadFcst == CUBIST_MISSING)
         Logg->write(" taodWrfF    MISSING\n" );
      else
         Logg->write(" taodWrfF    %f\n", toadFcst);
    
      if( cldFracFcst == CUBIST_MISSING)
         Logg->write(" cldWrfF     MISSING\n" );
      else
         Logg->write(" cldWrfF     %f\n", cldFracFcst);
 
      if( wvpFcst == CUBIST_MISSING)
         Logg->write(" wvpWrfF     MISSING\n" );
      else
         Logg->write(" wvpWrfF     %f\n", wvpFcst);
      
      if(  wpTotFcst == CUBIST_MISSING)
         Logg->write(" wpTWrfF     MISSING\n" );
      else
         Logg->write(" wpTWrfF     %f\n", wpTotFcst);
      
      if( tauQcTotFcst == CUBIST_MISSING)
         Logg->write(" tauQcTWrfF  MISSING\n" );
      else
         Logg->write(" tauQcTWrfF  %f\n", tauQcTotFcst);
      
      if( tauQsFcst == CUBIST_MISSING)
         Logg->write(" tauQsWrfF   MISSING\n" );
      else
         Logg->write(" tauQsWrfF   %f\n", tauQsFcst);
      
      if( tauQiTotFcst == CUBIST_MISSING)
         Logg->write(" tauQiTWrfF  MISSING\n" );
      else
         Logg->write(" tauQiTWrfF  %f\n", tauQiTotFcst);
      
      if( wrfKtFcst == CUBIST_MISSING)
         Logg->write(" ktWrfF      MISSING\n" );
      else
         Logg->write(" ktWrfF      %f\n", wrfKtFcst);
   }
}

void FcstProcessor::createCubistInputStr(const vector <float> predictorVals, 
                                   string &cubistInputStr)
{
   char numStr[32];
   //
   // Start the input string with 4 indicators of ignored data
   // for the date, lead time, mesonet site, valid time, and climate 
   // region that are a part of every training dataset
   //
   cubistInputStr = "?,?,?,?,?,";
   if (DebugLevel > 3)
   {
      Logg->write(" cubist input to align with names file\n ?\n ?\n ?\n ?\n ?\n");
   }

   //
   // Now add the observed and nwp values
   //
   for (int i = 0; i < (int) predictorVals.size(); i++)
   {
      //
      // There are a few missing data values to check for 
      //  
      if ( (predictorVals[i] != CUBIST_MISSING) && 
           (fabs(predictorVals[i] + 9999.0) > .00000001) && 
           (fabs(predictorVals[i] + 999.0)  >  .00000001 ))
      { 
         sprintf(numStr,"%.6f,", predictorVals[i]);

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
             Logg->write(" ?\n",numStr);
          }

      }
   }
   //
   // Erase the last comma
   //
   cubistInputStr.erase( cubistInputStr.end() -1);
}
