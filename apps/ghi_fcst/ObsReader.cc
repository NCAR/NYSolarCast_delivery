//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: ObsReader.cc,v $
//       Version: $Revision: 1.2 $  Dated: $Date: 2015/10/29 20:33:20 $
//
//==============================================================================

/**
 *
 * @file ObsReader.cc
 *
 * @note 
 *
 * @date 02/2018
 *
 */

// Include files 

#include <iostream>
#include <netcdfcpp.h>
#include <string.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>

#include <log/log.hh>
#include "ncfc/ncfc.hh"
#include "ObsReader.hh"

extern Log *Logg;
extern int DebugLevel;

using std::endl;
using std::cerr;
namespace fs = boost::filesystem;

const float ObsReader::OBS_MISSING = NC_FILL_FLOAT;
const float ObsReader::PI = 3.141592653589793;

ObsReader::ObsReader(const string &obsFilePath, const int obsDataResolution):
  inputFile(obsFilePath),
  obsDataResolutionSecs(obsDataResolution) 
{
  
}

int ObsReader::parse()
{
  error = string("");

  fs::path filePath(inputFile);
  if (!fs::exists(filePath))
    {
      error = string("Error: cdf file") + inputFile +  "does not exist";
      return 1;
    }

  //
  // NetCDF dimensions
  // 
  vector<string> dimNames;
  dimNames.push_back("rec_num");
  dimNames.push_back("station_name_dim");
 
  //
  // NetCDF variables
  // 
  vector<string> varNames;
  //varNames.push_back("station_name");
  varNames.push_back("stationID");
  varNames.push_back("observationTime");
  varNames.push_back("relative_humidity");
  varNames.push_back("T_2");
  varNames.push_back("solar_insolation");
  varNames.push_back("pressure");
  varNames.push_back("wind_speed");
  varNames.push_back("wind_dir");
  varNames.push_back("solar_elevation_angle");
  varNames.push_back("solar_azimuth_angle");
  varNames.push_back("TOA");
  varNames.push_back("Kt");

  //
  // Instantiate the netCDF input reader object
  //
  Var_input varInput(inputFile.c_str(), varNames, dimNames);

  //
  // Check for success or exit
  //
  int ret = varInput.error_status();
  if (ret != 0)
    {
      error = string("Error: Var_input constructor failed, error: ") + varInput.error() + 
	string(", return: ") + string(nc_strerror(ret)) + string("\n");
      
      return 1;
    } 
  
  //
  // Get the information about the data -- variable names, dimensions, data types
  //
  const vector<string> inVarNames = varInput.get_var_names();
  const vector<void *> inVarPtrs = varInput.get_var_ptrs();
  const vector<long int> inVarSizes = varInput.get_var_sizes();
  const vector<size_t *> inVarDims = varInput.get_var_dims();

  //
  // Create map from variable names to variable dimension lists 
  //
  map<string, size_t> fileDimMap;

  varInput.get_file_dim_map(fileDimMap);

  //
  // Create map from variable names to index of variable in file
  //
  map<string, uint> varIndexMap;

  varInput.get_var_index_map(varIndexMap);

  //
  // Get variables
  //

  //
  // Get the siteIds
  // 
  int ind = varIndexMap["stationID"];
  numSites = inVarSizes[ind];  
  for (int i=0; i<numSites; i++)
  {
    int site =  ((int *)inVarPtrs[ind])[i];
    siteList.push_back(site);
  }

  //
  // Get the observation times
  //
  ind = varIndexMap["observationTime"];
  numTimes = inVarSizes[ind];
  for (int i=0; i<numTimes; i++)
  {
    double t =   ((double *)inVarPtrs[ind])[i];

    timesList.push_back((double)t);  
  }

  //
  // Copy rh to vector
  //
  ind = varIndexMap["relative_humidity"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    rh.push_back(val);
  }

  //
  // Copy temp vector
  //
  ind = varIndexMap["T_2"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    temp.push_back(val);
  }

  //
  // Copy ghi data to vector
  //
  ind = varIndexMap["solar_insolation"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    if (val < 0)
       ghi.push_back(0);
    else
       ghi.push_back(val);
  }

  //
  // Copy pressure data to vector
  //
  ind = varIndexMap["pressure"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    pres.push_back(val);
  }
  
  //
  // Copy wind speed data to vector
  //
  ind = varIndexMap["wind_speed"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    windSpeed.push_back(val);
  }

  //
  // Copy wind direction data to vector
  //
  ind = varIndexMap["wind_dir"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    windDir.push_back(val);
  }

  //
  // Copy solar elevation data to vector
  //
  ind = varIndexMap["solar_elevation_angle"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    elevation.push_back(val);
  }

  //
  // Copy solar azimuth data to vector
  //
  ind = varIndexMap["solar_azimuth_angle"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    azimuth.push_back(val);
  }

  //
  // Copy TOA data to vector
  //
  ind = varIndexMap["TOA"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    toa.push_back(val);
  }

  //
  // Copy solar azimuth data to vector
  //
  ind = varIndexMap["Kt"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    kt.push_back(val);
  }

  //
  // Map siteIds to integer indices. This is used for calculating offsets of 
  // variable values
  //
  for (int i = 0; i < numSites; i++)
  {
    siteIdIndexMap[siteList[i]] = i;
  }
  return 0;
}

ObsReader:: ~ObsReader()
{
  
}

const int ObsReader::getArrayOffset(const int siteId, const double obsTime) 
{
 
  if ( siteIdIndexMap.find((int)siteId) == siteIdIndexMap.end() || 
       obsTime > timesList[ (int) timesList.size() -1] || 
       obsTime < timesList[0])
  {
    return -1;
  }
  else
  {
    int timeIndex = (obsTime - timesList[0])/obsDataResolutionSecs;

    int siteIndex = siteIdIndexMap[siteId];
  
    return  timeIndex * numSites + siteIndex;

  }
}

const bool ObsReader::haveData( const int siteId, const double obsTime) const
{
  if ( siteIdIndexMap.find(siteId) == siteIdIndexMap.end() || 
       obsTime > timesList[ (int) timesList.size() -1] ||
       obsTime < timesList[0])
  {
    return false;
    cerr << "Did not find data at " << obsTime << " for siteId " << endl; 
  }
  else
  {
    
    return true;
  }
}

void  ObsReader::getStartEndTimes( double &start, double &end) const
{
  start = timesList[0];

  end = timesList[(int) timesList.size() -1];
} 

const float ObsReader::getAzimuth(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return azimuth[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getElevation(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return elevation[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getGHI(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return ghi[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getKt(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return kt[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getPressure(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return pres[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getRh(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return rh[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getToa(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return toa[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getTemp(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);
  
  if ( arrayOffset >= 0) 
  {
    return temp[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getWindDir(const int siteId, const double obsTime)
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);

  if ( arrayOffset >= 0)
  {
    return windDir[arrayOffset];

  }
  else
  {
    return OBS_MISSING;
  }
}

const float ObsReader::getWindSpeed(const int siteId, const double obsTime) 
{
  int arrayOffset =  getArrayOffset(siteId, obsTime);
  
  if ( arrayOffset >= 0)
  { 
    return windSpeed[arrayOffset];
  }
  else
  {
    return OBS_MISSING;
  }
}
